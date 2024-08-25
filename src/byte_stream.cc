#include "byte_stream.hh"

using namespace std;

ByteStream::ByteStream( uint64_t capacity ) : capacity_( capacity ),error_(false),closed_(false),total_push_(0),total_pop_(0){}

bool Writer::is_closed() const
{
    
  // Your code here.
  return closed_;
}

void Writer::push( string data )
{
  // Your code here.
  if(closed_ || error_){
    return;
  }

  uint64_t writeable = available_capacity();
  uint64_t towrite = std::min(writeable,static_cast<uint64_t>(data.size()));


  for(uint64_t i=0;i<towrite;++i){
    buffer_.push_back(data[i]);
  }
  total_push_+=towrite;
}

void Writer::close()
{
  // Your code here.
  closed_ = true;
}

uint64_t Writer::available_capacity() const
{
  // Your code here.
  return capacity_ - buffer_.size();
}

uint64_t Writer::bytes_pushed() const
{
  // Your code here.
  return total_push_;
}

bool Reader::is_finished() const
{
  // Your code here.
  return closed_ && buffer_.empty();
}

uint64_t Reader::bytes_popped() const
{
  // Your code here.
  return total_pop_;
}

string_view Reader::peek() const
{
  // Your code here.
    return { string_view( &buffer_.front(), 1 ) }; 

}

void Reader::pop( uint64_t len )
{
  // Your code here.
  uint64_t topop = std::min(len, buffer_.size());
    for(uint64_t i=0;i<topop;++i){
    buffer_.pop_front();
  }
  total_pop_ += topop;
}

uint64_t Reader::bytes_buffered() const
{
  // Your code here.
  return buffer_.size();
}
