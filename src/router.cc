#include "router.hh"

#include <iostream>
#include <limits>

using namespace std;

// route_prefix: The "up-to-32-bit" IPv4 address prefix to match the datagram's destination address against
// prefix_length: For this route to be applicable, how many high-order (most-significant) bits of
//    the route_prefix will need to match the corresponding bits of the datagram's destination address?
// next_hop: The IP address of the next hop. Will be empty if the network is directly attached to the router (in
//    which case, the next hop address should be the datagram's final destination).
// interface_num: The index of the interface to send the datagram out on.
void Router::add_route( const uint32_t route_prefix,
                        const uint8_t prefix_length,
                        const optional<Address> next_hop,
                        const size_t interface_num )
{
  cerr << "DEBUG: adding route " << Address::from_ipv4_numeric( route_prefix ).ip() << "/"
       << static_cast<int>( prefix_length ) << " => " << ( next_hop.has_value() ? next_hop->ip() : "(direct)" )
       << " on interface " << interface_num << "\n";

  // Your code here.
    entry_table_.emplace_back(route_prefix, prefix_length, next_hop, interface_num);

}

// Go through all the interfaces, and route every incoming datagram to its proper outgoing interface.
void Router::route()
{
  // Your code here.
    for(auto& interface : _interfaces){ // 遍历所有的接口，然后发送该接口接受的数据包
        auto& datagrams_received = (*interface).datagrams_received();
        while(!datagrams_received.empty()){ // 发送数据包
            InternetDatagram& datagrm = datagrams_received.front(); //当前数据报
            uint32_t dst_ip_addr = datagrm.header.dst;
            auto max_it = entry_table_.end();
            for(auto it = entry_table_.begin();it != entry_table_.end();++it){ //遍历路由表
                // 注意不能左移32位，所以要单独拿出来
                if(it->prefix_length_ == 0 || ((it->route_prefix_ ^ dst_ip_addr) >> (32 - it->prefix_length_))==0){ //如果匹配成功
                    if(max_it == entry_table_.end() || max_it->prefix_length_ < it->prefix_length_){
                        max_it = it;
                    }
                }
            }
            if(max_it != entry_table_.end() && datagrm.header.ttl-- > 1){
                // 发送
                datagrm.header.compute_checksum();
                std::optional<Address> next_hop = max_it->next_hop_;
                auto& send_interface = _interfaces[max_it->interface_num_];
                if(next_hop.has_value()){
                    (*send_interface).send_datagram(datagrm, next_hop.value());
                }else{
                    (*send_interface).send_datagram(datagrm, Address::from_ipv4_numeric(dst_ip_addr));
                }
            }
            datagrams_received.pop();
        }

    }
}
