My name: Ammar Bin Mudassar

My SUNet ID: 23L-0630

I collaborated with: Hamza Akmal

I would like to thank/reward these classmates for their help: 23L-0513

This lab took me about 4 hours to do. I did attend the lab session.

Describe Wrap32 and TCPReceiver structure and design:
Wrap32 handles wrapping/unwrapping of 32-bit sequence numbers into absolute 64-bit indices. TCPReceiver tracks ISN, processes incoming messages, reassembles payloads using the Reassembler, and generates acknowledgments. Used a straightforward approach with checkpoints for unwrap and stream index calculation.

Implementation Challenges:
[Handling off-by-one issues with SYN/FIN and unwrap logic.]

Remaining Bugs:
[None observed so far.]

Optional: I was surprised by: [How easily small mistakes in index calculation break tests.]

Optional: I'm not sure about: [RST handling in corner cases.]