My name: Hamza Akmal

My SUNet ID: 23L-0513

I collaborated with: 23L-0630

This checkpoint took me about 12 hours to do. I did attend the lab session.

Program Structure and Design of the TCPSender My TCPSender implementation maintains a queue (deque) of outstanding segments that have been transmitted but not yet acknowledged. Each outstanding segment is tracked with its absolute sequence number and retransmission timing data. The sender keeps variables for the next sequence number (next_seqno_abs_), the last acknowledged sequence number (last_acked_abs_), the current retransmission timeout (current_RTO_ms_), and the number of consecutive retransmissions.

The sender fills the receiver’s window by reading from the ByteStream and forming segments that respect the window size and TCP limits (like MAX_PAYLOAD_SIZE). When the sender receives an ACK, it removes fully acknowledged segments, resets the retransmission timer, and restarts the timer if there are still unacknowledged segments.

If the timer expires before an acknowledgment, the earliest unacknowledged segment is retransmitted, and the RTO doubles (exponential backoff). The implementation also correctly handles special cases, including zero-window probes, impossible ACK numbers, and stream errors that trigger RST segments.

Alternative designs considered:
I considered maintaining only the total bytes in flight instead of storing each outstanding segment. That would have been simpler but made retransmission tracking and exact acknowledgment removal harder. The current design, using a deque of full message records, is clearer and more robust against off-by-one and timing bugs.factors. 

Report from the Hands-on Component

I verified correctness using the provided unit tests (send_connect, send_ack, send_close, etc.). After debugging window and acknowledgment handling, all tests passed. I also reviewed timing and retransmission behavior by examining debug output and ensuring that retransmission occurred after timeout expiry and reset properly on acknowledgment.


Implementation Challenges: hte hardest parts were:

Handling impossible ACK numbers (ignoring ACKs beyond the next sequence number).

Correctly implementing the retransmission timer logic and exponential backoff.

Ensuring RST behavior after a stream error for both empty and non-empty segments.

Understanding how sequence numbers “wrap” using Wrap32 was also tricky at first, but became intuitive once tested against edge cases.

Remaining Bugs: none

