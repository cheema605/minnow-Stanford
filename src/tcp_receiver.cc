#include "tcp_receiver.hh"
#include "debug.hh"

using namespace std;

void TCPReceiver::receive(TCPSenderMessage message) {
    // 0. Handle reset immediately
    if (message.RST) {
        reassembler_.writer().set_error();
        return;  // don’t process payload/FIN/SYN after reset
    }

    // 1. Set Initial Sequence Number (ISN) on first SYN
    if (!isn_set_ && message.SYN) {
        isn_ = message.seqno;
        isn_set_ = true;
    }

    // Can't process anything until we've seen the SYN
    if (!isn_set_) return;

    // 2. Compute checkpoint for unwrap (first unassembled)
    uint64_t checkpoint = reassembler_.reader().bytes_popped() + reassembler_.count_bytes_pending();

    // 3. Convert seqno -> absolute seqno (0-based: SYN -> 0)
    uint64_t abs_seq = message.seqno.unwrap(isn_, checkpoint);

    // 4. Compute stream index (0-based index of first payload byte).
    uint64_t stream_index64;
    if (message.SYN) {
        stream_index64 = abs_seq;
    } else {
        stream_index64 = abs_seq - 1;  // safe, since abs_seq >= 1 for payload
    }
    size_t stream_index = static_cast<size_t>(stream_index64);

    // 5. Always call insert so FIN is handled even when payload is empty
    reassembler_.insert(stream_index, message.payload, message.FIN);
}


TCPReceiverMessage TCPReceiver::send() const {
    TCPReceiverMessage msg;

    if (isn_set_) {
        uint64_t bytes_written = reassembler_.writer().bytes_pushed();
        uint64_t ack_abs = 1 + bytes_written;   // +1 for SYN
        if (reassembler_.writer().is_closed()) {
            ack_abs += 1;  // +1 for FIN
        }
        msg.ackno = Wrap32::wrap(ack_abs, isn_);
    }

    uint64_t avail = reassembler_.writer().available_capacity();
    msg.window_size = static_cast<uint16_t>(std::min<uint64_t>(avail, 65535));

    msg.RST = reassembler_.writer().has_error();

    return msg;
}
