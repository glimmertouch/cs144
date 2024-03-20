#include "byte_stream.hh"

using namespace std;

ByteStream::ByteStream( uint64_t capacity ) : capacity_( capacity ) {}

bool Writer::is_closed() const
{
  // Your code here.
  return closed_;
}

void Writer::push( string data )
{
  // Your code here.
  if ( closed_ ) {
    return;
  }
  auto len = min( available_capacity(), data.size() );
  // throw empty string || when capacity is 0
  if ( len == 0 ) {
    return;
  } else if ( len < data.size() ) {
    data.resize( len );
  }
  buffer_.push( move( data ) );
  bytes_pushed_ += len;
  if ( buffer_.size() == 1 ) {
    buffer_view_ = buffer_.front();
  }
}

void Writer::close()
{
  // Your code here.
  closed_ = true;
}

uint64_t Writer::available_capacity() const
{
  // Your code here.
  return capacity_ - bytes_pushed_ + bytes_popped_;
}

uint64_t Writer::bytes_pushed() const
{
  // Your code here.
  return bytes_pushed_;
}

bool Reader::is_finished() const
{
  // Your code here.
  return closed_ && bytes_buffered() == 0;
}

uint64_t Reader::bytes_popped() const
{
  // Your code here.
  return bytes_popped_;
}

string_view Reader::peek() const
{
  // Your code here.
  return buffer_view_;
}

void Reader::pop( uint64_t len )
{
  // Your code here.
  if ( len > bytes_buffered() ) {
    printf( "Reader::pop() called with more bytes than are buffered" );
    return;
  }
  bytes_popped_ += len;
  while ( len ) {
    if ( len >= buffer_view_.size() ) {
      len -= buffer_view_.size();
      buffer_.pop();
      buffer_view_ = buffer_.empty() ? ""sv : buffer_.front();
    } else {
      buffer_view_.remove_prefix( len );
      len = 0;
    }
  }
}

uint64_t Reader::bytes_buffered() const
{
  // Your code here.
  return bytes_pushed_ - bytes_popped_;
}