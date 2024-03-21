#include "reassembler.hh"

using namespace std;

void Reassembler::insert( uint64_t first_index, string data, bool is_last_substring )
{
  // Your code here.
  // new substring before the next_index_ && exceeds the capacity will be discarded
  if ( first_index + data.length() < next_index_
       || first_index >= output_.writer().available_capacity() + next_index_ ) {
    return;
  } else {
    // the overlapping substring will be truncated
    if ( first_index < next_index_ ) {
      data = data.substr( next_index_ - first_index, output_.writer().available_capacity() );
      first_index = next_index_;
    }
    // the part of substring exceeds the capacity will be truncated
    if ( first_index >= next_index_
         && next_index_ + output_.writer().available_capacity() - first_index < data.length() ) {
      data.resize( next_index_ + output_.writer().available_capacity() - first_index );
      is_last_substring = false;
    }
    // truncate the part before the next_index_
    if ( next_index_ > first_index ) {
      data = data.substr( next_index_ - first_index );
    }
    Substring next = { first_index, data.length(), move( data ) };
    // check if the new substring may overlaps whole substring in the pending_
    if ( !pending_.empty() ) {
      auto it = pending_.lower_bound( next );
      if ( it != pending_.begin() ) {
        it--;
        /*
         *      [    )    <- next  |   [ )   <- next
         *    [   )       <- it    |  [    ) <- it
         */
        if ( it->first_index + it->len > next.first_index ) {
          if ( it->first_index + it->len >= next.first_index + next.len ) {
            return;
          }
          uint64_t diff = it->first_index + it->len - next.first_index;
          next = { it->first_index, it->len + next.len - diff, it->data.substr( 0, it->len - diff ) + next.data };
          bytes_pending_ -= it->len;
          pending_.erase( it++ );
        } else {
          it++;
        }
      }

      for ( ; it != pending_.end(); ) {
        /*
         *  [  )     <- next
         *     [  )  <- it
         */
        if ( it->first_index >= next.first_index + next.len ) {
          break;
        }
        /*
         *  [      )     <- next
         *   [  )        <- it
         */
        if ( it->first_index + it->len <= next.first_index + next.len ) {
          bytes_pending_ -= it->len;
          pending_.erase( it++ );
        } else {
          uint64_t diff = next.first_index + next.len - it->first_index;
          next = { next.first_index, next.len + it->len - diff, next.data + it->data.substr( diff ) };
          bytes_pending_ -= it->len;
          pending_.erase( it++ );
        }
      }
    }
    pending_.insert( next );
    bytes_pending_ += next.len;
    for ( auto it = pending_.begin(); it != pending_.end(); ) {
      if ( next_index_ == it->first_index ) {
        output_.writer().push( it->data );
        next_index_ = output_.writer().bytes_pushed();
        bytes_pending_ -= it->len;
        pending_.erase( it++ );
      } else {
        break;
      }
    }
    if ( is_last_substring ) {
      last_substring_found_ = true;
    }
    if ( last_substring_found_ && pending_.empty() ) {
      output_.writer().close();
    }
  }
}

uint64_t Reassembler::bytes_pending() const
{
  // Your code here.
  return bytes_pending_;
}
