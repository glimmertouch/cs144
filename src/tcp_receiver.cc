#include "tcp_receiver.hh"

using namespace std;

void TCPReceiver::receive( TCPSenderMessage message )
{
  // Your code here.
  uint64_t stream_index_ = 0;
  if ( message.RST ) {
    reader().writer().set_error();
  }
  if ( message.SYN ) {
    syn_received_ = true;
    ISN = message.seqno;
    // SYN with payload
    stream_index_ = message.seqno.unwrap( ISN, reassembler_.abs_seqno() );
  } else {
    // refer to the trans of abs_seqno & stream_index
    stream_index_ = message.seqno.unwrap( ISN, reassembler_.abs_seqno() ) - 1;
  }
  reassembler_.insert( stream_index_, message.payload, message.FIN );
  if ( syn_received_ ) {
    if ( reader().writer().is_closed() ) {
      next_seqno_ = ISN + reassembler_.abs_seqno() + 1;
    } else {
      next_seqno_ = ISN + reassembler_.abs_seqno();
    }
  } else {
    next_seqno_ = {};
  }
}

TCPReceiverMessage TCPReceiver::send() const
{
  // Your code here.
  const uint16_t window_size
    = ( reader().writer().available_capacity() > UINT16_MAX ) ? UINT16_MAX : reader().writer().available_capacity();
  return { next_seqno_, window_size, reader().writer().has_error() };
}
