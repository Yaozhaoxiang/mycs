#pragma once

#include "byte_stream.hh"
#include <list>
#include <tuple>
using namespace std;
class Reassembler
{
public:
  // Construct Reassembler to write into given ByteStream.
  explicit Reassembler( ByteStream&& output ) : output_( std::move( output ) ) {}

  /*
   * Insert a new substring to be reassembled into a ByteStream.
   *   `first_index`: the index of the first byte of the substring
   *   `data`: the substring itself
   *   `is_last_substring`: this substring represents the end of the stream
   *   `output`: a mutable reference to the Writer
   *
   * The Reassembler's job is to reassemble the indexed substrings (possibly out-of-order
   * and possibly overlapping) back into the original ByteStream. As soon as the Reassembler
   * learns the next byte in the stream, it should write it to the output.
   * Reassembler的工作是重新组装索引的子字符串(可能是乱序的)和可能重叠)回到原来的字节流。一旦重组器
   * 学习流中的下一个字节，它应该将其写入输出。
   *
   * If the Reassembler learns about bytes that fit within the stream's available capacity
   * but can't yet be written (because earlier bytes remain unknown), it should store them
   * internally until the gaps are filled in.
   *
   * 如果Reassembler了解到在流的可用容量内适合的字节但还不能写入(因为之前的字节仍然是未知的)，它应该存储它们内部，直到空白被填补。
   *
   * The Reassembler should discard any bytes that lie beyond the stream's available capacity
   * (i.e., bytes that couldn't be written even if earlier gaps get filled in).
   *
   * Reassembler应该丢弃任何超出流可用容量的字节(即使早先的空白被填满也无法写入的字节)。
   * 
   * The Reassembler should close the stream after writing the last byte.
   */
  void push_to_output(std::string data);
  void buffer_push(uint64_t first_index,uint64_t end_index,std::string data);
  void buffer_pop();

  void insert( uint64_t first_index, std::string data, bool is_last_substring );

  // How many bytes are stored in the Reassembler itself?
  uint64_t bytes_pending() const;

  // Access output stream reader
  Reader& reader() { return output_.reader(); }
  const Reader& reader() const { return output_.reader(); }

  // Access output stream writer, but const-only (can't write from outside)
  const Writer& writer() const { return output_.writer(); }

private:
  bool had_last_ {}; //最后一个位置
  uint64_t buffer_size_ { 0 }; //buffer中的字节
  uint64_t next_index_ { 0 }; //下一个要读的下标
  list<std::tuple<uint64_t,uint64_t,std::string>>buffer_ {}; //存储[first,end]字符串，之间没有交集，start递增；但是first!=next_index_
  ByteStream output_; // the Reassembler writes to this ByteStream
};
