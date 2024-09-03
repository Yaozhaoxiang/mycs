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
    uint32_t next_hop_ip = next_hop.ipv4_numeric();
    auto it = mapping_table_.find(next_hop_ip);
    if(it == mapping_table_.end()){
        dgrams_waiting_addr_.emplace(next_hop_ip, dgram);
        if(arp_recorder_.find(next_hop_ip)==arp_recorder_.end()){
            ARPMessage  arp_request = make_arp(
                    ARPMessage::OPCODE_REQUEST,
                    ethernet_address_,
                    ip_address_.ipv4_numeric(),
                    {},
                    next_hop_ip
                );
            transmit(make_frame(
                ethernet_address_,
                ETHERNET_BROADCAST,
                EthernetHeader::TYPE_ARP,
                serialize(arp_request)
            ));
            arp_recorder_.emplace(next_hop_ip, 0);
        }
    }else{
        transmit(make_frame(
            ethernet_address_,
            it->second.get_ether(),
            EthernetHeader::TYPE_IPv4,
            serialize(dgram)
            ));
    }
}

//! \param[in] frame the incoming Ethernet frame
void NetworkInterface::recv_frame( const EthernetFrame& frame )
{
    if(frame.header.dst != ethernet_address_ && frame.header.dst != ETHERNET_BROADCAST){
    return;
  }
    switch (frame.header.type)
    {
    case EthernetHeader::TYPE_IPv4 :{
            InternetDatagram datagram;
            if(!parse(datagram, frame.payload)){
                return;
            }
            datagrams_received_.emplace(move(datagram));
        }
        break;
    case EthernetHeader::TYPE_ARP:{
        ARPMessage  arp_message;
        if(!parse(arp_message, frame.payload)){
            return;
        }
        mapping_table_.emplace(arp_message.sender_ip_address, address_mapping(arp_message.sender_ethernet_address));
        switch (arp_message.opcode)
        {
        case ARPMessage::OPCODE_REQUEST:{
            if(arp_message.target_ip_address == ip_address_.ipv4_numeric()){
                ARPMessage  arp_reply = make_arp(
                        ARPMessage::OPCODE_REPLY,
                        ethernet_address_,
                        ip_address_.ipv4_numeric(),
                        arp_message.sender_ethernet_address,
                        arp_message.sender_ip_address
                    );
                transmit(make_frame(
                    ethernet_address_,
                    arp_message.sender_ethernet_address,
                    EthernetHeader::TYPE_ARP,
                    serialize(arp_reply)
                ));
            }
        }
            break;
        case ARPMessage::OPCODE_REPLY:{
            // 把存的数据发送出去
            auto [head, tail] = dgrams_waiting_addr_.equal_range(arp_message.sender_ip_address);
            for_each(head, tail, [this, &arp_message](auto&& iter)->void{
                    transmit(make_frame(
                        ethernet_address_,
                        arp_message.sender_ethernet_address,
                        EthernetHeader::TYPE_IPv4,
                        serialize(iter.second)
                ));
            });
            if(head != tail){
                dgrams_waiting_addr_.erase(head, tail);
            }
        }
        break;
        default:
            break;
        }
    }
    break;
    default:
        break;
    }
}

//! \param[in] ms_since_last_tick the number of milliseconds since the last call to this method
void NetworkInterface::tick( const size_t ms_since_last_tick )
{
    const size_t ms_mappings_ttl = 30'000, ms_resend_arp = 5'000;
    auto flush_timer = [&ms_since_last_tick](auto& datasheet, const size_t deadline)->void{
        for(auto iter = datasheet.begin();iter != datasheet.end();){
            if((iter->second += ms_since_last_tick)>deadline){
                iter = datasheet.erase(iter);
            }else{
                ++iter;
            }
        }
    };
    flush_timer(mapping_table_, ms_mappings_ttl);
    flush_timer(arp_recorder_, ms_resend_arp);

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

ARPMessage NetworkInterface::make_arp( const uint16_t opcode,
                     EthernetAddress sender_ethernet_address,
                     const uint32_t& sender_ip_address,
                     EthernetAddress target_ethernet_address,
                     const uint32_t& target_ip_address )
{
  ARPMessage arp;
  arp.opcode = opcode;
  arp.sender_ethernet_address = sender_ethernet_address;
  arp.sender_ip_address = sender_ip_address;
  arp.target_ethernet_address = target_ethernet_address;
  arp.target_ip_address = target_ip_address;
  return arp;
}