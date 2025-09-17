// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <common/assert.h>
#include "core/libraries/kernel/kernel.h"
#include "net.h"
#include "net_error.h"
#include "sockets.h"

namespace Libraries::Net {

static int StripP2P(int type) {
    switch (type) {
    case ORBIS_NET_SOCK_DGRAM_P2P:
        return SOCK_DGRAM;
    default:
        UNREACHABLE_MSG("unsupported type {}", type);
    }
}

P2PSocket::P2PSocket(int domain, int type, int protocol) : Socket(domain, type, protocol) {
    sock = socket(domain, StripP2P(type), protocol);
    socket_type = type;
}


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
    LOG_ERROR(Lib_Net, "(STUBBED) called");
    return 0;
}

int P2PSocket::GetSocketOptions(int level, int optname, void* optval, u32* optlen) {
    LOG_ERROR(Lib_Net, "(STUBBED) called");
    return 0;
}

int P2PSocket::Bind(const OrbisNetSockaddr* addr, u32 addrlen) {
    auto sockaddr = (const OrbisNetSockaddrIn*)addr;
    
    LOG_ERROR(Lib_Net, "port = {}, addr = {}, vport = {}", sockaddr->sin_port, inet_ntoa((in_addr)(sockaddr->sin_addr)), sockaddr->sin_vport);
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