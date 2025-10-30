#include "tcp_sender.hh"
#include "debug.hh"
#include "tcp_config.hh"
#include "wrapping_integers.hh"
#include <algorithm>

using namespace std;

/* ---------------- Helper functions ---------------- */

void TCPSender::start_timer() {
  if (!timer_running_) {
    timer_running_ = true;
    time_since_timer_start_ms_ = 0;
  }
}

void TCPSender::stop_timer() {
  timer_running_ = false;
  time_since_timer_start_ms_ = 0;
}

void TCPSender::restart_timer() {
  timer_running_ = true;
  time_since_timer_start_ms_ = 0;
}

/* track an outstanding (sent but unacked) segment */
void TCPSender::track_outstanding(const TCPSenderMessage &m, uint64_t first_seqno_abs) {
  if (m.sequence_length() == 0) return; // do not track zero-length segments
  Outstanding os;
  os.msg = m;
  os.first_seqno_abs = first_seqno_abs;
  os.time_sent_ms = 0;
  outstanding_.push_back(std::move(os));
  if (!timer_running_) start_timer();
}

/* Remove outstanding segments that are fully acked by ack_abs (ack_abs is the absolute ackno) */
void TCPSender::remove_fully_acked(uint64_t ack_abs) {
  while (!outstanding_.empty()) {
    const Outstanding &os = outstanding_.front();
    uint64_t seg_first = os.first_seqno_abs;
    uint64_t seg_end = seg_first + os.msg.sequence_length(); // one past last
    if (seg_end <= ack_abs) {
      outstanding_.pop_front();
    } else {
      break;
    }
  }
}

/* ---------------- Accessors ---------------- */

uint64_t TCPSender::sequence_numbers_in_flight() const {
  uint64_t sum = 0;
  for (const auto &os : outstanding_) sum += os.msg.sequence_length();
  return sum;
}

uint64_t TCPSender::consecutive_retransmissions() const {
  return consecutive_retransmissions_;
}

/* ---------------- make_empty_message ----------------
   Create a zero-length message with seqno set correctly (consumes no sequence space).
*/
TCPSenderMessage TCPSender::make_empty_message() const {
  TCPSenderMessage m;
  m.SYN = false;
  m.FIN = false;
  m.payload.clear();
  m.seqno = Wrap32::wrap(next_seqno_abs_, isn_);

  // If the stream has suffered an error, mark this message as RST
  if (input_.has_error()) {
    m.RST = true;
  } else {
    m.RST = false;
  }

  return m;
}


/* ---------------- push ----------------
   Fill the receiver's window by reading from the ByteStream and sending segments.
*/
void TCPSender::push(const TransmitFunction& transmit) {
  // compute effective window (special-case: treat zero window as 1 for this call only)
  uint64_t effective_window = window_size_;
  if (effective_window == 0) effective_window = 1;

  // how many sequence numbers already in flight (from last_ack_abs_)
  uint64_t in_flight = sequence_numbers_in_flight();

  // available = (last_acked_abs_ + effective_window) - (last_acked_abs_ + in_flight)
  // simplify:
  if (last_acked_abs_ + in_flight >= last_acked_abs_ + effective_window) {
    // window full
    return;
  }

  // keep sending until window full or no more data (and SYN/FIN conditions)
  while (true) {
    uint64_t used = sequence_numbers_in_flight();
    uint64_t window_right = last_acked_abs_ + effective_window;
    if (window_right <= last_acked_abs_ + used) break;
    uint64_t avail = window_right - (last_acked_abs_ + used);
    if (avail == 0) break;

    TCPSenderMessage seg;
    seg.payload.clear();
    seg.SYN = false;
    seg.FIN = false;
    seg.RST = false;

    // include SYN if not yet sent
    if (!syn_sent_) {
      // SYN occupies one sequence number
      seg.SYN = true;
    }

    // figure out how much payload we can take
    size_t max_payload = 0;
    // available for payload = avail - (SYN?1:0)
    uint64_t after_syn = (seg.SYN ? (avail >= 1 ? avail - 1 : 0) : avail);
    if (after_syn > 0) {
      max_payload = static_cast<size_t>(std::min<uint64_t>(after_syn, TCPConfig::MAX_PAYLOAD_SIZE));
    } else {
      max_payload = 0;
    }

    // read up to max_payload bytes from the reader
    if (max_payload > 0) {
      // use the provided helper: read(Reader& reader, uint64_t max_len, string& out)
      string out;
      read(reader(), max_payload, out);
      if (!out.empty()) seg.payload = std::move(out);
    }

    // If the stream is finished (reader().is_finished()) and FIN not yet sent
    // and we have room for FIN, include FIN
    bool stream_finished = reader().is_finished();
    if (stream_finished && !fin_sent_) {
      // remaining available after SYN+payload:
      uint64_t used_now = seg.sequence_length(); // SYN + payload.size()
      if (avail > used_now) {
        seg.FIN = true;
      }
    }

    // If this segment uses zero sequence space (no SYN, no payload, no FIN), don't send.
    if (seg.sequence_length() == 0) {
      break;
    }

    // set seqno of segment to next_seqno_abs_
    seg.seqno = Wrap32::wrap(next_seqno_abs_, isn_);
    
    if (input_.has_error()) {
    seg.RST = true;
}

    // update flags tracking
    if (seg.SYN) syn_sent_ = true;
    if (seg.FIN) fin_sent_ = true;

    // advance next_seqno_abs_
    uint64_t seg_len = seg.sequence_length();
    uint64_t seg_first_abs = next_seqno_abs_;
    next_seqno_abs_ += seg_len;

    // transmit
    transmit(seg);

    // track as outstanding (if consumes sequence space)
    if (seg_len > 0) {
      track_outstanding(seg, seg_first_abs);
    }

    // If window is now full, break; loop condition will check
  }
}

