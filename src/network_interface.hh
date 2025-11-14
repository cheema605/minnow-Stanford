#pragma once

#include "address.hh"
#include "ethernet_frame.hh"
#include "arp_message.hh"
#include "ipv4_datagram.hh"

#include <cstdint>
#include <optional>
#include <unordered_map>
#include <queue>
#include <vector>
#include <memory>
#include <string>

class NetworkInterface {
  public:
    // An abstraction for the physical output port for a NetworkInterface
    class OutputPort {
      public:
        virtual void transmit(const NetworkInterface& sender, const EthernetFrame& frame) = 0;
        virtual ~OutputPort() = default;
    };

    NetworkInterface(std::string name, std::shared_ptr<OutputPort> port, const EthernetAddress &ethernet_address, const Address &ip_address);
    NetworkInterface(const EthernetAddress &ethernet_address, const Address &ip_address);

    void send_datagram(const InternetDatagram &dgram, const Address &next_hop);
    std::optional<EthernetFrame> recv_frame(const EthernetFrame &frame);
    void tick(size_t ms_since_last_tick);
    std::optional<EthernetFrame> maybe_send();

    const std::string& name() const { return _name; }
    
    // return a reference so callers (router) can bind to it
    std::queue<InternetDatagram> & datagrams_received() { return _datagrams_in; }

  private:
    std::string _name;
    std::shared_ptr<OutputPort> _output;
    EthernetAddress _ethernet_address;
    Address _ip_address;

    std::queue<EthernetFrame> _frames_out;
    std::queue<InternetDatagram> _datagrams_in;

    struct ARPEntry {
        EthernetAddress mac {};
        size_t ttl {};
    };

    // IP (numeric) -> ARPEntry
    std::unordered_map<uint32_t, ARPEntry> _arp_table;

    // IP -> ms since last ARP request was sent (we increment in tick)
    std::unordered_map<uint32_t, size_t> _arp_request_time;

    // IP -> queued datagrams waiting for ARP reply
    std::unordered_map<uint32_t, std::vector<InternetDatagram>> _waiting;

    void send_arp_request(uint32_t ip);
    void transmit(const EthernetFrame& frame);
};
