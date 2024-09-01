#include "wrapping_integers.hh"
#include <cmath>
using namespace std;

Wrap32 Wrap32::wrap( uint64_t n, Wrap32 zero_point )
{
  // Your code here.
//   (void)n;
//   (void)zero_point;
  //absolute seqno -> seqno
  
  return Wrap32 { static_cast<uint32_t>((n + zero_point.raw_value_) % (1ULL << 32))};
}

uint64_t Wrap32::unwrap( Wrap32 zero_point, uint64_t checkpoint ) const
{
  // Your code here.
//   (void)zero_point; 初始化点
//   (void)checkpoint; 检查点
// seqno -> absolute seqno
    uint64_t base = checkpoint & 0xFFFFFFFF00000000ULL;  // 获取 checkpoint 的高 32 位
    uint64_t value = raw_value_ - zero_point.raw_value_;
    uint64_t wrapped_value = base | value;

    // 如果 wrapped_value 比 checkpoint 小，并且它加上 2^32 比 checkpoint 更接近
    if (wrapped_value < checkpoint && checkpoint - wrapped_value > (1ULL << 31)) {
        return wrapped_value + (1ULL << 32);
    }

    // 如果 wrapped_value 比 checkpoint 大，并且它减去 2^32 比 checkpoint 更接近
    if(wrapped_value > checkpoint && wrapped_value > UINT32_MAX &&  wrapped_value - checkpoint > (1ULL << 31)){
        return wrapped_value - (1ULL << 32);
    }

    return wrapped_value;

}
