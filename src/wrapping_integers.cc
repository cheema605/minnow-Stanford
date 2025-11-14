#include "wrapping_integers.hh"
#include "debug.hh"

using namespace std;

Wrap32 Wrap32::wrap(uint64_t n, Wrap32 zero_point) {
    // Convert absolute sequence number n to  32-bit wrapped sequence number
    uint32_t wrapped_value = static_cast<uint32_t>(n + zero_point.raw_value_);
    return Wrap32{wrapped_value};
}

uint64_t Wrap32::unwrap(Wrap32 zero_point, uint64_t checkpoint) const {
    // Calculate the offset from zero_point
    uint32_t offset = raw_value_ - zero_point.raw_value_;
    
    // Start with the simple case where we assume the absolute seqno is just the offset
    uint64_t result = offset;
    
    // If checkpoint is large enough, we need to find the multiple of 2^32 that gets us closest
    if (checkpoint > (1ULL << 31)) {
        // Calculate the base (multiple of 2^32) that aligns with checkpoint
        uint64_t base = (checkpoint - offset) & ~0xFFFFFFFFULL;
        result = base + offset;
        
        // Consider the candidate one cycle before and after
        uint64_t candidate1 = result;
        uint64_t candidate2 = result + (1ULL << 32);
        uint64_t candidate0 = result < (1ULL << 32) ? 0 : result - (1ULL << 32);
        
        // Find which candidate is closest to checkpoint
        uint64_t diff0 = candidate0 > checkpoint ? candidate0 - checkpoint : checkpoint - candidate0;
        uint64_t diff1 = candidate1 > checkpoint ? candidate1 - checkpoint : checkpoint - candidate1;
        uint64_t diff2 = candidate2 > checkpoint ? candidate2 - checkpoint : checkpoint - candidate2;
        
        if (diff0 <= diff1 && diff0 <= diff2) {
            result = candidate0;
        } else if (diff2 <= diff1 && diff2 <= diff0) {
            result = candidate2;
        } else {
            result = candidate1;
        }
    }
    
    return result;
}
