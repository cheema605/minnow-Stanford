Checkpoint 6 Writeup
====================

My name: Ammar Bin Mudassar

My Roll Number: 23L-0630

I collaborated with: Hamza Akmal (23L-0513)

I would like to thank/reward these classmates for their help: N/A

This checkpoint took me about 3 hours to do. I did not attend the lab session.

Program Structure and Design of the Router:

Our Router implementation is straightforward and efficient:

**Data Structures:**
1. **Routing Table (routing_table_)**: A vector<Route> storing routing entries. Each Route contains:
   - route_prefix: The network prefix to match
   - prefix_len: Length of the prefix (0-32 bits)
   - next_hop: Optional next hop address (if empty, send directly to destination)
   - interface_num: Index of the outgoing interface

2. **Interfaces (interfaces_)**: A vector of shared_ptr<NetworkInterface> representing all router interfaces.

**Algorithm - Longest Prefix Match (LPM):**
The route() function implements classic LPM routing:
1. Iterate through all interfaces and check their incoming datagram queues
2. For each datagram, scan the routing table to find the best matching route
3. Use bitwise masking to check if destination IP matches each route's prefix
4. Select the route with the longest matching prefix (most specific)
5. Drop packets if no route found or TTL expires
6. Decrement TTL and recompute checksum before forwarding
7. Forward to next hop or directly to destination based on route configuration

**Performance:** O(n*m) where n is the number of routes and m is the number of datagrams. For small routing tables this is acceptable, but for production routers, a trie or radix tree would be more efficient (O(log n) or O(1) with hardware acceleration).

**Alternative Designs Considered:**
- Trie-based routing table: Would improve lookup to O(log n) but adds complexity
- Sorting routes by prefix length: Could optimize but requires maintaining sorted order
- Hash-based exact match: Fast but doesn't support prefix matching

Our simple linear scan approach is appropriate for this lab given the small routing table sizes.

Implementation Challenges:

1. **Critical Bug - Missing Checksum Recomputation**: The initial implementation decremented TTL but forgot to recompute the IPv4 header checksum. This caused all forwarded packets to be rejected as "bad IPv4 datagram". Fixed by adding dgram.header.compute_checksum() after TTL decrement.

2. **Understanding Next Hop Logic**: Had to carefully handle two cases:
   - If next_hop is specified, forward to that address (indirect routing)
   - If next_hop is empty, forward directly to the destination IP (direct routing)

3. **Mask Calculation**: Correctly computing the subnet mask for prefix matching required careful bit manipulation: mask = (prefix_len == 0) ? 0 : (0xFFFFFFFF << (32 - prefix_len))

4. **Interface Integration**: Understanding how the router interacts with NetworkInterface's datagrams_received() queue and send_datagram() method.

Remaining Bugs:
None - all router tests pass successfully.

- Optional: I was surprised by: How critical the checksum recomputation is. Without it, the entire routing fails silently - packets are sent but rejected by recipients. This emphasizes the importance of maintaining protocol integrity at every hop.

- Optional: I think you could make this lab better by: Adding a test case that specifically checks for checksum correctness after TTL modification, so students can catch this bug more easily during development.
