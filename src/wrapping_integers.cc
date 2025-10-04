#include "wrapping_integers.hh"
#include "debug.hh"

using namespace std;

Wrap32 Wrap32::wrap(uint64_t n, Wrap32 zero_point) {
    // Convert absolute sequence number n to  32-bit wrapped sequence number
    uint32_t wrapped_value = static_cast<uint32_t>(n + zero_point.raw_value_);
    return Wrap32{wrapped_value};
}

uint64_t Wrap32::unwrap(Wrap32 zero_point, uint64_t checkpoint) const {
    // Step 1: compute offset from ISN modulo 2^32
    uint64_t offset = static_cast<uint32_t>(raw_value_ - zero_point.raw_value_);

    // Step 2: find base near checkpoint (round checkpoint down to nearest multiple of 2^32)
    uint64_t base = checkpoint & ~0xFFFFFFFFULL;
    uint64_t candidate = base + offset;

    // Step 3: adjust candidate if too far from checkpoint
    if (candidate + (1ULL << 31) < checkpoint) {
        candidate += (1ULL << 32);  // too far behind → move forward
    } else if (candidate > checkpoint + (1ULL << 31)) {
        candidate -= (1ULL << 32);  // too far ahead → move back
    }

    return candidate;
}
