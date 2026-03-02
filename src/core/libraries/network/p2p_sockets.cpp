// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <set>

#include <common/assert.h>
#include "core/libraries/kernel/kernel.h"
#include "net.h"
#include "net_error.h"
#include "sockets.h"

namespace Libraries::Net {

bool P2PSocket::IsValid() const {
#ifdef _WIN32
    return sock != INVALID_SOCKET;
#else
    return sock != -1;
#endif
}

int P2PSocket::Close() {
    LOG_ERROR(Lib_Net, "(STUBBED) called");
    return 0;
}

int P2PSocket::SetSocketOptions(int level, int optname, const void* optval, u32 optlen) {
    std::vector<u8> zero(optlen, 0);
    auto val = memcmp(optval, zero.data(), optlen) != 0;
    LOG_ERROR(Lib_Net, "(STUBBED) called, value is {}", val);
    return 0;
}

int P2PSocket::GetSocketOptions(int level, int optname, void* optval, u32* optlen) {
    LOG_ERROR(Lib_Net, "(STUBBED) called");
    return 0;
}

constexpr u16 ORBIS_NP_PORT = 3658;

struct P2PPort {
    P2PPort(u16 port) : port(port) {
        p2p_socket = socket(AF_INET, SOCK_DGRAM, 0);

        sockaddr_in p2p_addr {
            .sin_family = AF_INET,
            .sin_port = htons(port),
            .sin_addr = {0},
        };

        if (bind(p2p_socket, reinterpret_cast<const sockaddr*>(&p2p_addr), sizeof(p2p_addr)) == -1) {
            LOG_ERROR(Lib_Net, "failed to bind p2p socket to port {}", port);
        }
    }
    u16 port = 0;
    net_socket p2p_socket;

    std::mutex vports_mutex;
    std::map<u16, std::set<s32>> bound_p2p_vports{};
};

struct P2PContext {
    std::mutex p2p_ports_mutex;
    std::map<u16, P2PPort> p2p_ports;
    std::atomic<u32> num_p2p_ports = 0;

    P2PContext() : p2p_ports_mutex{}, p2p_ports{} {
        CreateP2PPort(ORBIS_NP_PORT);
    }

    P2PPort& CreateP2PPort(u16 port) {
        LOG_DEBUG(Lib_Net, "creating p2p socket {}", port);
        if (!p2p_ports.contains(port)) {
            p2p_ports.emplace(port, port);
            num_p2p_ports++;
        }

        return p2p_ports.at(port);
    }
};

P2PContext g_p2p_context;

int P2PSocket::Bind(const OrbisNetSockaddr* addr, u32 addrlen) {
    const auto* addr_in = reinterpret_cast<const OrbisNetSockaddrIn*>(addr);
    u16 port = htons(addr_in->sin_port);
    u16 vport = htons(addr_in->sin_vport);
    LOG_ERROR(Lib_Net,
              "(STUBBED) called, addr->sin_family = {}, addr->sin_port = {}, addr->sin_vport = {}, addr->sin_addr = {}",
              addr_in->sin_family, port, vport, inet_ntoa((in_addr)addr_in->sin_addr));

    if (port != 3658) {
        if (port == 0) {
            *Libraries::Kernel::__Error() = ORBIS_NET_EINVAL;
            return -1;
        }
    }

    net_socket native_socket {};
    {
        std::scoped_lock lk{g_p2p_context.p2p_ports_mutex};

        auto& p2p_port = g_p2p_context.CreateP2PPort(port);
        native_socket = p2p_port.p2p_socket;

        {
            std::scoped_lock lk{p2p_port.vports_mutex};
            if (vport == 0) {
                vport = 30000;
                while (p2p_port.bound_p2p_vports.contains(vport)) {
                    vport++;
                }
                LOG_DEBUG(Lib_Net, "found a free vport at {}", vport);
            }

            if (p2p_port.bound_p2p_vports.contains(vport)) {
                // check that all sockets are SO_REUSEADDR or SO_REUSEPORT
                // ...
                UNREACHABLE_MSG("multiple sockets bound to the same vport unimplemented");
            }
            else {
                std::set<s32> sockets{m_orbis_fd};
                p2p_port.bound_p2p_vports.emplace(vport, sockets);
            }
        }
    }

    {
        std::scoped_lock lk{m_mutex};
        this->port = port;
        this->vport = vport;
        this->sock = native_socket;
    }
    
    return 0;
}

int P2PSocket::Listen(int backlog) {
    LOG_ERROR(Lib_Net, "(STUBBED) called");
    return 0;
}

int P2PSocket::SendMessage(const OrbisNetMsghdr* msg, int flags) {
    LOG_ERROR(Lib_Net, "(STUBBED) called");
    *Libraries::Kernel::__Error() = ORBIS_NET_EAGAIN;
    return -1;
}

int P2PSocket::SendPacket(const void* msg, u32 len, int flags, const OrbisNetSockaddr* to,
                          u32 tolen) {
    LOG_ERROR(Lib_Net, "(STUBBED) called");
    *Libraries::Kernel::__Error() = ORBIS_NET_EAGAIN;
    return -1;
}

int P2PSocket::ReceiveMessage(OrbisNetMsghdr* msg, int flags) {
    LOG_ERROR(Lib_Net, "(STUBBED) called");
    *Libraries::Kernel::__Error() = ORBIS_NET_EAGAIN;
    return -1;
}

int P2PSocket::ReceivePacket(void* buf, u32 len, int flags, OrbisNetSockaddr* from, u32* fromlen) {
    LOG_ERROR(Lib_Net, "(STUBBED) called");
    *Libraries::Kernel::__Error() = ORBIS_NET_EAGAIN;
    return -1;
}

SocketPtr P2PSocket::Accept(OrbisNetSockaddr* addr, u32* addrlen) {
    LOG_ERROR(Lib_Net, "(STUBBED) called");
    *Libraries::Kernel::__Error() = ORBIS_NET_EAGAIN;
    return nullptr;
}

int P2PSocket::Connect(const OrbisNetSockaddr* addr, u32 namelen) {
    LOG_ERROR(Lib_Net, "(STUBBED) called");
    return 0;
}

int P2PSocket::GetSocketAddress(OrbisNetSockaddr* name, u32* namelen) {
    LOG_ERROR(Lib_Net, "(STUBBED) called");
    return 0;
}

int P2PSocket::GetPeerName(OrbisNetSockaddr* addr, u32* namelen) {
    LOG_ERROR(Lib_Net, "(STUBBED) called");
    return 0;
}

int P2PSocket::fstat(Libraries::Kernel::OrbisKernelStat* stat) {
    LOG_ERROR(Lib_Net, "(STUBBED) called");
    return 0;
}

} // namespace Libraries::Net