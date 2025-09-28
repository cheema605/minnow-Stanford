#include "byte_stream.hh"

using namespace std;

ByteStream::ByteStream( uint64_t capacity ) : capacity_( capacity ),
    error_(false),
    buffer_(),
    pushed_(0),
    popped_(0),
    closed_(false)
{}



void Writer::push(std::string data)
{
  size_t n = std::min<uint64_t>(data.size(), available_capacity());
  for (size_t i = 0; i < n; i++) {
    buffer_.push_back(data[i]);
  }
  pushed_ += n;
}

void Writer::close()
{
  closed_ = true;
}

bool Writer::is_closed() const
{
  return closed_;
}

uint64_t Writer::available_capacity() const
{
  return capacity_ - buffer_.size();
}

uint64_t Writer::bytes_pushed() const
{
  return pushed_; // Your code here.
}

string_view Reader::peek() const
{
  if (buffer_.empty()) return {};
  return string_view(buffer_.data(), buffer_.size());
}


void Reader::pop(uint64_t len)
{
  size_t n = min<uint64_t>(len, buffer_.size());
  buffer_.erase(0, n);
  popped_ += n;
}

bool Reader::is_finished() const
{
  return closed_ && buffer_.empty();
}


uint64_t Reader::bytes_buffered() const
{
  return buffer_.size();
}


uint64_t Reader::bytes_popped() const
{
  return popped_;
}
