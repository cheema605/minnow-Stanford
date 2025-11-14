#include "reassembler.hh"
#include <algorithm>
#include <iostream>
#include <vector>

using namespace std;

Reassembler::Reassembler( ByteStream&& output )
  : output_( move( output ) ), first_unassembled_index_( 0 ), eof_seen_( false ), eof_index_( 0 ), buffer_()
{
  cout << "[Reassembler starting up]" << endl;
}

void Reassembler::insert( uint64_t first_index, string data, bool is_last_substring )
{
  // original start and end of this incoming chunk
  uint64_t orig_first = first_index;
  uint64_t orig_end = first_index + data.size();

  // if this is the last substring, remember where the stream ends
  if ( is_last_substring ) {
    eof_seen_ = true;
    eof_index_ = orig_end;
  }

  // ignore empty input unless it’s the actual EOF marker
  if ( data.empty() && !is_last_substring ) {
    cout << "insert() called with empty data... (lol useless)" << endl;
    return;
  }

  // writable window: what we can actually fit into the output rn
  uint64_t window_start = first_unassembled_index_;
  uint64_t capacity = output_.writer().available_capacity();
  uint64_t window_end = window_start + capacity;

  // if the chunk ends before the stuff we already assembled, discard
  if ( orig_end <= window_start ) {
    if ( eof_seen_ && first_unassembled_index_ >= eof_index_ ) {
      cout << "Stream completed already. gg ez" << endl;
      output_.writer().close();
    }
    return;
  }

  // trim chunk so it fits in window
  uint64_t new_first = max<uint64_t>( orig_first, window_start );
  if ( new_first >= window_end ) {
    cout << "This chunk is too far ahead bruh, can’t handle it yet" << endl;
    return;
  }
  uint64_t new_end = min<uint64_t>( orig_end, window_end );
  size_t offset = static_cast<size_t>( new_first - orig_first );
  size_t keep_len = static_cast<size_t>( new_end - new_first );
  string piece = data.substr( offset, keep_len );

  if ( piece.empty() )
    return;

  // merging overlapping chunks (this is the fun messy part)
  uint64_t merged_first = new_first;
  uint64_t merged_end = new_end;

  auto it = buffer_.lower_bound( new_first );
  if ( it != buffer_.begin() )
    --it; // go one step back to catch neighbors

  vector<pair<uint64_t, string>> overlapping;
  while ( it != buffer_.end() ) {
    uint64_t seg_first = it->first;
    uint64_t seg_end = seg_first + it->second.size();

    if ( seg_end < merged_first ) {
      ++it;
      continue;
    } // segment ends before us
    if ( seg_first > merged_end )
      break; // segment starts after us

    overlapping.emplace_back( seg_first, it->second ); // ok they overlap
    merged_first = min( merged_first, seg_first );
    merged_end = max( merged_end, seg_end );

    ++it;
  }

  // if we found overlaps, do another sweep in case merging grew
  if ( !overlapping.empty() ) {
    overlapping.clear();
    it = buffer_.lower_bound( merged_first );
    if ( it != buffer_.begin() )
      --it;
    while ( it != buffer_.end() ) {
      uint64_t seg_first = it->first;
      uint64_t seg_end = seg_first + it->second.size();
      if ( seg_end < merged_first ) {
        ++it;
        continue;
      }
      if ( seg_first > merged_end )
        break;
      overlapping.emplace_back( seg_first, it->second );
      merged_first = min( merged_first, seg_first );
      merged_end = max( merged_end, seg_end );
      ++it;
    }
  }

  // big combined buffer for everything in [merged_first, merged_end)
  size_t merged_size = static_cast<size_t>( merged_end - merged_first );
  vector<char> merged_bytes( merged_size, 0 ); // 0 means “empty slot”
  vector<char> present( merged_size, 0 );      // tracks filled slots

  // put the new data inside the merged buffer
  for ( size_t i = 0; i < piece.size(); ++i ) {
    size_t idx = static_cast<size_t>( new_first - merged_first ) + i;
    merged_bytes[idx] = piece[i];
    present[idx] = 1;
  }

  // copy data from overlapping old chunks into merged buffer
  for ( auto& pr : overlapping ) {
    uint64_t seg_first = pr.first;
    string& seg_data = pr.second;
    for ( size_t i = 0; i < seg_data.size(); ++i ) {
      size_t idx = static_cast<size_t>( seg_first - merged_first ) + i;
      merged_bytes[idx] = seg_data[i];
      present[idx] = 1;
    }
  }

  // clear the old overlapping entries from buffer (they’re merged now)
  for ( auto& pr : overlapping )
    buffer_.erase( pr.first );

  // cut the merged range back into continuous filled segments
  size_t pos = 0;
  while ( pos < merged_size ) {
    if ( !present[pos] ) {
      ++pos;
      continue;
    }
    size_t start = pos;
    while ( pos < merged_size && present[pos] )
      ++pos;
    size_t len = pos - start;
    string out;
    out.reserve( len );
    for ( size_t k = 0; k < len; ++k )
      out.push_back( merged_bytes[start + k] );
    uint64_t store_index = merged_first + start;
    buffer_[store_index] = move( out );
    cout << "Stored chunk at " << store_index << " size " << buffer_[store_index].size() << " (yes chef)" << endl;
  }

  // push out any data that starts exactly where we left off
  while ( true ) {
    auto itf = buffer_.find( first_unassembled_index_ );
    if ( itf == buffer_.end() )
      break;
    string& s = itf->second;
    output_.writer().push( s );
    first_unassembled_index_ += s.size();
    buffer_.erase( itf );
  }

  // close stream if we’re fully done
  if ( eof_seen_ && first_unassembled_index_ >= eof_index_ ) {
    cout << "Stream completed 🎉 Mission accomplished, roll credits" << endl;
    output_.writer().close();
  }
}

uint64_t Reassembler::count_bytes_pending() const
{
  uint64_t total = 0;
  for ( const auto& kv : buffer_ )
    total += kv.second.size();
  return total;
}