/* ---------------- receive ----------------
   Process an incoming TCPReceiverMessage (ackno optional, window size).
*/
void TCPSender::receive(const TCPReceiverMessage& msg) {
  // If RST, mark stream error and return.
  if (msg.RST) {
    input_.set_error();
    return;
  }

  // Update window size immediately (even if ack missing/ignored).
  window_size_ = msg.window_size;

  // If no ack number is present, nothing more to do.
  if (!msg.ackno.has_value()) {
    return;
  }

  // Unwrap ackno into absolute sequence number (use next_seqno_abs_ as checkpoint).
  Wrap32 w = msg.ackno.value();
  uint64_t ack_abs = w.unwrap(isn_, next_seqno_abs_);

  // === IMPORTANT: ignore impossible ACKs beyond what we've sent ===
  // If the peer is claiming to have received bytes we never sent, ignore this ACK.
  if (ack_abs > next_seqno_abs_) {
    return;
  }

  // Ignore non-advancing acks.
  if (ack_abs <= last_acked_abs_) {
    return;
  }

  // This is a valid, new ack: update last ack and remove fully acknowledged segments.
  last_acked_abs_ = ack_abs;
  remove_fully_acked(ack_abs);

  // Reset retransmission timeout (RTO) and consecutive retransmission counter.
  current_RTO_ms_ = initial_RTO_ms_;
  consecutive_retransmissions_ = 0;

  // If outstanding segments remain, restart the timer; otherwise stop it.
  if (!outstanding_.empty()) {
    restart_timer();
  } else {
    stop_timer();
  }
}

/* ---------------- tick ----------------
   Time has passed; check retransmission timer and retransmit earliest outstanding segment if necessary.
*/
void TCPSender::tick(uint64_t ms_since_last_tick, const TransmitFunction& transmit) {
  if (!timer_running_) return;

  time_since_timer_start_ms_ += ms_since_last_tick;

  if (time_since_timer_start_ms_ < current_RTO_ms_) return;

  // timer expired
  if (!outstanding_.empty()) {
    Outstanding &os = outstanding_.front();
    // retransmit earliest outstanding
    transmit(os.msg);

    // Apply exponential backoff only if the window is nonzero (per lab text)
    if (window_size_ > 0) {
      ++consecutive_retransmissions_;
      current_RTO_ms_ *= 2;
    }

    // restart timer counting from zero
    time_since_timer_start_ms_ = 0;
    timer_running_ = true;
  } else {
    // nothing outstanding, stop timer
    stop_timer();
  }
}
