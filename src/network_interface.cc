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
  EthernetFrame frame;
  uint32_t next_hop_ipv4_numeric_ = next_hop.ipv4_numeric();
  auto it = arp_table_.find( next_hop_ipv4_numeric_ );
  if ( it == arp_table_.end() ) {
    if ( arp_requests_.find( next_hop_ipv4_numeric_ ) != arp_requests_.end() ) {
      return;
    }
    datagrams_to_send_.emplace_back( make_pair( dgram, next_hop ) );
    ARPMessage arp = ARPMessage( ethernet_address_, ip_address_.ipv4_numeric(), {}, next_hop_ipv4_numeric_ );
    arp.opcode = ARPMessage::OPCODE_REQUEST;
    frame = EthernetFrame( EthernetHeader( ETHERNET_BROADCAST, ethernet_address_, EthernetHeader::TYPE_ARP ),
                           serialize( arp ) );
    arp_requests_[next_hop_ipv4_numeric_] = 0;
  } else {
    frame = EthernetFrame( EthernetHeader( it->second.first, ethernet_address_, EthernetHeader::TYPE_IPv4 ),
                           serialize( dgram ) );
  }
  transmit( frame );
}

//! \param[in] frame the incoming Ethernet frame
void NetworkInterface::recv_frame( const EthernetFrame& frame )
{
  // Your code here.
  if ( frame.header.dst == ethernet_address_ || frame.header.dst == ETHERNET_BROADCAST ) {
    if ( frame.header.type == EthernetHeader::TYPE_ARP ) {
      ARPMessage arp;
      if ( parse( arp, frame.payload ) ) {
        if ( arp.opcode == ARPMessage::OPCODE_REQUEST && arp.target_ip_address == ip_address_.ipv4_numeric() ) {
          ARPMessage reply = ARPMessage(
            ethernet_address_, ip_address_.ipv4_numeric(), arp.sender_ethernet_address, arp.sender_ip_address );
          reply.opcode = ARPMessage::OPCODE_REPLY;
          EthernetFrame reply_frame = EthernetFrame(
            EthernetHeader( arp.sender_ethernet_address, ethernet_address_, EthernetHeader::TYPE_ARP ),
            serialize( reply ) );
          transmit( reply_frame );
        }
        arp_table_[arp.sender_ip_address] = make_pair( arp.sender_ethernet_address, 0 );
        for ( auto it = datagrams_to_send_.begin(); it != datagrams_to_send_.end(); ) {
          InternetDatagram& dgram = it->first;
          Address& next_hop = it->second;
          EthernetAddress& next_hop_ethernet_address = arp_table_[next_hop.ipv4_numeric()].first;
          if ( arp_table_.find( next_hop.ipv4_numeric() ) != arp_table_.end() ) {
            EthernetFrame send_frame = EthernetFrame(
              EthernetHeader( next_hop_ethernet_address, ethernet_address_, EthernetHeader::TYPE_IPv4 ),
              serialize( dgram ) );
            transmit( send_frame );
            it = datagrams_to_send_.erase( it );
          } else {
            it++;
          }
        }
      }

    } else if ( frame.header.type == EthernetHeader::TYPE_IPv4 ) {
      IPv4Datagram dgram;
      if ( parse( dgram, frame.payload ) ) {
        datagrams_received_.push( dgram );
      }
    }
  }
}

//! \param[in] ms_since_last_tick the number of milliseconds since the last call to this method
void NetworkInterface::tick( const size_t ms_since_last_tick )
{
  // Your code here.
  for ( auto it = arp_requests_.begin(); it != arp_requests_.end(); ) {
    if ( it->second + ms_since_last_tick >= 5000 ) {
      it = arp_requests_.erase( it );
    } else {
      it->second += ms_since_last_tick;
      it++;
    }
  }

  for ( auto it = arp_table_.begin(); it != arp_table_.end(); ) {
    if ( it->second.second + ms_since_last_tick >= 30000 ) {
      it = arp_table_.erase( it );
    } else {
      it->second.second += ms_since_last_tick;
      it++;
    }
  }
}
