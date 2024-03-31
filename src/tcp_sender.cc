#include "tcp_sender.hh"
#include "tcp_config.hh"

using namespace std;

uint64_t TCPSender::sequence_numbers_in_flight() const
{
  // Your code here.
  return sequence_numbers_in_flight_;
}

uint64_t TCPSender::consecutive_retransmissions() const
{
  // Your code here.
  return consecutive_retransmissions_;
}

void TCPSender::push( const TransmitFunction& transmit )
{
  // Your code here.;
  /*
  1. If the sender has not received a SYN from the receiver, it should send a SYN.
  2. If the sender has received a SYN from the receiver
    a. the sender should send as much data as it can until the receiver's window size is 0.
    b. the sender should send a FIN if the input stream is finished and the sender has not sent a FIN.
    c. the sender should send a RST if the input stream has an error/receiver has sent a RST.
    d. special case: if the receiver's window size is 0, the sender should send a single byte of data to the
  receiver.
  */
  while ( ( !syn_received_ && !syn_has_sent_ )
          || ( syn_received_ && window_size_ - sequence_numbers_in_flight_ > 0
               && ( reader().bytes_buffered() != 0 || ( reader().is_finished() && !fin_has_sent_ )
                    || ( reader().has_error() && !rst_has_sent_ ) )
               && receiver_busy_ == 0 ) ) {
    TCPSenderMessage message
      = { next_seqno_,
          !syn_received_,
          string( reader().peek() )
            .substr( 0, min( window_size_ - sequence_numbers_in_flight_, TCPConfig::MAX_PAYLOAD_SIZE ) ),
          false,
          reader().has_error() };
    if ( !syn_received_ ) {
      syn_has_sent_ = true;
    }
    input_.reader().pop( message.payload.size() );

    if ( reader().is_finished() && window_size_ - message.sequence_length() > 0 ) {
      message.FIN = true;
    }
    fin_has_sent_ = message.FIN;

    if ( reader().has_error() ) {
      rst_has_sent_ = true;
    }

    window_.push( message );
    transmit( message );

    sequence_numbers_in_flight_ += message.sequence_length();
    next_seqno_ = next_seqno_ + message.sequence_length();
  }

  // special case
  if ( syn_received_ && window_size_ == 0 && receiver_reply_ ) {
    char busy_payload_ = 'a' + receiver_busy_;
    TCPSenderMessage message = { next_seqno_,
                                 false,
                                 ( receiver_busy_ < 3 ) ? string( 1, busy_payload_ ) : "",
                                 ( receiver_busy_ == 3 ) ? true : false,
                                 false };
    next_seqno_ = next_seqno_ + 1;
    sequence_numbers_in_flight_ += 1;
    transmit( message );
    window_.push( message );

    receiver_busy_ += 1;
    receiver_reply_ = false;
  }
}

TCPSenderMessage TCPSender::make_empty_message() const
{
  // Your code here.
  return TCPSenderMessage { next_seqno_, false, "", false, reader().has_error() };
}

void TCPSender::receive( const TCPReceiverMessage& msg )
{
  // Your code here.
  receiver_reply_ = true;
  RTO_ms_ = initial_RTO_ms_;

  if ( msg.RST ) {
    input_.reader().set_error();
    return;
  }

  if ( msg.ackno.has_value()
       && msg.ackno.value().unwrap( isn_, abs_isn_ ) <= next_seqno_.unwrap( isn_, abs_isn_ ) ) {
    syn_received_ = true;

    // former ackno is invalid
    if ( msg.ackno.value().unwrap( isn_, abs_isn_ ) < msg_abs_seqno_last_ ) {
      return;
    }

    window_size_ = msg.window_size;
    if ( window_size_ > 0 ) {
      receiver_busy_ = 0;
    }
    if ( msg.ackno.value().unwrap( isn_, abs_isn_ ) > msg_abs_seqno_last_ ) {
      sequence_numbers_in_flight_
        = next_seqno_.unwrap( isn_, abs_isn_ ) - msg.ackno.value().unwrap( isn_, abs_isn_ );
      uint64_t checkpoint = abs_isn_ + input_.reader().bytes_popped();
      while ( !window_.empty()
              && ( window_.front().seqno + window_.front().sequence_length() ).unwrap( isn_, checkpoint )
                   <= msg.ackno.value().unwrap( isn_, checkpoint ) ) {
        window_.pop();
      }

      msg_abs_seqno_last_ = msg.ackno.value().unwrap( isn_, abs_isn_ );
      ms_since_last_transmission_ = 0;
      consecutive_retransmissions_ = 0;
    }
  }
}

void TCPSender::tick( uint64_t ms_since_last_tick, const TransmitFunction& transmit )
{
  // Your code here.
  ms_since_last_transmission_ += ms_since_last_tick;
  if ( !window_.empty() && ms_since_last_transmission_ >= RTO_ms_ ) {
    transmit( window_.front() );
    consecutive_retransmissions_++;
    ms_since_last_transmission_ = 0;
    if ( receiver_busy_ == 0 ) {
      RTO_ms_ *= 2;
    }
  }
}
