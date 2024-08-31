#include "tcp_sender.hh"
#include "tcp_config.hh"
#include <utility>
using namespace std;

uint64_t TCPSender::sequence_numbers_in_flight() const
{
  // Your code here.
  return outstanding_num_ ;
}

uint64_t TCPSender::consecutive_retransmissions() const
{
  // Your code here.
  return retransmissions_num_ ;
}

void TCPSender::push( const TransmitFunction& transmit )
{
  // Your code here.
  //(void)transmit;

  // 当 FIN 已发出、或者 ByteStream 中没有数据、亦或者发出的数据已经填满了滑动窗口，拒绝发出任何报文；
  while(((window_size_ > 0 ? window_size_ : 1) > (nextseqno_ - send_base_)) && !fin_flag_){

    TCPSenderMessage message = make_empty_message();
    if(!syn_flag_){ // 第一次调用
        message.SYN = true;
        syn_flag_ =true;
    }
    
    uint64_t sz = (window_size_ > 0 ? window_size_ : 1) - (nextseqno_ - send_base_);// 可用空间
    uint64_t len = min(TCPConfig::MAX_PAYLOAD_SIZE, sz); // 读取字符串最大不超过 MAX_PAYLOAD_SIZE
    read(input_.reader(), len, message.payload);
    
    //如果input stream 可被读取完
    if(!fin_flag_ && sz > message.sequence_length() && input_.reader().is_finished()){
        message.FIN = true;
        fin_flag_ = true;
    } 

    if(message.sequence_length() == 0){
        break;
    }
    //传输
    transmit(message);
    //启动定时器
    if(!timer_.is_running()){
        timer_.load_rto(initial_RTO_ms_);
        timer_.start_new();
    }
    // 更新数据
    nextseqno_ += message.sequence_length();
    outstanding_num_ += message.sequence_length();

    buffer_.emplace(move(message));
  }

}

TCPSenderMessage TCPSender::make_empty_message() const
{
  // Your code here.
  TCPSenderMessage sendmessage;
  sendmessage.seqno = Wrap32::wrap(nextseqno_, isn_);
  sendmessage.RST = input_.has_error();
  return sendmessage;
}

void TCPSender::receive( const TCPReceiverMessage& msg )
{
  // Your code here.
  //(void)msg;

  //注意采用累计重传
  if(input_.has_error()){
    return;
  }
  if(msg.RST){
    input_.set_error();
    return;
  }

  // 拒绝 ackno 的值是不存在的、或者是超过了当前已发出的最后一个字节序号的值的报文；
  window_size_ = msg.window_size;
  
  if(!msg.ackno.has_value()){
    return;
  }
  uint64_t rcv_base = msg.ackno.value().unwrap(isn_, nextseqno_);
  if(rcv_base > nextseqno_){
    return;
  }

  bool is_ack = false;
  while(!buffer_.empty()){
    auto message = buffer_.front();    
    // 报文中的 ackno 不大于缓冲区队首的首字节序号 seqno + payload.size() 的值时
    if(send_base_ + message.sequence_length() > rcv_base){
        break;
    }
    is_ack = true;
    send_base_ += message.sequence_length();
    outstanding_num_ -= message.sequence_length();
    buffer_.pop();
  }

  if(is_ack){
    retransmissions_num_ = 0;
    timer_.load_rto(initial_RTO_ms_);
    buffer_.empty() ? timer_.stop_retrans() : timer_.start_new();
  }

}

void TCPSender::tick( uint64_t ms_since_last_tick, const TransmitFunction& transmit )
{
  // Your code here.
//   (void)ms_since_last_tick;
//   (void)transmit;
//   (void)initial_RTO_ms_;

  timer_.tick_to_retrans_timer(ms_since_last_tick);
  if(timer_.is_expired()){

    if(buffer_.empty()){
        return;
    }
    transmit(buffer_.front());
    if(window_size_ != 0){
        retransmissions_num_++;
        timer_.backoff();
    }
    timer_.start_new();
  }
}
