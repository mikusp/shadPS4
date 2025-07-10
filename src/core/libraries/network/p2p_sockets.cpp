// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "common/assert.h"
#include "common/error.h"
#include "net.h"
#include "net_error.h"
#include "sockets.h"

namespace Libraries::Net {

int StripP2P(int type) {
    switch (type) {
    case ORBIS_NET_SOCK_DGRAM_P2P:
    case ORBIS_NET_SOCK_STREAM_P2P:
        return SOCK_DGRAM;
    default:
        UNREACHABLE_MSG("unexpected P2P socket type: {}", type);
    }
}

P2PSocket::P2PSocket(int domain, int type, int protocol) : Socket(domain, type, protocol) {
    LOG_INFO(Lib_Net, "domain = {}, type = {}, protocol = {}", domain, type, protocol);
    sock = socket(ConvertFamilies(domain), StripP2P(type), protocol);
    LOG_DEBUG(Lib_Net, "socket = {}", sock);
    socket_type = type;
}

int P2PSocket::Close() {
    LOG_ERROR(Lib_Net, "(STUBBED) called");
    return 0;
}

#define CASE_SETSOCKOPT(opt)                                                                       \
    case ORBIS_NET_##opt:                                                                          \
        return ConvertReturnErrorCode(setsockopt(sock, level, opt, (const char*)optval, optlen))

#define CASE_SETSOCKOPT_VALUE(opt, value)                                                          \
    case opt:                                                                                      \
        if (optlen != sizeof(*value)) {                                                            \
            return ORBIS_NET_ERROR_EFAULT;                                                         \
        }                                                                                          \
        memcpy(value, optval, optlen);                                                             \
        return 0

#define CASE_SETSOCKOPT_VALUE_BINARY(opt, value)                                                   \
    case opt: {                                                                                    \
        if (optlen != sizeof(*value)) {                                                            \
            LOG_WARNING(Lib_Net, "wrong size for setting " #opt);                                  \
            if (optlen == sizeof(u64)) {                                                           \
                const auto optval_u64 = reinterpret_cast<const u64*>(optval);                      \
                const bool result = *optval_u64 != 0;                                              \
                memset(value, 0, sizeof(*value));                                                  \
                memcpy(value, &result, 1);                                                         \
            } else {                                                                               \
                return ORBIS_NET_ERROR_EINVAL;                                                     \
            }                                                                                      \
        } else {                                                                                   \
            memcpy(value, optval, optlen);                                                         \
        }                                                                                          \
        return 0;                                                                                  \
    }

int P2PSocket::SetSocketOptions(int level, int optname, const void* optval, u32 optlen) {
    level = ConvertLevels(level);
    LOG_INFO(Lib_Net, "level = {}, optname = {}, optlen = {}", level, optname, optlen);
    std::scoped_lock lock{m_mutex};
    ::linger native_linger;
    if (level == SOL_SOCKET) {
        switch (optname) {
            CASE_SETSOCKOPT(SO_REUSEADDR);
            CASE_SETSOCKOPT(SO_KEEPALIVE);
            CASE_SETSOCKOPT(SO_BROADCAST);
            // CASE_SETSOCKOPT(SO_LINGER);
            CASE_SETSOCKOPT(SO_SNDBUF);
            CASE_SETSOCKOPT(SO_RCVBUF);
            CASE_SETSOCKOPT(SO_SNDTIMEO);
            CASE_SETSOCKOPT(SO_RCVTIMEO);
            CASE_SETSOCKOPT(SO_ERROR);
            CASE_SETSOCKOPT(SO_TYPE);
            CASE_SETSOCKOPT_VALUE_BINARY(ORBIS_NET_SO_REUSEPORT, &sockopt_so_reuseport);
            CASE_SETSOCKOPT_VALUE_BINARY(ORBIS_NET_SO_ONESBCAST, &sockopt_so_onesbcast);
            CASE_SETSOCKOPT_VALUE_BINARY(ORBIS_NET_SO_USECRYPTO, &sockopt_so_usecrypto);
            CASE_SETSOCKOPT_VALUE_BINARY(ORBIS_NET_SO_USESIGNATURE, &sockopt_so_usesignature);
        case ORBIS_NET_SO_LINGER: {
            if (socket_type != ORBIS_NET_SOCK_STREAM) {
                return ORBIS_NET_EPROCUNAVAIL;
            }
            if (optlen < sizeof(OrbisNetLinger)) {
                LOG_ERROR(Lib_Net, "size missmatched! optlen = {} OrbisNetLinger={}", optlen,
                          sizeof(OrbisNetLinger));
                return ORBIS_NET_ERROR_EINVAL;
            }

            const void* native_val = &native_linger;
            u32 native_len = sizeof(native_linger);
            native_linger.l_onoff = reinterpret_cast<const OrbisNetLinger*>(optval)->l_onoff;
            native_linger.l_linger = reinterpret_cast<const OrbisNetLinger*>(optval)->l_linger;
            return ConvertReturnErrorCode(
                setsockopt(sock, level, SO_LINGER, (const char*)native_val, native_len));
        }

        case ORBIS_NET_SO_NAME:
            return ORBIS_NET_ERROR_EINVAL; // don't support set for name
        case ORBIS_NET_SO_NBIO: {
            if (optlen != sizeof(sockopt_so_nbio)) {
                LOG_WARNING(Lib_Net, "wrong size for setting SO_NBIO");
                if (optlen == sizeof(u64)) {
                    const auto value = reinterpret_cast<const u64*>(optval);
                    decltype(sockopt_so_nbio) nbio = *value != 0;
                    memcpy(&sockopt_so_nbio, &nbio, sizeof(sockopt_so_nbio));
                } else {
                    return ORBIS_NET_ERROR_EINVAL;
                }
            } else {
                memcpy(&sockopt_so_nbio, optval, optlen);
            }
#ifdef _WIN32
            static_assert(sizeof(u_long) == sizeof(sockopt_so_nbio),
                          "type used for ioctlsocket value does not have the expected size");
            return ConvertReturnErrorCode(ioctlsocket(sock, FIONBIO, (u_long*)&sockopt_so_nbio));
#else
            return ConvertReturnErrorCode(ioctl(sock, FIONBIO, &sockopt_so_nbio));
#endif
        }
        }
    } else if (level == IPPROTO_IP) {
        switch (optname) {
            // CASE_SETSOCKOPT(IP_HDRINCL);
            CASE_SETSOCKOPT(IP_TOS);
            CASE_SETSOCKOPT(IP_TTL);
            CASE_SETSOCKOPT(IP_MULTICAST_IF);
            CASE_SETSOCKOPT(IP_MULTICAST_TTL);
            CASE_SETSOCKOPT(IP_MULTICAST_LOOP);
            CASE_SETSOCKOPT(IP_ADD_MEMBERSHIP);
            CASE_SETSOCKOPT(IP_DROP_MEMBERSHIP);
            CASE_SETSOCKOPT_VALUE(ORBIS_NET_IP_TTLCHK, &sockopt_ip_ttlchk);
            CASE_SETSOCKOPT_VALUE(ORBIS_NET_IP_MAXTTL, &sockopt_ip_maxttl);
        case ORBIS_NET_IP_HDRINCL: {
            if (socket_type != ORBIS_NET_SOCK_RAW) {
                return ORBIS_NET_EPROCUNAVAIL;
            }
            return ConvertReturnErrorCode(
                setsockopt(sock, level, optname, (const char*)optval, optlen));
        }
        }
    } else if (level == IPPROTO_TCP) {
        switch (optname) {
            CASE_SETSOCKOPT(TCP_NODELAY);
            CASE_SETSOCKOPT(TCP_MAXSEG);
            CASE_SETSOCKOPT_VALUE(ORBIS_NET_TCP_MSS_TO_ADVERTISE, &sockopt_tcp_mss_to_advertise);
        }
    }

    UNREACHABLE_MSG("Unknown level ={} optname ={}", level, optname);
    return 0;
}

int P2PSocket::GetSocketOptions(int level, int optname, void* optval, u32* optlen) {
    LOG_ERROR(Lib_Net, "(STUBBED) called, level = {}, optname = {}", level, optname);
    return 0;
}

std::string AddrToString(const sockaddr* addr) {
    char hbuf[NI_MAXHOST];

    if (getnameinfo(addr, sizeof(sockaddr), hbuf, sizeof(hbuf), nullptr, 0,
                    NI_NUMERICHOST | NI_NUMERICSERV))
        return "";
    else
        return std::string{hbuf};
}

int P2PSocket::Bind(const OrbisNetSockaddr* addr, u32 addrlen) {
    std::scoped_lock lock{m_mutex};

    sockaddr addr2;
    convertOrbisNetSockaddrToPosix(addr, &addr2);
    const sockaddr_in* in_addr2 = reinterpret_cast<const sockaddr_in*>(&addr2);

    LOG_DEBUG(Lib_Net, "addr = {}, port = {}, vport = {}", AddrToString(&addr2), in_addr2->sin_port,
              ((const OrbisNetSockaddrIn*)addr)->sin_vport);

    const auto result = ::bind(sock, &addr2, sizeof(addr2));
    LOG_DEBUG(Lib_Net, "raw bind result = {}, errno = {}", result,
              result == -1 ? Common::GetLastErrorMsg() : "none");
    return ConvertReturnErrorCode(result);
}

int P2PSocket::Listen(int backlog) {
    LOG_ERROR(Lib_Net, "(STUBBED) called, backlog = {}", backlog);
    return 0;
}

int P2PSocket::SendPacket(const void* msg, u32 len, int flags, const OrbisNetSockaddr* to,
                          u32 tolen) {
    LOG_ERROR(Lib_Net, "(STUBBED) called");
    *Libraries::Kernel::__Error() = ORBIS_NET_EAGAIN;
    return -1;
}

s64 P2PSocket::ReceivePacket(void* buf, u32 len, int flags, OrbisNetSockaddr* from, u32* fromlen) {
    std::scoped_lock lock{m_mutex};
    LOG_DEBUG(Lib_Net, "len = {}, flags = {:#x}, from = {:#x}", len, flags,
        reinterpret_cast<u64>(from));
    if (from != nullptr) {
        sockaddr addr;
        s64 res = recvfrom(sock, (char*)buf, len, flags, &addr, (socklen_t*)fromlen);
        LOG_DEBUG(Lib_Net, "recvfrom raw result: {:#x} ({})", (u32)res, Common::GetLastErrorMsg());
        convertPosixSockaddrToOrbis(&addr, from);
        *fromlen = sizeof(OrbisNetSockaddrIn);
        return ConvertReturnErrorCode(res);
    } else {
        return ConvertReturnErrorCode(recv(sock, (char*)buf, len, flags));
    }
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