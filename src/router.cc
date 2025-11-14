#include "router.hh"
#include "address.hh"
#include <optional>
#include <limits>
#include <algorithm>

using namespace std;

void Router::add_route(uint32_t route_prefix,
                       uint8_t prefix_length,
                       optional<Address> next_hop,
                       size_t interface_num)
{
    routing_table_.push_back(Route{
        route_prefix,
        prefix_length,
        next_hop,
        interface_num
    });
}

void Router::route()
{
    for (size_t i = 0; i < interfaces_.size(); i++)
    {
        auto &iface = interfaces_.at(i);

        // While there are datagrams waiting
        auto &queue = iface->datagrams_received();
        while (!queue.empty())
        {
            InternetDatagram dgram = queue.front();
            queue.pop();

            uint32_t dst = dgram.header.dst;

            // Longest prefix match
            const Route *best_route = nullptr;
            uint8_t best_len = 0;

            for (const auto &route : routing_table_)
            {
                uint32_t mask = route.prefix_len == 0 ? 0 : (0xFFFFFFFF << (32 - route.prefix_len));
                if ((dst & mask) == (route.prefix & mask))
                {
                    if (route.prefix_len >= best_len)
                    {
                        best_len = route.prefix_len;
                        best_route = &route;
                    }
                }
            }

            if (!best_route)
                continue;

            // TTL
            if (dgram.header.ttl <= 1)
                continue;
            dgram.header.ttl--;
            dgram.header.compute_checksum();

            // Next hop selection
            Address target =
                best_route->next_hop.has_value()
                ? best_route->next_hop.value()
                : Address::from_ipv4_numeric(dst);

            interfaces_.at(best_route->interface_num)->send_datagram(dgram, target);
        }
    }
}
