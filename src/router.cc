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
  routes_.push_back( { route_prefix >> prefix_length, prefix_length, next_hop, interface_num } );
}

// Go through all the interfaces, and route every incoming datagram to its proper outgoing interface.
void Router::route()
{
  // Your code here.
  for ( auto& recv_interface : _interfaces ) {
    while ( !recv_interface->datagrams_received().empty() ) {
      InternetDatagram dgram = recv_interface->datagrams_received().front();
      uint32_t dst = dgram.header.dst;
      auto best_match = routes_.end();
      // choose the best route
      for ( auto it = routes_.begin(); it != routes_.end(); it++ ) {
        if ( it->prefix == ( dst >> it->prefix_length ) || it->prefix_length == 0 ) {
          if ( best_match == routes_.end() || ( best_match->prefix_length < it->prefix_length ) ) {
            best_match = it;
          }
        }
      }

      dgram.header.ttl--;
      dgram.header.compute_checksum();
      if ( best_match != routes_.end() ) {
        if ( best_match->next_hop.has_value() ) {
          _interfaces[best_match->interface_num]->send_datagram( dgram, best_match->next_hop.value() );
        } else {
          _interfaces[best_match->interface_num]->send_datagram( dgram, Address::from_ipv4_numeric( dst ) );
        }
      } else {
        cerr << "DEBUG: no route for " << Address::from_ipv4_numeric( dst ).ip() << "\n";
      }
      recv_interface->datagrams_received().pop();
    }
  }
}
