// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#ifdef _WIN32
#define _WINSOCK_DEPRECATED_NO_WARNINGS
#include <Ws2tcpip.h>
#include <iphlpapi.h>
#include <winsock2.h>
typedef SOCKET net_socket;
typedef int socklen_t;
#else
#include <cerrno>
#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <unistd.h>
typedef int net_socket;
#endif
#include <map>
#include <memory>
#include <mutex>
#include "common/assert.h"
#include "net.h"
#include "net_error.h"

namespace Libraries::Kernel {
struct OrbisKernelStat;
s32* PS4_SYSV_ABI __Error();
}

namespace Libraries::Kernel {
struct OrbisKernelStat;
}

namespace Libraries::Net {

struct Socket;

typedef std::shared_ptr<Socket> SocketPtr;

int ConvertFamilies(int family);

static int ConvertLevels(int level) {
    switch (level) {
    case ORBIS_NET_SOL_SOCKET:
        return SOL_SOCKET;
    case ORBIS_NET_IPPROTO_IP:
        return IPPROTO_IP;
    case ORBIS_NET_IPPROTO_TCP:
        return IPPROTO_TCP;
    case ORBIS_NET_IPPROTO_UDP:
        return IPPROTO_UDP;
    case ORBIS_NET_IPPROTO_IPV6:
        return IPPROTO_IPV6;
    default:
        UNREACHABLE_MSG("unhandled socket level {}", level);
    }
}

static void convertOrbisNetSockaddrToPosix(const OrbisNetSockaddr* src, sockaddr* dst) {
    if (src == nullptr || dst == nullptr)
        return;
    memset(dst, 0, sizeof(sockaddr));
    const OrbisNetSockaddrIn* src_in = (const OrbisNetSockaddrIn*)src;
    sockaddr_in* dst_in = (sockaddr_in*)dst;
    dst_in->sin_family = src_in->sin_family;
    dst_in->sin_port = src_in->sin_port;
    memcpy(&dst_in->sin_addr, &src_in->sin_addr, 4);
}

static void convertPosixSockaddrToOrbis(sockaddr* src, OrbisNetSockaddr* dst) {
    if (src == nullptr || dst == nullptr)
        return;
    memset(dst, 0, sizeof(OrbisNetSockaddr));
    OrbisNetSockaddrIn* dst_in = (OrbisNetSockaddrIn*)dst;
    sockaddr_in* src_in = (sockaddr_in*)src;
    dst_in->sin_family = static_cast<unsigned char>(src_in->sin_family);
    dst_in->sin_port = src_in->sin_port;
    memcpy(&dst_in->sin_addr, &src_in->sin_addr, 4);
}

#ifdef _WIN32
#define ERROR_CASE(errname)                                                                        \
    case (WSA##errname):                                                                           \
        return ORBIS_NET_ERROR_##errname;
#else
#define ERROR_CASE(errname)                                                                        \
    case (errname):                                                                                \
        *Libraries::Kernel::__Error() = ORBIS_NET_##errname;                                        \
        return -1;
#endif

#ifdef _WIN32
#define ERROR_LITERAL_CASE(errname)                                                                        \
    case (WSA##errname):                                                                           \
        return ORBIS_NET_##errname;
#else
#define ERROR_LITERAL_CASE(errname)                                                                        \
    case (errname):                                                                                \
        return ORBIS_NET_##errname;
#endif

static s64 ConvertLiteralErrorCode(s64 retval) {
    switch (retval) {
    case 0:
        return 0;
#ifndef _WIN32 // These errorcodes don't exist in WinSock
        ERROR_CASE(EPERM)
        ERROR_CASE(ENOENT)
        // ERROR_CASE(ESRCH)
        // ERROR_CASE(EIO)
        // ERROR_CASE(ENXIO)
        // ERROR_CASE(E2BIG)
        // ERROR_CASE(ENOEXEC)
        // ERROR_CASE(EDEADLK)
        ERROR_CASE(ENOMEM)
        // ERROR_CASE(ECHILD)
        // ERROR_CASE(EBUSY)
        ERROR_CASE(EEXIST)
        // ERROR_CASE(EXDEV)
        ERROR_CASE(ENODEV)
        // ERROR_CASE(ENOTDIR)
        // ERROR_CASE(EISDIR)
        ERROR_CASE(ENFILE)
        // ERROR_CASE(ENOTTY)
        // ERROR_CASE(ETXTBSY)
        // ERROR_CASE(EFBIG)
        ERROR_CASE(ENOSPC)
        // ERROR_CASE(ESPIPE)
        // ERROR_CASE(EROFS)
        // ERROR_CASE(EMLINK)
        ERROR_CASE(EPIPE)
        // ERROR_CASE(EDOM)
        // ERROR_CASE(ERANGE)
        // ERROR_CASE(ENOLCK)
        // ERROR_CASE(ENOSYS)
        // ERROR_CASE(EIDRM)
        // ERROR_CASE(EOVERFLOW)
        // ERROR_CASE(EILSEQ)
        // ERROR_CASE(ENOTSUP)
        ERROR_CASE(ECANCELED)
        // ERROR_CASE(EBADMSG)
        ERROR_CASE(ENODATA)
        // ERROR_CASE(ENOSR)
        // ERROR_CASE(ENOSTR)
        // ERROR_CASE(ETIME)
#endif
        ERROR_CASE(EINTR)
        ERROR_CASE(EBADF)
        ERROR_CASE(EACCES)
        ERROR_CASE(EFAULT)
        ERROR_CASE(EINVAL)
        ERROR_CASE(EMFILE)
        ERROR_CASE(EWOULDBLOCK)
        ERROR_CASE(EINPROGRESS)
        ERROR_CASE(EALREADY)
        ERROR_CASE(ENOTSOCK)
        ERROR_CASE(EDESTADDRREQ)
        ERROR_CASE(EMSGSIZE)
        ERROR_CASE(EPROTOTYPE)
        ERROR_CASE(ENOPROTOOPT)
        ERROR_CASE(EPROTONOSUPPORT)
#if defined(__APPLE__) || defined(_WIN32)
        ERROR_CASE(EOPNOTSUPP)
#endif
        ERROR_CASE(EAFNOSUPPORT)
        ERROR_CASE(EADDRINUSE)
        ERROR_CASE(EADDRNOTAVAIL)
        ERROR_CASE(ENETDOWN)
        ERROR_CASE(ENETUNREACH)
        ERROR_CASE(ENETRESET)
        ERROR_CASE(ECONNABORTED)
        ERROR_CASE(ECONNRESET)
        ERROR_CASE(ENOBUFS)
        ERROR_CASE(EISCONN)
        ERROR_CASE(ENOTCONN)
        ERROR_CASE(ETIMEDOUT)
        ERROR_CASE(ECONNREFUSED)
        ERROR_CASE(ELOOP)
        ERROR_CASE(ENAMETOOLONG)
        ERROR_CASE(EHOSTUNREACH)
        ERROR_CASE(ENOTEMPTY)
    }
    UNREACHABLE_MSG("unhandled {}", retval);
}

static s64 ConvertReturnErrorCode(s64 retval) {
    if (retval < 0) {
#ifdef _WIN32
        switch (WSAGetLastError()) {
#else
        switch (errno) {
#endif
#ifndef _WIN32 // These errorcodes don't exist in WinSock
            ERROR_CASE(EPERM)
            ERROR_CASE(ENOENT)
            // ERROR_CASE(ESRCH)
            // ERROR_CASE(EIO)
            // ERROR_CASE(ENXIO)
            // ERROR_CASE(E2BIG)
            // ERROR_CASE(ENOEXEC)
            // ERROR_CASE(EDEADLK)
            ERROR_CASE(ENOMEM)
            // ERROR_CASE(ECHILD)
            // ERROR_CASE(EBUSY)
            ERROR_CASE(EEXIST)
            // ERROR_CASE(EXDEV)
            ERROR_CASE(ENODEV)
            // ERROR_CASE(ENOTDIR)
            // ERROR_CASE(EISDIR)
            ERROR_CASE(ENFILE)
            // ERROR_CASE(ENOTTY)
            // ERROR_CASE(ETXTBSY)
            // ERROR_CASE(EFBIG)
            ERROR_CASE(ENOSPC)
            // ERROR_CASE(ESPIPE)
            // ERROR_CASE(EROFS)
            // ERROR_CASE(EMLINK)
            ERROR_CASE(EPIPE)
            // ERROR_CASE(EDOM)
            // ERROR_CASE(ERANGE)
            // ERROR_CASE(ENOLCK)
            // ERROR_CASE(ENOSYS)
            // ERROR_CASE(EIDRM)
            // ERROR_CASE(EOVERFLOW)
            // ERROR_CASE(EILSEQ)
            // ERROR_CASE(ENOTSUP)
            ERROR_CASE(ECANCELED)
            // ERROR_CASE(EBADMSG)
            ERROR_CASE(ENODATA)
            // ERROR_CASE(ENOSR)
            // ERROR_CASE(ENOSTR)
            // ERROR_CASE(ETIME)
#endif
            ERROR_CASE(EINTR)
            ERROR_CASE(EBADF)
            ERROR_CASE(EACCES)
            ERROR_CASE(EFAULT)
            ERROR_CASE(EINVAL)
            ERROR_CASE(EMFILE)
            ERROR_CASE(EWOULDBLOCK)
            ERROR_CASE(EINPROGRESS)
            ERROR_CASE(EALREADY)
            ERROR_CASE(ENOTSOCK)
            ERROR_CASE(EDESTADDRREQ)
            ERROR_CASE(EMSGSIZE)
            ERROR_CASE(EPROTOTYPE)
            ERROR_CASE(ENOPROTOOPT)
            ERROR_CASE(EPROTONOSUPPORT)
#if defined(__APPLE__) || defined(_WIN32)
            ERROR_CASE(EOPNOTSUPP)
#endif
            ERROR_CASE(EAFNOSUPPORT)
            ERROR_CASE(EADDRINUSE)
            ERROR_CASE(EADDRNOTAVAIL)
            ERROR_CASE(ENETDOWN)
            ERROR_CASE(ENETUNREACH)
            ERROR_CASE(ENETRESET)
            ERROR_CASE(ECONNABORTED)
            ERROR_CASE(ECONNRESET)
            ERROR_CASE(ENOBUFS)
            ERROR_CASE(EISCONN)
            ERROR_CASE(ENOTCONN)
            ERROR_CASE(ETIMEDOUT)
            ERROR_CASE(ECONNREFUSED)
            ERROR_CASE(ELOOP)
            ERROR_CASE(ENAMETOOLONG)
            ERROR_CASE(EHOSTUNREACH)
            ERROR_CASE(ENOTEMPTY)
        }
        *Libraries::Kernel::__Error() = ORBIS_NET_EINTERNAL;
        return -1;
    }
    // if it is 0 or positive return it as it is
    return retval;
}

struct OrbisNetLinger {
    s32 l_onoff;
    s32 l_linger;
};

struct Socket {
    explicit Socket(int domain, int type, int protocol) {}
    virtual ~Socket() = default;
    virtual bool IsValid() const = 0;
    virtual int Close() = 0;
    virtual int SetSocketOptions(int level, int optname, const void* optval, u32 optlen) = 0;
    virtual int GetSocketOptions(int level, int optname, void* optval, u32* optlen) = 0;
    virtual int Bind(const OrbisNetSockaddr* addr, u32 addrlen) = 0;
    virtual int Listen(int backlog) = 0;
    virtual int SendPacket(const void* msg, u32 len, int flags, const OrbisNetSockaddr* to,
                           u32 tolen) = 0;
    virtual SocketPtr Accept(OrbisNetSockaddr* addr, u32* addrlen) = 0;
    virtual s64 ReceivePacket(void* buf, u32 len, int flags, OrbisNetSockaddr* from,
                              u32* fromlen) = 0;
    virtual int Connect(const OrbisNetSockaddr* addr, u32 namelen) = 0;
    virtual int GetSocketAddress(OrbisNetSockaddr* name, u32* namelen) = 0;
    virtual int GetPeerName(OrbisNetSockaddr* addr, u32* namelen) = 0;
    virtual int fstat(Libraries::Kernel::OrbisKernelStat* stat) = 0;
    virtual std::optional<net_socket> Native() = 0;
    std::mutex m_mutex;
    std::mutex receive_mutex;
};

struct PosixSocket : public Socket {
    net_socket sock;
    int sockopt_so_connecttimeo = 0;
    int sockopt_so_reuseport = 0;
    int sockopt_so_onesbcast = 0;
    int sockopt_so_usecrypto = 0;
    int sockopt_so_usesignature = 0;
    int sockopt_so_nbio = 0;
    int sockopt_ip_ttlchk = 0;
    int sockopt_ip_maxttl = 0;
    int sockopt_tcp_mss_to_advertise = 0;
    int socket_type;
    explicit PosixSocket(int domain, int type, int protocol);
    explicit PosixSocket(net_socket sock) : Socket(0, 0, 0), sock(sock) {}
    bool IsValid() const override;
    int Close() override;
    int SetSocketOptions(int level, int optname, const void* optval, u32 optlen) override;
    int GetSocketOptions(int level, int optname, void* optval, u32* optlen) override;
    int Bind(const OrbisNetSockaddr* addr, u32 addrlen) override;
    int Listen(int backlog) override;
    int SendPacket(const void* msg, u32 len, int flags, const OrbisNetSockaddr* to,
                   u32 tolen) override;
    s64 ReceivePacket(void* buf, u32 len, int flags, OrbisNetSockaddr* from, u32* fromlen) override;
    SocketPtr Accept(OrbisNetSockaddr* addr, u32* addrlen) override;
    int Connect(const OrbisNetSockaddr* addr, u32 namelen) override;
    int GetSocketAddress(OrbisNetSockaddr* name, u32* namelen) override;
    int GetPeerName(OrbisNetSockaddr* addr, u32* namelen) override;
    int fstat(Libraries::Kernel::OrbisKernelStat* stat) override;
    std::optional<net_socket> Native() override {
        return sock;
    }
};

struct P2PSocket : public Socket {
    net_socket sock;
    int sockopt_so_reuseport = 0;
    int sockopt_so_onesbcast = 0;
    int sockopt_so_usecrypto = 0;
    int sockopt_so_usesignature = 0;
    int sockopt_so_nbio = 0;
    int sockopt_ip_ttlchk = 0;
    int sockopt_ip_maxttl = 0;
    int sockopt_tcp_mss_to_advertise = 0;
    int socket_type;
    explicit P2PSocket(int domain, int type, int protocol);
    bool IsValid() const override {
        return true;
    }
    int Close() override;
    int SetSocketOptions(int level, int optname, const void* optval, u32 optlen) override;
    int GetSocketOptions(int level, int optname, void* optval, u32* optlen) override;
    int Bind(const OrbisNetSockaddr* addr, u32 addrlen) override;
    int Listen(int backlog) override;
    int SendPacket(const void* msg, u32 len, int flags, const OrbisNetSockaddr* to,
                   u32 tolen) override;
    s64 ReceivePacket(void* buf, u32 len, int flags, OrbisNetSockaddr* from, u32* fromlen) override;
    SocketPtr Accept(OrbisNetSockaddr* addr, u32* addrlen) override;
    int Connect(const OrbisNetSockaddr* addr, u32 namelen) override;
    int GetSocketAddress(OrbisNetSockaddr* name, u32* namelen) override;
    int GetPeerName(OrbisNetSockaddr* addr, u32* namelen) override;
    int fstat(Libraries::Kernel::OrbisKernelStat* stat) override;
    std::optional<net_socket> Native() override {
        return {};
    }
};

} // namespace Libraries::Net