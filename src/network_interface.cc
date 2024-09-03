#include <iostream>

#include "arp_message.hh"
#include "exception.hh"
#include "network_interface.hh"

using namespace std;

//! \param[in] ethernet_address Ethernet (what ARP calls "hardware") address of the interface
//! \param[in] ip_address IP (what ARP calls "protocol") address of the interface
NetworkInterface::NetworkInterface( string_view name,
                                    shared_ptr<OutputPort> port,
                                    const EthernetAddress& ethernet_address,
                                    const Address& ip_address )
  : name_( name )
  , port_( notnull( "OutputPort", move( port ) ) )
  , ethernet_address_( ethernet_address )
  , ip_address_( ip_address )
{
  cerr << "DEBUG: Network interface has Ethernet address " << to_string( ethernet_address ) << " and IP address "
       << ip_address.ip() << "\n";
}

//! \param[in] dgram the IPv4 datagram to be sent
//! \param[in] next_hop the IP address of the interface to send it to (typically a router or default gateway, but
//! may also be another host if directly connected to the same network as the destination) Note: the Address type
//! can be converted to a uint32_t (raw 32-bit IP address) by using the Address::ipv4_numeric() method.
void NetworkInterface::send_datagram( const InternetDatagram& dgram, const Address& next_hop )
{
  // Your code here.
    const uint32_t next_hop_id = next_hop.ipv4_numeric();
    auto it = _ip_to_ethernet.find(next_hop_id);
    if(it == _ip_to_ethernet.end()){
        //如果之前也没有请求,则重新发送，超时的在tick中处理
        if(_waiting_arp_respons_ip_addr.find(next_hop_id)==_waiting_arp_respons_ip_addr.end()){
            ARPMessage arp;
            arp.opcode = ARPMessage::OPCODE_REQUEST;
            arp.sender_ethernet_address = ethernet_address_;
            arp.sender_ip_address = ip_address_.ipv4_numeric();
            arp.target_ethernet_address = {};
            arp.target_ip_address = next_hop_id;

            EthernetFrame eth_frame = make_frame(
                ethernet_address_,
                ETHERNET_BROADCAST,
                EthernetHeader::TYPE_ARP,
                serialize(arp)
            );
            transmit(eth_frame);

            _waiting_arp_respons_ip_addr[next_hop_id] = _arp_response_default_ttl;
        }
        //ip包加入等待队列
        _waiting_arp_internet_datagrams.push_back({next_hop,dgram});
    }else{
        transmit(make_frame(
                ethernet_address_,
                it->second.first,
                EthernetHeader::TYPE_IPv4,
                serialize(dgram))
            );
    }
}

//! \param[in] frame the incoming Ethernet frame
void NetworkInterface::recv_frame( const EthernetFrame& frame )
{
  // Your code here.
    if(frame.header.dst != ethernet_address_ && frame.header.dst != ETHERNET_BROADCAST){
        return;
    }
    // 如果是数据包
    if(frame.header.type == EthernetHeader::TYPE_IPv4){
        InternetDatagram daragram;
        if(!parse(daragram, frame.payload)){
            return;
        }
        datagrams_received_.emplace( move(daragram) );
    }else if(frame.header.type == EthernetHeader::TYPE_ARP){
        // 如果是arp，可能是请求，也可能是响应
        ARPMessage arp_message;
        if(!parse(arp_message, frame.payload)){
            return;
        }
        EthernetAddress sender_ethernet_address = arp_message.sender_ethernet_address;
        uint32_t sender_ip_address = arp_message.sender_ip_address;

        EthernetAddress target_ethernet_address = arp_message.target_ethernet_address;
        uint32_t target_ip_address = arp_message.target_ip_address;

        bool is_valid_arp_request = (arp_message.opcode == ARPMessage::OPCODE_REQUEST) && (target_ip_address == ip_address_.ipv4_numeric());
        bool is_valid_aro_reply = (arp_message.opcode == ARPMessage::OPCODE_REPLY) && (target_ethernet_address == ethernet_address_);
        if(is_valid_arp_request){ //如果是请求，发送arp
            ARPMessage arp_reply;
            arp_reply.opcode = ARPMessage::OPCODE_REPLY;
            arp_reply.sender_ethernet_address = ethernet_address_;
            arp_reply.sender_ip_address = ip_address_.ipv4_numeric();
            arp_reply.target_ethernet_address = sender_ethernet_address;
            arp_reply.target_ip_address = sender_ip_address;

            EthernetFrame eth_frame = make_frame(
                ethernet_address_,
                sender_ethernet_address,
                EthernetHeader::TYPE_ARP,
                serialize(arp_reply)
            );
            transmit(eth_frame);
        }
        if(is_valid_arp_request || is_valid_aro_reply){
            _ip_to_ethernet[sender_ip_address] = make_pair(sender_ethernet_address,_arp_entry_default_ttl);
            for(auto iter =_waiting_arp_internet_datagrams.begin();iter != _waiting_arp_internet_datagrams.end();){
                if(iter->first.ipv4_numeric() == sender_ip_address){
                    send_datagram(iter->second,iter->first);
                    iter = _waiting_arp_internet_datagrams.erase(iter);
                }else{
                    ++iter;
                }
            }
            _waiting_arp_respons_ip_addr.erase(sender_ip_address);
        }
    }
}

//! \param[in] ms_since_last_tick the number of milliseconds since the last call to this method
void NetworkInterface::tick( const size_t ms_since_last_tick )
{
  // Your code here.
  //删除超出30s的
  for(auto it = _ip_to_ethernet.begin();it != _ip_to_ethernet.end();){
    if(it->second.second <= ms_since_last_tick){
        it = _ip_to_ethernet.erase(it);
    }else{
        it->second.second -= ms_since_last_tick;
        ++it;
    }
  }
  for(auto it = _waiting_arp_respons_ip_addr.begin();it != _waiting_arp_respons_ip_addr.end();){
    if(it->second <= ms_since_last_tick){
            ARPMessage arp_request;
            arp_request.opcode = ARPMessage::OPCODE_REQUEST;
            arp_request.sender_ethernet_address = ethernet_address_;
            arp_request.sender_ip_address = ip_address_.ipv4_numeric();
            arp_request.target_ethernet_address = {};
            arp_request.target_ip_address = it->first;

            EthernetFrame eth_frame = make_frame(
                ethernet_address_,
                ETHERNET_BROADCAST,
                EthernetHeader::TYPE_ARP,
                serialize(arp_request)
            );
            transmit(eth_frame);
            it->second = _arp_response_default_ttl;
    }else{
        it->second -= ms_since_last_tick;
        ++it;
    }
  }
}
EthernetFrame NetworkInterface::make_frame( const EthernetAddress& src,
                          const EthernetAddress& dst,
                          const uint16_t type,
                          vector<string> payload )
{
  EthernetFrame frame;
  frame.header.src = src;
  frame.header.dst = dst;
  frame.header.type = type;
  frame.payload = std::move( payload );
  return frame;
}

// ARPMessage NetworkInterface::make_arp( const uint16_t opcode,
//                      EthernetAddress sender_ethernet_address,
//                      uint32_t& sender_ip_address,
//                      EthernetAddress target_ethernet_address,
//                      uint32_t& target_ip_address )
// {
//   ARPMessage arp;
//   arp.opcode = opcode;
//   arp.sender_ethernet_address = sender_ethernet_address;
//   arp.sender_ip_address = sender_ip_address;
//   arp.target_ethernet_address = target_ethernet_address;
//   arp.target_ip_address = target_ip_address;
//   return arp;
// }