#include "network_interface.hh"
#include "../util/helpers.hh"

using namespace std;

static constexpr size_t ARP_TTL = 30'000;        // 30 seconds (ms)
static constexpr size_t ARP_REQUEST_INTERVAL = 5'000; // 5 seconds (ms)

NetworkInterface::NetworkInterface(std::string name, 
                                   std::shared_ptr<OutputPort> port,
                                   const EthernetAddress &ethernet_address,
                                   const Address &ip_address)
  : _name(std::move(name)),
    _output(std::move(port)),
    _ethernet_address(ethernet_address),
    _ip_address(ip_address),
    _frames_out(),
    _datagrams_in(),
    _arp_table(),
    _arp_request_time(),
    _waiting()
{}

NetworkInterface::NetworkInterface(const EthernetAddress &ethernet_address,
                                   const Address &ip_address)
  : _name(),
    _output(nullptr),
    _ethernet_address(ethernet_address),
    _ip_address(ip_address),
    _frames_out(),
    _datagrams_in(),
    _arp_table(),
    _arp_request_time(),
    _waiting()
{}

void NetworkInterface::transmit(const EthernetFrame& frame) {
    if (_output) {
        _output->transmit(*this, frame);
    } else {
        _frames_out.push(frame);
    }
}

void NetworkInterface::send_datagram(const InternetDatagram &dgram, const Address &next_hop) {
    uint32_t nh = next_hop.ipv4_numeric();

    // If we know the MAC, send immediately
    auto it = _arp_table.find(nh);
    if (it != _arp_table.end()) {
        EthernetFrame frame;
        frame.header.type = EthernetHeader::TYPE_IPv4;
        frame.header.src = _ethernet_address;
        frame.header.dst = it->second.mac;

        frame.payload = serialize(dgram);

        transmit(frame);
        return;
    }

    // Otherwise queue and send ARP request if allowed
    auto rit = _arp_request_time.find(nh);
    if (rit == _arp_request_time.end() || rit->second >= ARP_REQUEST_INTERVAL) {
        // Clear old queue when sending a new ARP request
        _waiting[nh].clear();
        _waiting[nh].push_back(dgram);
        send_arp_request(nh);
        _arp_request_time[nh] = 0; // reset timer; tick() will increment
    } else {
        // Just queue if we recently sent an ARP request
        _waiting[nh].push_back(dgram);
    }
}

std::optional<EthernetFrame> NetworkInterface::recv_frame(const EthernetFrame &frame) {
    const auto &hdr = frame.header;

    // Ignore frames not destined to us or to broadcast
    if (hdr.dst != _ethernet_address && hdr.dst != ETHERNET_BROADCAST)
        return std::nullopt;

    // IPv4 payload
    if (hdr.type == EthernetHeader::TYPE_IPv4) {
        InternetDatagram d;
        if (parse(d, frame.payload)) {
            _datagrams_in.push(d);
        }
        return std::nullopt;
    }

    // ARP payload
    if (hdr.type == EthernetHeader::TYPE_ARP) {
        ARPMessage msg;
        if (!parse(msg, frame.payload))
            return std::nullopt;

        uint32_t sender_ip = msg.sender_ip_address;
        EthernetAddress sender_mac = msg.sender_ethernet_address;

        // Learn mapping and reset ttl
        _arp_table[sender_ip] = { sender_mac, ARP_TTL };

        // Allow future ARP requests to be sent immediately (clear timer)
        _arp_request_time.erase(sender_ip);

        // If we have queued datagrams for this IP, send them now
        auto qit = _waiting.find(sender_ip);
        if (qit != _waiting.end()) {
            for (const InternetDatagram &d : qit->second) {
                EthernetFrame out;
                out.header.type = EthernetHeader::TYPE_IPv4;
                out.header.src = _ethernet_address;
                out.header.dst = sender_mac;

                out.payload = serialize(d);

                transmit(out);
            }
            _waiting.erase(qit);
        }

        // If this is an ARP request for our IP, reply
        if (msg.opcode == ARPMessage::OPCODE_REQUEST &&
            msg.target_ip_address == _ip_address.ipv4_numeric()) {

            EthernetFrame reply;
            reply.header.type = EthernetHeader::TYPE_ARP;
            reply.header.src = _ethernet_address;
            reply.header.dst = sender_mac;

            ARPMessage rep;
            rep.opcode = ARPMessage::OPCODE_REPLY;
            rep.sender_ip_address = _ip_address.ipv4_numeric();
            rep.sender_ethernet_address = _ethernet_address;
            rep.target_ip_address = sender_ip;
            rep.target_ethernet_address = sender_mac;

            reply.payload = serialize(rep);

            transmit(reply);
        }

        return std::nullopt;
    }

    return std::nullopt;
}

void NetworkInterface::tick(size_t ms_since_last_tick) {
    // Expire ARP cache entries
    for (auto it = _arp_table.begin(); it != _arp_table.end(); ) {
        if (it->second.ttl <= ms_since_last_tick) {
            it = _arp_table.erase(it);
        } else {
            it->second.ttl -= ms_since_last_tick;
            ++it;
        }
    }

    // Increment ARP request timers and resend if expired and waiting datagrams exist
    for (auto &p : _arp_request_time) {
        p.second += ms_since_last_tick;
        if (p.second >= ARP_REQUEST_INTERVAL && _waiting.count(p.first)) {
            // Clear old queued datagrams when resending ARP request
            _waiting[p.first].clear();
            send_arp_request(p.first);
            p.second = 0; // reset timer
        }
    }
}

std::optional<EthernetFrame> NetworkInterface::maybe_send() {
    if (_frames_out.empty())
        return std::nullopt;

    EthernetFrame f = _frames_out.front();
    _frames_out.pop();
    return f;
}

void NetworkInterface::send_arp_request(uint32_t ip) {
    // reset timer; tick increments it
    _arp_request_time[ip] = 0;

    EthernetFrame frame;
    frame.header.type = EthernetHeader::TYPE_ARP;
    frame.header.src = _ethernet_address;
    frame.header.dst = ETHERNET_BROADCAST;

    ARPMessage msg;
    msg.opcode = ARPMessage::OPCODE_REQUEST;
    msg.sender_ip_address = _ip_address.ipv4_numeric();
    msg.sender_ethernet_address = _ethernet_address;
    msg.target_ip_address = ip;
    // target_ethernet_address can be left default (zeros)

    frame.payload = serialize(msg);

    transmit(frame);
}
