#include "tcp_receiver.hh"

using namespace std;

void TCPReceiver::receive( TCPSenderMessage message )
{
  // Your code here.
  //(void)message;
  // 将任何数据推送到 Reassembler
  uint64_t first_index; //当前接收的字符串索引，后面传入 reassembler
  bool is_last_substring = false;

  // 处理 RST
  if(message.RST){
    reassembler_.reader().set_error();
    return;
  }else if(reassembler_.reader().has_error()){
    return;
  }
  // 处理 SYN
  if(message.SYN){
    if(!zero_point.has_value()){
        zero_point = message.seqno;
        message.seqno = message.seqno + 1; //如果 只有一个syn， 那么将传递 ""
    }else { //如果之前已经有 syn了，则还是用原来的哪个,应该不会出现这种情况，因为下一个是基于send发送的
        return;
    }
  }

  if(!zero_point.has_value()){ //如果一直没有 syn 返回
    return;
  }
// 此时，已经有syn了,并且全部指向字符串
  first_index = message.seqno.unwrap(zero_point.value(), reassembler_.writer().bytes_pushed());  //转化为ab seqno
  if(first_index == 0){ //byte with invalid stream index should be ignored，有个测试 先传递 2345 syn,然后 2345 a,这个a不合法
    return;
  }else{
    first_index--; //转化为 stream index
  }

  is_last_substring = message.FIN;
  reassembler_.insert(first_index, move(message.payload), is_last_substring);

  ackon_ = zero_point.value() + zero_point.has_value()+ reassembler_.writer().bytes_pushed() + reassembler_.writer().is_closed();

}

TCPReceiverMessage TCPReceiver::send() const
{
  // Your code here.
  TCPReceiverMessage ReceiverMessage;
  
  ReceiverMessage.RST = reassembler_.reader().has_error();
  if(zero_point.has_value()){
    ReceiverMessage.ackno = ackon_;
  }

  
  if ( reassembler_.writer().available_capacity() > UINT16_MAX ) {
    ReceiverMessage.window_size = UINT16_MAX;
  } else {
    ReceiverMessage.window_size = reassembler_.writer().available_capacity();
  }

  return ReceiverMessage;
}
