#pragma once

#include "byte_stream.hh"
#include "tcp_receiver_message.hh"
#include "tcp_sender_message.hh"

#include <cstdint>
#include <functional>
#include <list>
#include <memory>
#include <optional>
#include <queue>

class RetransTimer{
private:
    uint64_t RTO_ {0};
    uint64_t time_ {0}; //运行时间
    bool is_running_ {false};  //是否运行
public:
    RetransTimer() {}
    RetransTimer(uint64_t rto) : RTO_(rto){}
    bool is_expired() const {return time_ >= RTO_ && is_running_;}
    bool is_running() const {return is_running_;}
    //计时
    void tick_to_retrans_timer(uint64_t ms_since_last_tick){
        if(!is_running_){
            return;
        }
        time_ += ms_since_last_tick;
    }
    // 重启
    void start_new(){
        time_ = 0;
        is_running_ = true;
    }
    //停止
    void stop_retrans(){
        is_running_ = false;
    }
    void backoff(){
        this->RTO_ *= 2;
    }
    void load_rto(uint64_t rto){
        this->RTO_ = rto;
    }
};

class TCPSender
{
public:
  /* Construct TCP sender with given default Retransmission Timeout and possible ISN */
  TCPSender( ByteStream&& input, Wrap32 isn, uint64_t initial_RTO_ms )
    : input_( std::move( input ) ), isn_( isn ), initial_RTO_ms_( initial_RTO_ms )
    , timer_(initial_RTO_ms)

  {}

  /* Generate an empty TCPSenderMessage */
  TCPSenderMessage make_empty_message() const;

  /* Receiv   e and process a TCPReceiverMessage from the peer's receiver */
  void receive( const TCPReceiverMessage& msg );

  /* Type of the `transmit` function that the push and tick methods can use to send messages */
  using TransmitFunction = std::function<void( const TCPSenderMessage& )>;

  /* Push bytes from the outbound stream */
  void push( const TransmitFunction& transmit );

  /* Time has passed by the given # of milliseconds since the last time the tick() method was called */
  void tick( uint64_t ms_since_last_tick, const TransmitFunction& transmit );

  // Accessors
  uint64_t sequence_numbers_in_flight() const;  // 当前有多少序列号未确认？
  uint64_t consecutive_retransmissions() const; //  发生了多少次连续的重传？
  Writer& writer() { return input_.writer(); }
  const Writer& writer() const { return input_.writer(); }

  // Access input stream reader, but const-only (can't read from outside)
  const Reader& reader() const { return input_.reader(); }

private:
  // Variables initialized in constructor
  ByteStream input_;   
  Wrap32 isn_;
  uint64_t initial_RTO_ms_; //重传超时（RTO）的初始值

RetransTimer timer_ {}; // 定时器
std::queue<TCPSenderMessage> buffer_ {}; //窗口，维护 发送还未确认，可用还未发送的 seqno
uint64_t window_size_ {1}; //窗口大小
uint64_t send_base_ {0};  // 最早未被确定的的 seqno，发送还未确认
uint64_t nextseqno_ {0}; //下一个序号，可用还未发送
uint64_t outstanding_num_ {0}; //当前有多少序列号未确认
uint64_t retransmissions_num_ {0}; // 重传次数
bool syn_flag_ {false};
bool fin_flag_ {false};

};
