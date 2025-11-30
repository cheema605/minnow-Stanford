Checkpoint 5 Writeup
====================

My name: Ammar Bin Mudassar

My Roll Number: 23L-0630

I collaborated with: Hamza Akmal (23L-0513)

I would like to thank/reward these classmates for their help: N/A

This checkpoint took me about 6 hours to do. I did not attend the lab session.

Program Structure and Design of the NetworkInterface:

Our NetworkInterface implementation uses several key data structures:

1. **ARP Cache (_arp_table)**: An unordered_map<uint32_t, ARPEntry> that maps IP addresses (as uint32_t) to MAC addresses with TTL values. Each entry stores the Ethernet address and a time-to-live counter that expires after 30 seconds.

2. **ARP Request Timer (_arp_request_time)**: An unordered_map<uint32_t, size_t> that tracks when we last sent an ARP request for each IP address. This prevents flooding the network with ARP requests by enforcing a 5-second minimum interval between requests.

3. **Waiting Queue (_waiting)**: An unordered_map<uint32_t, vector<InternetDatagram>> that stores datagrams waiting for ARP resolution. When we receive an ARP reply, all queued datagrams for that IP are transmitted immediately.

4. **OutputPort Pattern**: We implemented a dependency injection pattern using an abstract OutputPort class. This allows the NetworkInterface to be tested in isolation and integrated into larger network simulations.

Key Design Decisions:
- When an ARP request timeout occurs (>5 seconds), we clear the old queue and only keep new datagrams. This prevents accumulating stale datagrams and matches expected behavior.
- We use the serialize() helper function from helpers.hh for consistent serialization instead of manual Serializer usage.
- The transmit() method centralizes all frame output, checking if OutputPort is available before falling back to internal queue.

Performance: O(1) average case for ARP lookups and O(n) for routing table iteration where n is the number of pending datagrams per IP.

Implementation Challenges:

1. **API Confusion**: Initially used frame.header() and frame.payload() with parentheses, but these are member variables, not functions. Fixed by removing parentheses throughout.

2. **Serialization API**: Had to switch from direct Serializer usage to the serialize() helper function and use s.finish() instead of s.output().

3. **Parse Result Handling**: Changed from checking ParseResult::NoError to using the parse() helper that returns bool.

4. **ARP Queue Management**: The most challenging bug was handling expired ARP requests. The test "Pending datagrams dropped when pending request expires" revealed that we needed to clear old queued datagrams when sending a new ARP request after timeout, not just append to the queue.

5. **OutputPort Architecture**: Had to add the OutputPort abstraction and update the constructor signature to match the test harness expectations.

Remaining Bugs:
None - all network interface tests pass successfully.

- Optional: I was surprised by: The complexity of the ARP queue management logic, particularly the interaction between tick() incrementing timers and send_datagram() checking those timers. Getting the queue clearing logic right in both places was tricky.
