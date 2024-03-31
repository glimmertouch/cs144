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

class TCPSender
{
public:
  /* Construct TCP sender with given default Retransmission Timeout and possible ISN */
  TCPSender( ByteStream&& input, Wrap32 isn, uint64_t initial_RTO_ms )
    : input_( std::move( input ) )
    , isn_( isn )
    , abs_isn_( isn.unwrap( isn, 0 ) )
    , next_seqno_( isn )
    , initial_RTO_ms_( initial_RTO_ms )
    , RTO_ms_( initial_RTO_ms )
  {}

  /* Generate an empty TCPSenderMessage */
  TCPSenderMessage make_empty_message() const;

  /* Receive and process a TCPReceiverMessage from the peer's receiver */
  void receive( const TCPReceiverMessage& msg );

  /* Type of the `transmit` function that the push and tick methods can use to send messages */
  using TransmitFunction = std::function<void( const TCPSenderMessage& )>;

  /* Push bytes from the outbound stream */
  void push( const TransmitFunction& transmit );

  /* Time has passed by the given # of milliseconds since the last time the tick() method was called */
  void tick( uint64_t ms_since_last_tick, const TransmitFunction& transmit );

  // Accessors
  uint64_t sequence_numbers_in_flight() const;  // How many sequence numbers are outstanding?
  uint64_t consecutive_retransmissions() const; // How many consecutive *re*transmissions have happened?
  Writer& writer() { return input_.writer(); }
  const Writer& writer() const { return input_.writer(); }

  // Access input stream reader, but const-only (can't read from outside)
  const Reader& reader() const { return input_.reader(); }

private:
  // Variables initialized in constructor
  ByteStream input_;
  Wrap32 isn_;
  uint64_t abs_isn_;
  Wrap32 next_seqno_;
  uint64_t initial_RTO_ms_;
  uint64_t RTO_ms_;
  // avoid repeated SYNs, FINs and RSTs
  bool syn_has_sent_ = false;
  bool syn_received_ = false;
  bool fin_has_sent_ = false;
  bool rst_has_sent_ = false;
  uint64_t sequence_numbers_in_flight_ = 0;
  size_t window_size_ = 1452;
  uint64_t ms_since_last_transmission_ = 0;
  uint64_t msg_abs_seqno_last_ = 0; // closest ackno received
  uint8_t consecutive_retransmissions_ = 0;
  std::queue<TCPSenderMessage> window_ {};
  // use for receiver win = 0
  bool receiver_reply_ = false;
  uint8_t receiver_busy_ = 0;
};
