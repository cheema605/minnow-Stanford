#pragma once

#include "byte_stream.hh"
#include "tcp_receiver_message.hh"
#include "tcp_sender_message.hh"
#include "wrapping_integers.hh"
#include "tcp_config.hh"

#include <cstdint>
#include <deque>
#include <functional>

class TCPSender
{
public:
  /* Construct TCP sender with given default Retransmission Timeout and possible ISN */
  TCPSender( ByteStream&& input, Wrap32 isn, uint64_t initial_RTO_ms )
    : input_( std::move( input ) ), isn_( isn ), initial_RTO_ms_( initial_RTO_ms ),
      current_RTO_ms_( initial_RTO_ms ),
      next_seqno_abs_( 0 ), last_acked_abs_( 0 ),
      window_size_( 1 ), outstanding_(), timer_running_( false ),
      time_since_timer_start_ms_( 0 ), consecutive_retransmissions_( 0 ),
      syn_sent_( false ), fin_sent_( false )
  {}

  /* Generate an empty TCPSenderMessage */
  TCPSenderMessage make_empty_message() const;

  /* Receive and process a TCPReceiverMessage from the peer's receiver */
  void receive( const TCPReceiverMessage& msg );

  /* Type of the `transmit` function that the push and tick methods can use to send messages */
  using TransmitFunction = std::function<void( const TCPSenderMessage& )>;

  /* Push bytes from the outbound stream */
  void push( const TransmitFunction& transmit );

  /* Time has passed by the given # of milliseconds since the last time the tick() method was called */
  void tick( uint64_t ms_since_last_tick, const TransmitFunction& transmit );

  // Accessors
  uint64_t sequence_numbers_in_flight() const;  // For testing: how many sequence numbers are outstanding?
  uint64_t consecutive_retransmissions() const; // For testing: how many consecutive retransmissions have happened?
  const Writer& writer() const { return input_.writer(); }
  const Reader& reader() const { return input_.reader(); }
  Writer& writer() { return input_.writer(); }

private:
  Reader& reader() { return input_.reader(); }

  struct Outstanding {
    TCPSenderMessage msg {};
    uint64_t first_seqno_abs {}; // absolute seqno of the first sequence number of this segment
    uint64_t time_sent_ms {};    // not used by tests, but kept for bookkeeping

    Outstanding() : msg(), first_seqno_abs(0), time_sent_ms(0) {}
  };

  // helpers
  void start_timer();
  void stop_timer();
  void restart_timer();
  void track_outstanding(const TCPSenderMessage &m, uint64_t first_seqno_abs);
  void remove_fully_acked(uint64_t ack_abs);

  // stream
  ByteStream input_;

  // wrapping / sequence
  Wrap32 isn_;

  // retransmission timeout state
  uint64_t initial_RTO_ms_;
  uint64_t current_RTO_ms_;

  // absolute sequence numbers:
  // next_seqno_abs_ is the absolute sequence number that will be assigned to the next sequence-space byte (or SYN if SYN not yet sent).
  uint64_t next_seqno_abs_;
  // highest ack we've seen (absolute number of next seq expected by receiver)
  uint64_t last_acked_abs_;

  // receiver window (as last advertised)
  uint16_t window_size_;

  // outstanding segments (oldest first)
  std::deque<Outstanding> outstanding_;

  // retransmission timer
  bool timer_running_;
  uint64_t time_since_timer_start_ms_;

  // retransmission bookkeeping
  uint64_t consecutive_retransmissions_;

  // flags about SYN/FIN
  bool syn_sent_;
  bool fin_sent_;
};