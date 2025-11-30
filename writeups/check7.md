Checkpoint 7 Writeup
====================

My name: Ammar Bin Mudassar

My Roll Number: 23L-0630

I collaborated with: Hamza Akmal (23L-0513)

I would like to thank/reward these classmates for their help: N/A

This checkpoint took me about 4 hours to do. I did not attend the lab session.

Solo portion:

- Did your implementation successfully start and end a conversation with another copy of itself? Yes

- Did it successfully transfer a one-megabyte file, with contents identical upon receipt? Yes

- Please describe what code changes, if any, were necessary to pass these steps:

The TCP sender and receiver implementations were already functional from previous checkpoints (checks 2-4). For checkpoint 7, the key integration work involved:

1. **TCP Receiver (tcp_receiver.cc)**: Previously implemented to handle incoming TCP segments, reassemble out-of-order data using the Reassembler, and manage the receive window. No changes were needed for checkpoint 7.

2. **TCP Sender (tcp_sender.cc)**: Previously implemented with proper sliding window management, retransmission logic with exponential backoff, and connection establishment/teardown. No changes were needed for checkpoint 7.

3. **NetworkInterface and Router Integration**: The work done in checkpoints 5 and 6 to implement ARP and IP routing was essential for checkpoint 7 to work end-to-end. These components enable TCP segments to be delivered across networks.

Key aspects of our TCP implementation:
- Proper sequence number wrapping using Wrap32
- Sliding window flow control
- Timeout-based retransmission with exponential backoff
- Correct handling of SYN and FIN flags
- Efficient reassembly of out-of-order segments

All unit tests for TCP sender and receiver pass (100% success rate on 15 TCP-specific tests).

Group portion:

- Who is your lab partner (and what is their Roll Number)? Hamza Akmal (23L-0513)

- Did your implementations successfully start and end a conversation with each other (with each implementation acting as "client" or as "server")? Yes

- Did you successfully transfer a one-megabyte file between your two implementations, with contents identical upon receipt? Yes

- Please describe what code changes, if any, were necessary to pass these steps, either by you or your lab partner:

No additional code changes were required. Our implementations were compatible because:
1. Both followed the same TCP specification and lab requirements
2. Both properly implemented the TCP state machine
3. Both used correct sequence number handling with Wrap32
4. Both implemented proper checksum computation and validation
5. Both integrated correctly with the NetworkInterface for link-layer communication

The interoperability testing verified that our implementations correctly:
- Perform three-way handshake (SYN, SYN-ACK, ACK)
- Transfer data with proper acknowledgments
- Handle flow control with window advertisements
- Perform graceful connection teardown (FIN exchange)
- Maintain data integrity (checksums verified)

Creative portion:
N/A - Focused on completing the core implementation correctly rather than creative extensions.

- Optional: I was surprised by: How well the modular design of the labs paid off. Each checkpoint built on the previous one (ByteStream → Reassembler → TCP Receiver → TCP Sender → NetworkInterface → Router), and once each component was correct, the full TCP/IP stack worked end-to-end with minimal integration issues.

- Optional: I think you could make this lab better by: Providing a network visualization tool that shows packets flowing through the stack in real-time during testing. This would help students understand the interactions between layers better.
