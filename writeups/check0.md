Checkpoint 0 Writeup
====================

My name: Ammar Bin Mudassar

My SUNet ID: 23L-0630

I collaborated with: Hamza Akmal

I would like to credit/thank these classmates for their help: 23L-0513

This lab took me about 4 hours to do. I did attend the lab session.

My secret code from section 2.1 was: N/A

I was surprised by or edified to learn that: How networks work

The ByteStream is implemented as a bounded FIFO buffer using a std::string for contiguous storage, along with counters to track bytes pushed, bytes popped, and a closed flag. This design keeps peek() simple and efficient by returning a std::string_view without copying. Alternative designs such as std::deque or a circular buffer were considered; while they offer faster pops, they complicate peek() or require more bookkeeping. Our approach favors simplicity and correctness over optimal asymptotic performance, which is acceptable for this assignment’s scale.

- Optional: I had unexpected difficulty with: [describe]

- Optional: I think you could make this lab better by: [describe]

- Optional: I'm not sure about: [describe]

- Optional: I contributed a new test case that catches a plausible bug
  not otherwise caught: [provide Pull Request URL]