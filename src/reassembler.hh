#pragma once

#include "byte_stream.hh"
#include <cstdint>
#include <map>
#include <string>

using namespace std;

class Reassembler
{
public:
  // Construct Reassembler to write into given ByteStream.
  explicit Reassembler( ByteStream&& output );

  // Insert a new substring to be reassembled into a ByteStream.
  void insert( uint64_t first_index, string data, bool is_last_substring );

  // How many bytes are stored in the Reassembler itself?
  uint64_t count_bytes_pending() const;

  // Access output stream reader
  Reader& reader() { return output_.reader(); }
  const Reader& reader() const { return output_.reader(); }

  // Access output stream writer, but const-only (can't write from outside)
  Writer& writer() { return output_.writer(); } 
  const Writer& writer() const { return output_.writer(); }

private:
  ByteStream output_;

  // Next byte index expected to be written
  uint64_t first_unassembled_index_;

  // EOF tracking: whether we've seen a last-substring marker and where the stream ends
  bool eof_seen_;
  uint64_t eof_index_; // absolute index of first byte *after* the last byte (i.e. end index)

  // Buffer for unassembled substrings (start index -> data)
  map<uint64_t, string> buffer_;
};