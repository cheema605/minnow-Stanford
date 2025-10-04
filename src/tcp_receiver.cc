#include "tcp_receiver.hh"
#include "debug.hh"

using namespace std;

void TCPReceiver::receive(TCPSenderMessage message) {
    // 1. Set the Initial Sequence Number (ISN) if this segment has SYN
    if (!isn_set_ && message.SYN) {
        isn_ = message.seqno;
        isn_set_ = true;
    }

    // Cannot process any data before SYN
    if (!isn_set_) return;

    // 2. Determine checkpoint for unwrap (absolute index guess)
    uint64_t checkpoint = reassembler_.reader().bytes_popped() + reassembler_.count_bytes_pending();

    // 3. Convert sequence number to absolute 64-bit sequence number
    uint64_t abs_seq = message.seqno.unwrap(isn_, checkpoint);

    // 4. Convert absolute seqno to stream index for Reassembler
    // SYN consumes 1 sequence number, but isn't part of stream
    size_t stream_index = abs_seq - 1;

    // 5. Push payload into Reassembler
    reassembler_.insert(stream_index, message.payload, message.FIN);
}

TCPReceiverMessage TCPReceiver::send() const {
    TCPReceiverMessage msg;

    if (isn_set_) {
        // The next byte we need = first unassembled byte
        uint64_t abs_ack = reassembler_.reader().bytes_popped() + reassembler_.count_bytes_pending();
        msg.ackno = Wrap32::wrap(abs_ack + 1, isn_); // +1 for SYN
    }

    // Flow-control window: how many more bytes we can accept
    msg.window_size = static_cast<uint16_t>(reassembler_.writer().available_capacity());

    // Reset flag not used here
    msg.RST = false;

    return msg;
}
