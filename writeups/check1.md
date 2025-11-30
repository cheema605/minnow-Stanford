Checkpoint 1 Writeup
====================

My name: Hamza Akmal

My SUNet ID: 23L-0513

I collaborated with: 23L-0630


This lab took US about 12-14 hours (combined) to do. I did attend the lab session.

I was surprised by or edified to learn that: how the reciever end of the TCP works and it s alot trickier to reassemble them in a meaningfull pattern.

Report from the hands-on component of the lab checkpoint:
In 2.1(4), I was able to fetch a page using my TCP implementation, which felt pretty cool because I could see the raw pieces being reassembled into meaningful data.

In 2.2, writing the Reassembler and testing it with the given harness helped me realize the importance of checking boundaries and making sure no “holes” get passed through as null bytes. At first, I saw a\x00bc in one of my tests instead of abcd, which showed me my merging logic had a bug. Fixing that was a satisfying moment.

Describe Reassembler structure and design.

 For my Reassembler, I used A map (ordered associative container) where the key is the starting index of a stored chunk, and the value is the substring.
 This lets me quickly find overlapping or adjacent chunks by searching near the insertion point.

Approach:
When a new chunk comes in, I trim it to fit inside the current writable window. check for overlaps/adjacent segments already in the map and merge them into one continuous region. I break the merged region into contiguous “hole-free” substrings and put them back into the map. Finally, if the next needed substring starts exactly at first_unassembled_index_, I push it straight into the output stream and advance. I track EOF separately so I only close the stream once all data up to that point has been assembled and written.

Alternative designs considered:
I thought about using a fixed-size array buffer with presence flags for each byte. This would simplify merging but would be very memory-heavy for large capacities (like 65,000). Another option was using a vector<pair<uint64_t,string>> instead of a map, but searching/merging would be slower and messier.

Benefits of my design:
Simple to reason about: every chunk is stored by its starting index. Using map made merging logic relatively straightforward. Memory-efficient compared to a giant array.

Weaknesses:
Somewhat verbose merging logic. More bookkeeping required (I had to do a “second pass” when merging because the range could expand). Not constant time — insertion and merging can be O(n) in the worst case, though acceptable for the project.

Performance:
it passed all tests in reasonable time. I think (not sure) it’s O(n log n) for insertions (due to map operations) plus merging, which is fine here.

Implementation Challenges:
they were handling overlapping segments, EOF segments and the window trimming.

Remaining Bugs: 
none all test case passed