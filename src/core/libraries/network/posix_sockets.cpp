// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <common/assert.h>
#include "common/error.h"
#include "core/libraries/kernel/file_system.h"
#include "net.h"
#ifndef _WIN32
#include <sys/stat.h>
#endif
#include "net_error.h"
#include "sockets.h"

namespace Libraries::Net {

PosixSocket::PosixSocket(int domain, int type, int protocol) : Socket(domain, type, protocol) {
    sock = socket(ConvertFamilies(domain), type, protocol);
    LOG_DEBUG(Lib_Net, "socket = {}", sock);
    socket_type = type;
}

bool PosixSocket::IsValid() const {
#ifdef _WIN32
    return sock != INVALID_SOCKET;
#else
    return sock != -1;
#endif
}

int PosixSocket::Close() {
    std::scoped_lock lock{m_mutex};
#ifdef _WIN32
    auto out = closesocket(sock);
#else
    auto out = ::close(sock);
#endif
    return ConvertReturnErrorCode(out);
}

int PosixSocket::Bind(const OrbisNetSockaddr* addr, u32 addrlen) {
    std::scoped_lock lock{m_mutex};

    sockaddr addr2;
    convertOrbisNetSockaddrToPosix(addr, &addr2);
    const auto result = ::bind(sock, &addr2, sizeof(addr2));
    LOG_DEBUG(Lib_Net, "raw bind result = {}, errno = {}", result,
              result == -1 ? Common::GetLastErrorMsg() : "none");
    return ConvertReturnErrorCode(result);
}

int PosixSocket::Listen(int backlog) {
    std::scoped_lock lock{m_mutex};
    LOG_DEBUG(Lib_Net, "called");

    return ConvertReturnErrorCode(::listen(sock, backlog));
}

int PosixSocket::SendPacket(const void* msg, u32 len, int flags, const OrbisNetSockaddr* to,
                            u32 tolen) {
    std::scoped_lock lock{m_mutex};
    LOG_DEBUG(Lib_Net, "called");

    if (to != nullptr) {
        sockaddr addr;
        convertOrbisNetSockaddrToPosix(to, &addr);
        return ConvertReturnErrorCode(
            sendto(sock, (const char*)msg, len, flags, &addr, sizeof(sockaddr_in)));
    } else {
        return ConvertReturnErrorCode(send(sock, (const char*)msg, len, flags));
    }
}

s64 PosixSocket::ReceivePacket(void* buf, u32 len, int flags, OrbisNetSockaddr* from,
                               u32* fromlen) {
    std::scoped_lock lock{receive_mutex};
    if (from != nullptr) {
        sockaddr addr;
        ssize_t res = recvfrom(sock, (char*)buf, len, flags, &addr, (socklen_t*)fromlen);
        LOG_DEBUG(Lib_Net, "recvfrom raw: {:#x}", (u32)res);
        convertPosixSockaddrToOrbis(&addr, from);
        *fromlen = sizeof(OrbisNetSockaddrIn);
        return ConvertReturnErrorCode(res);
    } else {
        ssize_t res = recv(sock, (char*)buf, len, flags);
        LOG_DEBUG(Lib_Net, "recv raw: {:#x}", (u32)res);
        return ConvertReturnErrorCode(res);
    }
}

SocketPtr PosixSocket::Accept(OrbisNetSockaddr* addr, u32* addrlen) {
    std::scoped_lock lock{m_mutex};
    LOG_DEBUG(Lib_Net, "called");

    sockaddr addr2;
    socklen_t len = sizeof(addr2);
    net_socket new_socket = ::accept(sock, &addr2, &len);
#ifdef _WIN32
    if (new_socket != INVALID_SOCKET) {
#else
    if (new_socket >= 0) {
#endif
        if (addr && addrlen) {
            convertPosixSockaddrToOrbis(&addr2, addr);
            *addrlen = sizeof(OrbisNetSockaddrIn);
        }
        return std::make_shared<PosixSocket>(new_socket);
    }
    else {
        ConvertReturnErrorCode(new_socket);
    }
    return nullptr;
}

int PosixSocket::Connect(const OrbisNetSockaddr* addr, u32 namelen) {
    std::scoped_lock lock{m_mutex};
    LOG_DEBUG(Lib_Net, "called");

    sockaddr addr2;
    convertOrbisNetSockaddrToPosix(addr, &addr2);
    int result = 0;
    do {
        result = ::connect(sock, &addr2, sizeof(sockaddr_in));
        LOG_DEBUG(Lib_Net, "raw connect result = {}, errno = {}", result,
                  result == -1 ? Common::GetLastErrorMsg() : "none");
    } while (result == -1 && (errno == EINTR || errno == EINPROGRESS));
    return ConvertReturnErrorCode(result);
}

int PosixSocket::GetSocketAddress(OrbisNetSockaddr* name, u32* namelen) {
    std::scoped_lock lock{m_mutex};
    LOG_DEBUG(Lib_Net, "called");

    sockaddr addr;
    convertOrbisNetSockaddrToPosix(name, &addr);
    if (name != nullptr) {
        *namelen = sizeof(sockaddr_in);
    }
    int res = getsockname(sock, &addr, (socklen_t*)namelen);
    if (res >= 0) {
        convertPosixSockaddrToOrbis(&addr, name);
        *namelen = sizeof(OrbisNetSockaddrIn);
    }
    return ConvertReturnErrorCode(res);
}

#define CASE_SETSOCKOPT(opt)                                                                       \
    case ORBIS_NET_##opt:                                                                          \
        return ConvertReturnErrorCode(                                                             \
            setsockopt(sock, native_level, opt, (const char*)optval, optlen))

#define CASE_SETSOCKOPT_VALUE(opt, value)                                                          \
    case opt:                                                                                      \
        if (optlen != sizeof(*value)) {                                                            \
            *Libraries::Kernel::__Error() = ORBIS_NET_EFAULT;                                      \
            return -1;                                                                             \
        }                                                                                          \
        memcpy(value, optval, optlen);                                                             \
        return 0

int PosixSocket::SetSocketOptions(int level, int optname, const void* optval, u32 optlen) {
    std::scoped_lock lock{m_mutex};
    LOG_DEBUG(Lib_Net, "level = {}, optname = {}, optlen = {}", level, optname, optlen);
    s32 native_level = ConvertLevels(level);
    ::linger native_linger;
    if (native_level == SOL_SOCKET) {
        switch (optname) {
            CASE_SETSOCKOPT(SO_REUSEADDR);
            CASE_SETSOCKOPT(SO_KEEPALIVE);
            CASE_SETSOCKOPT(SO_BROADCAST);
            // CASE_SETSOCKOPT(SO_LINGER);
            CASE_SETSOCKOPT(SO_SNDBUF);
            CASE_SETSOCKOPT(SO_RCVBUF);
            CASE_SETSOCKOPT(SO_SNDTIMEO);
            CASE_SETSOCKOPT(SO_RCVTIMEO);
            CASE_SETSOCKOPT(SO_TYPE);
            CASE_SETSOCKOPT_VALUE(ORBIS_NET_SO_CONNECTTIMEO, &sockopt_so_connecttimeo);
            CASE_SETSOCKOPT_VALUE(ORBIS_NET_SO_REUSEPORT, &sockopt_so_reuseport);
            CASE_SETSOCKOPT_VALUE(ORBIS_NET_SO_ONESBCAST, &sockopt_so_onesbcast);
            CASE_SETSOCKOPT_VALUE(ORBIS_NET_SO_USECRYPTO, &sockopt_so_usecrypto);
            CASE_SETSOCKOPT_VALUE(ORBIS_NET_SO_USESIGNATURE, &sockopt_so_usesignature);
        case ORBIS_NET_SO_ERROR: {
            *Libraries::Kernel::__Error() = ORBIS_NET_ENOPROTOOPT;
            return -1;
        }
        case ORBIS_NET_SO_LINGER: {
            if (socket_type != ORBIS_NET_SOCK_STREAM) {
                *Libraries::Kernel::__Error() = ORBIS_NET_EPROCUNAVAIL;
                return -1;
            }
            if (optlen < sizeof(OrbisNetLinger)) {
                LOG_ERROR(Lib_Net, "size missmatched! optlen = {} OrbisNetLinger={}", optlen,
                          sizeof(OrbisNetLinger));
                *Libraries::Kernel::__Error() = ORBIS_NET_EINVAL;
                return -1;
            }

            const void* native_val = &native_linger;
            u32 native_len = sizeof(native_linger);
            native_linger.l_onoff = reinterpret_cast<const OrbisNetLinger*>(optval)->l_onoff;
            native_linger.l_linger = reinterpret_cast<const OrbisNetLinger*>(optval)->l_linger;
            return ConvertReturnErrorCode(
                setsockopt(sock, native_level, SO_LINGER, (const char*)native_val, native_len));
        }

        case ORBIS_NET_SO_NAME:
            *Libraries::Kernel::__Error() = ORBIS_NET_EINVAL;
            return -1; // don't support set for name
        case ORBIS_NET_SO_NBIO: {
            if (optlen < sizeof(sockopt_so_nbio)) {
                *Libraries::Kernel::__Error() = ORBIS_NET_EINVAL;
                return -1;
            } else {
                memcpy(&sockopt_so_nbio, optval, sizeof(sockopt_so_nbio));
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
    } else if (native_level == IPPROTO_IP) {
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
                *Libraries::Kernel::__Error() = ORBIS_NET_EPROCUNAVAIL;
                return -1;
            }
            return ConvertReturnErrorCode(
                setsockopt(sock, native_level, optname, (const char*)optval, optlen));
        }
        }
    } else if (native_level == IPPROTO_TCP) {
        switch (optname) {
            CASE_SETSOCKOPT(TCP_NODELAY);
            CASE_SETSOCKOPT(TCP_MAXSEG);
            CASE_SETSOCKOPT_VALUE(ORBIS_NET_TCP_MSS_TO_ADVERTISE, &sockopt_tcp_mss_to_advertise);
        }
    }

    UNREACHABLE_MSG("Unknown level ={} optname ={}", level, optname);
    return 0;
}

#define CASE_GETSOCKOPT(opt)                                                                       \
    case ORBIS_NET_##opt: {                                                                        \
        socklen_t optlen_temp = *optlen;                                                           \
        auto retval = ConvertReturnErrorCode(                                                      \
            getsockopt(sock, native_level, opt, (char*)optval, &optlen_temp));                     \
        *optlen = optlen_temp;                                                                     \
        return retval;                                                                             \
    }
#define CASE_GETSOCKOPT_VALUE(opt, value)                                                          \
    case opt:                                                                                      \
        if (*optlen < sizeof(value)) {                                                             \
            *optlen = sizeof(value);                                                               \
            *Libraries::Kernel::__Error() = ORBIS_NET_EFAULT;                                      \
            return -1;                                                                             \
        }                                                                                          \
        *optlen = sizeof(value);                                                                   \
        *(decltype(value)*)optval = value;                                                         \
        return 0;

int PosixSocket::GetSocketOptions(int level, int optname, void* optval, u32* optlen) {
    std::scoped_lock lock{m_mutex};
    s32 native_level = ConvertLevels(level);
    if (native_level == SOL_SOCKET) {
        switch (optname) {
        case ORBIS_NET_SO_ERROR: {
            socklen_t optlen_temp = *optlen;
            auto result = getsockopt(sock, native_level, SO_ERROR, (char*)optval, &optlen_temp);
            *optlen = optlen_temp;
            return ConvertLiteralErrorCode(result);
        }
            CASE_GETSOCKOPT(SO_REUSEADDR);
            CASE_GETSOCKOPT(SO_KEEPALIVE);
            CASE_GETSOCKOPT(SO_BROADCAST);
            CASE_GETSOCKOPT(SO_LINGER);
            CASE_GETSOCKOPT(SO_SNDBUF);
            CASE_GETSOCKOPT(SO_RCVBUF);
            CASE_GETSOCKOPT(SO_SNDTIMEO);
            CASE_GETSOCKOPT(SO_RCVTIMEO);
            // CASE_GETSOCKOPT(SO_ERROR);
            CASE_GETSOCKOPT(SO_TYPE);
            CASE_GETSOCKOPT_VALUE(ORBIS_NET_SO_NBIO, sockopt_so_nbio);
            CASE_GETSOCKOPT_VALUE(ORBIS_NET_SO_CONNECTTIMEO, sockopt_so_connecttimeo);
            CASE_GETSOCKOPT_VALUE(ORBIS_NET_SO_REUSEPORT, sockopt_so_reuseport);
            CASE_GETSOCKOPT_VALUE(ORBIS_NET_SO_ONESBCAST, sockopt_so_onesbcast);
            CASE_GETSOCKOPT_VALUE(ORBIS_NET_SO_USECRYPTO, sockopt_so_usecrypto);
            CASE_GETSOCKOPT_VALUE(ORBIS_NET_SO_USESIGNATURE, sockopt_so_usesignature);
            CASE_GETSOCKOPT_VALUE(ORBIS_NET_SO_NAME,
                                  (char)0); // writes an empty string to the output buffer
        }
    } else if (native_level == IPPROTO_IP) {
        switch (optname) {
            CASE_GETSOCKOPT(IP_HDRINCL);
            CASE_GETSOCKOPT(IP_TOS);
            CASE_GETSOCKOPT(IP_TTL);
            CASE_GETSOCKOPT(IP_MULTICAST_IF);
            CASE_GETSOCKOPT(IP_MULTICAST_TTL);
            CASE_GETSOCKOPT(IP_MULTICAST_LOOP);
            CASE_GETSOCKOPT(IP_ADD_MEMBERSHIP);
            CASE_GETSOCKOPT(IP_DROP_MEMBERSHIP);
            CASE_GETSOCKOPT_VALUE(ORBIS_NET_IP_TTLCHK, sockopt_ip_ttlchk);
            CASE_GETSOCKOPT_VALUE(ORBIS_NET_IP_MAXTTL, sockopt_ip_maxttl);
        }
    } else if (native_level == IPPROTO_TCP) {
        switch (optname) {
            CASE_GETSOCKOPT(TCP_NODELAY);
            CASE_GETSOCKOPT(TCP_MAXSEG);
            CASE_GETSOCKOPT_VALUE(ORBIS_NET_TCP_MSS_TO_ADVERTISE, sockopt_tcp_mss_to_advertise);
        }
    }
    UNREACHABLE_MSG("Unknown level ={} optname ={}", level, optname);
    return 0;
}

int PosixSocket::GetPeerName(OrbisNetSockaddr* name, u32* namelen) {
    std::scoped_lock lock{m_mutex};
    LOG_DEBUG(Lib_Net, "called");

    sockaddr addr;
    convertOrbisNetSockaddrToPosix(name, &addr);
    if (name != nullptr) {
        *namelen = sizeof(sockaddr_in);
    }
    int res = ::getpeername(sock, &addr, (socklen_t*)namelen);
    if (res >= 0) {
        convertPosixSockaddrToOrbis(&addr, name);
        *namelen = sizeof(OrbisNetSockaddrIn);
    }
    return ConvertReturnErrorCode(res);
}

int PosixSocket::fstat(Libraries::Kernel::OrbisKernelStat* sb) {
#ifdef _WIN32
    LOG_ERROR(Lib_Net, "(STUBBED) called");
    sb->st_mode = 0000777u | 0140000u;
    return 0;
#else
    struct stat st{};
    int result = ::fstat(sock, &st);
    sb->st_mode = 0000777u | 0140000u;
    sb->st_size = st.st_size;
    sb->st_blocks = st.st_blocks;
    sb->st_blksize = st.st_blksize;
    // sb->st_flags = st.st_flags;
    return ConvertReturnErrorCode(result);
#endif
}

} // namespace Libraries::Net