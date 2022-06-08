#pragma once

#include <string>
#include <stdio.h>
#include <unistd.h>

#include <netinet/in.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <string.h>
#include <errno.h>

///
/// @brief Namespace for Network implementation details.
///
namespace hsd::network_detail
{
    ///
    /// @brief Internal alias for the unix socket handle.
    ///
    using unix_socket_handle = int;

    ///
    /// @brief Internal handle for the socket.
    ///
    using native_socket_type = unix_socket_handle;

    ///
    /// @brief Enumeration for the different socket protocols.
    ///
    enum class socket_protocol
    {
        tcp = IPPROTO_TCP, ///< TCP protocol.
        udp = IPPROTO_UDP  ///< UDP protocol.
    };
    ///
    /// @brief Enumeration for the different address families.
    ///
    enum class ip_protocol
    {
        ipv4   = AF_INET,  ///< IPv4 protocol.
        ipv6   = AF_INET6, ///< IPv6 protocol.
        unspec = AF_UNSPEC ///< Unspecified protocol.
    };

    ///
    /// @brief Enumeration for the different socket types, used for POSIX.
    ///
    enum class unix_socket_type
    {
        stream     = SOCK_STREAM,   ///< Stream socket.
        dgram      = SOCK_DGRAM,    ///< Datagram socket.
        raw        = SOCK_RAW,      ///< Raw socket.
    };

    ///
    /// @brief Alias for the socket type enum
    ///
    using socket_type = unix_socket_type;
} // namespace hsd::network_detail