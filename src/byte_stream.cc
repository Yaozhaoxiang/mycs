#include "byte_stream.hh"

using namespace std;

ByteStream::ByteStream( uint64_t capacity ) : capacity_( capacity ){}

bool Writer::is_closed() const
{
  // Your code here.
  return closed_;
}

void Writer::push( string data )
{
  // Your code here.
  uint64_t towrite = std::min(available_capacity(),static_cast<uint64_t>(data.size()));
  if(towrite == 0){
    return;
  }else if(towrite < data.size()){
    data.resize(towrite);
  }

  buffer_data.push( std::move(data) );
  buffer_view.push( {buffer_data.back().c_str(),towrite});

  bytes_pushed_ +=towrite;
}

void Writer::close()
{
  // Your code here.
  closed_ = true;
}

uint64_t Writer::available_capacity() const
{
  // Your code here.
  return capacity_ - reader().bytes_buffered();
}

uint64_t Writer::bytes_pushed() const
{
  // Your code here.
  return bytes_pushed_;
}

bool Reader::is_finished() const
{
  // Your code here.
  return (closed_ && (bytes_buffered() == 0));
}

uint64_t Reader::bytes_popped() const
{
  // Your code here.
  return bytes_popped_;
}

string_view Reader::peek() const
{
  // Your code here.
  if( buffer_view.empty() ){
    return {};
  }
    return buffer_view.front();
}

void Reader::pop( uint64_t len )
{
  // Your code here.
  auto n = min(len, reader().bytes_buffered());
  while(n>0)
  {
    auto sz = buffer_view.front().size();
    if( n < sz ){
        buffer_view.front().remove_prefix(n);
        bytes_popped_ += n;
        return;
    }
    buffer_data.pop();
    buffer_view.pop();
    n-=sz;
    bytes_popped_ += sz;
  }  
}

uint64_t Reader::bytes_buffered() const
{
  // Your code here.
  return writer().bytes_pushed() - bytes_popped();
}