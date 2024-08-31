#include "reassembler.hh"
#include <algorithm>
#include <ranges>
using namespace std;

void Reassembler::insert( uint64_t first_index, string data, bool is_last_substring )
{
  // Your code here.
//   (void)first_index;
//   (void)data;
//   (void)is_last_substring;
  //插入data
  if(data.empty()){
    if(is_last_substring) // 所有的数据全都接收完成，此后就不需要发送端发送数据了
        output_.writer().close();
    return;
  }
  uint64_t end_index = first_index + data.size(); // data: [first_index,end_index)
  uint64_t last_index = next_index_ + output_.writer().available_capacity(); // 可用的范围[next_index_,last_index)
  // 如果data的范围不在[next_index_,last_index)，不添加,直接返回
  if(end_index <= next_index_ || first_index >= last_index){
    return;
  }
  // 如果data的范围与[next_index_,last_index)有重叠，则修剪
  if(end_index > last_index){
    data.resize(last_index-first_index);
    end_index = last_index; 
    is_last_substring = false;
  }
  if(first_index < next_index_){
    data = data.substr(next_index_-first_index);
    first_index = next_index_;
  }


  // 把data放到buffer中
  buffer_push(first_index,end_index-1,data);
  had_last_ |= is_last_substring; //查看是否到结尾
  //推送buffer
  buffer_pop();

}

uint64_t Reassembler::bytes_pending() const
{
  // Your code here.
  return buffer_size_;
}

void Reassembler::push_to_output(std::string data)
{
  next_index_+=data.size();
  output_.writer().push(move(data));
}
void Reassembler::buffer_push(uint64_t first_index,uint64_t end_index,std::string data)
{
  //leetcode 59
  // data[first_index,end_index]插入到buffer中

  auto l = first_index, r = end_index;
  auto beg = buffer_.begin();
  auto end = buffer_.end();
  auto lef = lower_bound(beg, end, l, [](auto& a,auto& b){return get<1>(a)<b;});
  auto rig = upper_bound(lef, end, r, [](auto& b,auto& a){return b < get<0>(a);});
  if(lef != end)
    l = min(l,get<0>(*lef));
  if(rig != beg)
    r = max(r,get<1>(*prev(rig)));
  
  //如果data已经被包含,不需要加入
  if(lef != end && get<0>(*lef) == l && get<1>(*lef) == r){
    return;
  }

  //如果data不和任何相交
  buffer_size_ += r - l + 1;
  if(data.size() == r-l+1 && lef==rig){
    buffer_.emplace(rig,l,r,move(data));
    return;
  }

  //如果有相交的，则合并，并且减去buffer_中相交部分的大小
  string s(r-l+1,0);
  auto it = lef;
  while(it!=rig){
    auto& [a,b,c] = *it;
    buffer_size_ -= c.size();
    ranges::copy(c,s.begin()+a-l);
    ++it;
  }
  ranges::copy(data, s.begin() + first_index - l);
  buffer_.emplace( buffer_.erase( lef, rig ), l, r, move( s ) );
}
void Reassembler::buffer_pop()
{
    while(!buffer_.empty() && get<0>(buffer_.front())==next_index_){
        auto& [a,b,c] = buffer_.front(); 
        buffer_size_ -= c.size();
        push_to_output(move(c));
        buffer_.pop_front();
    }
    if(had_last_ && buffer_.empty()){
        output_.writer().close();
    }
}

