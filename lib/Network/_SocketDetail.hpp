#pragma once

#include "_NetworkFuncs.hpp"
#include <list>
#include <vector>

namespace hsd::network_detail
{
    ///
    /// @brief TCP socket handler implementation
    /// 
    /// @tparam Protocol - protocol type
    ///
    template <ip_protocol Protocol>
    class tcp_socket
    {
    private:
        /// Data type for socket handle
        pollfd _data;

        ///
        /// @brief Creates a server socket using the given address
        /// 
        /// @param info - address info used to create the socket
        /// @return true - socket created successfully
        /// @return false - socket creation failed
        ///
        inline bool _handle_server(const addrinfo* const info)
        {
            if (info->ai_socktype == SOCK_STREAM)
            {   
                if (is_valid() == false)
                {
                    return false;
                }
                
                _data.events = POLLIN;
                _data.revents = 0;
                
                if (::bind(_data.fd, info->ai_addr, info->ai_addrlen) == -1)
                {
                    close();
                    return false;
                }

                static int _on = 1;
                auto _rc = setsockopt(
                    _data.fd, SOL_SOCKET, 
                    SO_REUSEADDR,
                    &_on, sizeof(_on)
                );
                
                if (_rc < 0)
                {
                    perror("setsockopt() failed");
                    close(); abort();
                }
            
                _rc = ioctl(_data.fd, FIONBIO, &_on);

                if (_rc < 0)
                {
                    perror("ioctl() failed");
                    close(); abort();
                }

                if (::listen(_data.fd, 128) == -1)
                {
                    return false;
                }

                return true;
            }

            return false;
        }

        ///
        /// @brief Creates a client socket using the given address
        /// 
        /// @param info - address info used to create the socket
        /// @return true - socket created successfully
        /// @return false - socket creation failed
        ///
        inline bool _handle_client(const addrinfo* const info)
        {
            if (info->ai_socktype == SOCK_STREAM)
            {
                if (is_valid() == false)
                {
                    return false;
                }

                _data.events = POLLOUT;
                _data.revents = POLLOUT | POLLIN;
                
                if (::connect(_data.fd, info->ai_addr, info->ai_addrlen) == -1)
                {
                    close();
                    return false;
                }

                return true;
            }

            return false;
        }

    public:
        ///
        /// @brief Deleted copy constructor for TCP socket
        ///
        inline tcp_socket(const tcp_socket&) = delete;

        ///
        /// @brief Deleted copy assignment operator for TCP socket
        ///
        inline tcp_socket& operator=(const tcp_socket&) = delete;
        
        ///
        /// @brief Construct a new tcp socket object
        /// 
        /// @param data - socket data conaining the socket
        /// handle and events type to be monitored
        ///
        inline tcp_socket(pollfd&& data)
            : _data{data}
        {}

        ///
        /// @brief Construct by move a new tcp socket object
        /// 
        /// @param other - socket class to be moved
        ///
        inline tcp_socket(tcp_socket&& other)
            : _data{std::move(other._data)}
        {
            other._data.fd = static_cast<network_detail::native_socket_type>(-1);
        }

        ///
        /// @brief Construct a new tcp socket
        /// object for the given address
        /// 
        /// @param addr - address to be used to create the socket
        /// @param is_server - true if the socket is a server socket
        ///
        inline tcp_socket(const char* const addr, bool is_server = false)
        {
            _data = pollfd {
                .fd = static_cast<network_detail::native_socket_type>(-1),
                .events = 0,
                .revents = 0
            };

            if (switch_to(addr, is_server) == false)
            {
                Serial.println(
                    "hsd::network_detail::"
                    "tcp_socket::switch_to()"
                    " failed.\n"
                );
                close();
            }
        }

        ///
        /// @brief Destroy the tcp socket object
        ///
        inline ~tcp_socket()
        {
            close();
        }

        inline tcp_socket& operator=(tcp_socket&& other)
        {
            close();
            _data = other._data;
            other._data.fd = static_cast<network_detail::native_socket_type>(-1);
            return *this;
        }

        inline tcp_socket& operator=(pollfd&& rhs)
        {
            close();
            _data = rhs;
            return *this;
        }

        inline bool operator==(const tcp_socket& rhs) const
        {
            return _data.fd == rhs._data.fd;
        }

        inline bool operator!=(const tcp_socket& rhs) const
        {
            return _data.fd != rhs._data.fd;
        }

        inline void close()
        {
            if (is_valid() == true)
            {
                close_socket(_data.fd);
                _data.fd = static_cast<native_socket_type>(-1);
                _data.events = _data.revents = 0;
            }
        }

        inline bool handle(const addrinfo* const info, bool is_server)
        {
            if (is_valid() == false)
            {
                fputs("tcp_socket::handle() called on invalid socket\n", stderr);
                return false;
            } 
            else if (is_server == true)
            {
                return _handle_server(info);
            }
            else
            {
                return _handle_client(info);
            }
        }

        [[nodiscard]] inline bool switch_to(const char* const addr, bool is_server = false)
        {
            return hsd::network_detail::switch_to(*this, addr, is_server);
        }

        inline bool is_valid() const
        {
            return _data.fd != static_cast<native_socket_type>(-1);
        }

        inline bool is_readable() const
        {
            return _data.revents & POLLIN;
        }

        inline bool is_writable() const
        {
            return _data.revents & POLLOUT;
        }

        static constexpr auto ip_protocol()
        {
            return static_cast<int>(Protocol);
        }

        static constexpr auto sock_type()
        {
            return static_cast<int>(socket_type::stream);
        }

        static constexpr auto net_protocol()
        {
            return static_cast<int>(socket_protocol::tcp);
        }

        inline auto fd() const
        {
            return _data.fd;
        }

        inline long send(const void* const buffer, const std::size_t size)
        {
            if (is_valid() == false || size > 65635)
            {
                return -1;
            }
            
            return ::send(_data.fd, buffer, size, 0);
        }

        inline long receive(void* const buffer, const std::size_t size)
        {
            if (is_valid() == false)
            {
                return -1;
            }
            else if (is_readable() == true)
            {
                return ::recv(_data.fd, buffer, size, 0);
            }

            return 0;
        }
    };

    template <ip_protocol Protocol>
    class tcp_child_socket
        : private tcp_socket<Protocol>
    {
    public:
        using tcp_socket<Protocol>::tcp_socket;

        inline tcp_child_socket(pollfd&& data)
            : tcp_socket<Protocol>{std::move(data)}
        {}

        inline tcp_child_socket(tcp_child_socket&& other)
            : tcp_socket<Protocol>{std::move(other)}
        {}

        inline tcp_child_socket(const char* const addr, bool is_server = false)
            : tcp_socket<Protocol>{addr, is_server}
        {}

        inline tcp_child_socket& operator=(tcp_child_socket&& other)
        {
            static_cast<tcp_socket<Protocol>&>(*this) = std::move(other);
            return *this;
        }

        inline tcp_child_socket& operator=(pollfd&& rhs)
        {
            static_cast<tcp_socket<Protocol>&>(*this) = std::move(rhs);
            return *this;
        }

        using tcp_socket<Protocol>::is_valid;
        using tcp_socket<Protocol>::is_readable;
        using tcp_socket<Protocol>::is_writable;
        using tcp_socket<Protocol>::fd;
        using tcp_socket<Protocol>::ip_protocol;
        using tcp_socket<Protocol>::sock_type;
        using tcp_socket<Protocol>::net_protocol;
        using tcp_socket<Protocol>::send;
        using tcp_socket<Protocol>::receive;
    };

    template <ip_protocol Protocol>
    class udp_socket
    {
    private:
        native_socket_type _data;
        sockaddr_storage _addr;
        socklen_t _addr_len;

        inline bool _handle_server(const addrinfo* const info)
        {
            if (info->ai_socktype == SOCK_DGRAM)
            {
                if (is_valid() == false)
                {
                    return false;
                }

                if (::bind(_data, info->ai_addr, info->ai_addrlen) == -1)
                {
                    close();
                    return false;
                }

                memcpy(&_addr, info->ai_addr, info->ai_addrlen);
                _addr_len = info->ai_addrlen;

                static int _on = 1;
                auto _rc = setsockopt(
                    _data, SOL_SOCKET, 
                    SO_REUSEADDR,
                    &_on, sizeof(_on)
                );
                
                if (_rc < 0)
                {
                    perror("setsockopt() failed");
                    close(); abort();
                }

                _rc = ioctl(_data, FIONBIO, &_on);

                if (_rc < 0)
                {
                    perror("ioctl() failed");
                    close(); abort();
                }

                return true;
            }

            return false;
        }

        inline bool _handle_client(const addrinfo* const info)
        {
            if (info->ai_socktype == SOCK_DGRAM)
            {
                if (is_valid() == false)
                {
                    return false;
                }

                if (::connect(_data, info->ai_addr, info->ai_addrlen) == -1)
                {
                    close();
                    return false;
                }

                memcpy(&_addr, info->ai_addr, info->ai_addrlen);
                _addr_len = info->ai_addrlen;
                return true;
            }

            return false;
        }

    public:
        inline udp_socket(const udp_socket& other) = delete;
        inline udp_socket& operator=(const udp_socket& other) = delete;

        inline udp_socket(const char* const addr, bool is_server = false)
        {
            _data = static_cast<network_detail::native_socket_type>(-1);

            if (switch_to(addr, is_server) == false)
            {
                fputs("hsd::network_detail::udp_socket::switch_to() failed.\n", stderr);
                close();
            }
        }

        inline udp_socket(native_socket_type data, 
            const sockaddr_storage& info, socklen_t addr_len)
            : _data{data}, _addr{info}, _addr_len{addr_len}
        {}

        inline udp_socket(udp_socket&& other)
            : _data{other._data}, _addr{std::move(other._addr)}, _addr_len{other._addr_len}
        {
            other._addr_len = 0;
            other._data = static_cast<network_detail::native_socket_type>(-1);
        }

        inline ~udp_socket()
        {
            close();
        }

        inline udp_socket& operator=(pollfd&& rhs)
        {
            close();
            _data = rhs.fd;
            return *this;
        }

        inline udp_socket& operator=(udp_socket&& rhs)
        {
            close();
            _data = rhs._data;
            _addr_len = rhs._addr_len;
            memcpy(&_addr, &rhs._addr, sizeof(sockaddr_storage));
            rhs._data = static_cast<network_detail::native_socket_type>(-1);
            rhs._addr_len = 0;
            return *this;
        }

        inline bool operator==(const udp_socket& rhs) const
        {
            return memcmp(&_addr, &rhs._addr, sizeof(sockaddr_storage)) == 0;
        }

        inline bool operator!=(const udp_socket& rhs) const
        {
            return memcmp(&_addr, &rhs._addr, sizeof(sockaddr_storage)) != 0;
        }

        inline void close()
        {
            if (is_valid() == true)
            {
                close_socket(_data);
                _data = static_cast<native_socket_type>(-1);
            }
        }

        inline bool handle(const addrinfo* const info, bool is_server)
        {
            if (is_valid() == false)
            {
                fputs("udp_socket::handle() called on invalid socket\n", stderr);
                return false;
            }
            else if (is_server == true)
            {
                return _handle_server(info);
            }
            else
            {
                return _handle_client(info);
            }
        }

        inline bool is_valid() const
        {
            return _data != static_cast<native_socket_type>(-1);
        }

        static constexpr auto ip_protocol()
        {
            return static_cast<int>(Protocol);
        }

        static constexpr auto sock_type()
        {
            return static_cast<int>(socket_type::dgram);
        }

        static constexpr auto net_protocol()
        {
            return static_cast<int>(socket_protocol::udp);
        }

        inline auto fd()
        {
            return _data;
        }

        inline auto& addr()
        {
            return _addr;
        }

        inline auto& length()
        {
            return _addr_len;
        }

        inline bool switch_to(const char* const addr, bool is_server = false)
        {
            return hsd::network_detail::switch_to(*this, addr, is_server);
        }

        inline long send(const void* const buffer, const std::size_t size)
        {
            if (is_valid() == false)
            {
                return -1;
            }
                
            return ::sendto(
                _data, buffer, size, 0, 
                reinterpret_cast<sockaddr*>(&_addr), 
                _addr_len
            );
        }

        inline long receive(void* const buffer, const std::size_t size)
        {
            if (is_valid() == false)
            {
                return -1;
            }

            return ::recvfrom(
                _data, buffer, size, 0,
                reinterpret_cast<sockaddr*>(&_addr), &_addr_len
            );
        }
    };

    template <ip_protocol Protocol>
    struct udp_child_socket
    {
    private:
        native_socket_type _data;
        sockaddr_storage _addr{};
        socklen_t _addr_len{};

        std::list<std::array<char, 1024>> _recv_buffer{};

    public:
        inline udp_child_socket() 
            : _data{static_cast<native_socket_type>(-1)}
        {}

        inline udp_child_socket(native_socket_type data,
            const sockaddr_storage& info, socklen_t addr_len)
            : _data{data}, _addr_len{addr_len}
        {
            memcpy(&_addr, &info, sizeof(sockaddr_storage));
        }

        inline udp_child_socket(udp_child_socket&& other)
            : _data{other._data}, _addr_len{other._addr_len}, 
            _recv_buffer{std::move(other._recv_buffer)}
        {
            memcpy(&_addr, &other._addr, sizeof(sockaddr_storage));
        }

        inline udp_child_socket& operator=(udp_child_socket&& rhs)
        {
            _data = rhs._data;
            _addr_len = rhs._addr_len;
            memcpy(&_addr, &rhs._addr, sizeof(sockaddr_storage));
            _recv_buffer = std::move(rhs._recv_buffer);

            return *this;
        }

        inline bool operator==(const udp_child_socket& rhs) const
        {
            auto _res = memcmp(&_addr, &rhs._addr, sizeof(sockaddr_storage));
            return _res == 0 && rhs._data == _data;
        }

        inline bool operator!=(const udp_child_socket& rhs) const
        {
            return !(*this == rhs);
        }

        inline bool is_valid() const
        {
            return _data != static_cast<native_socket_type>(-1);
        }

        inline void add(const std::array<char, 1024>& buffer)
        {
            _recv_buffer.push_back(buffer);
        }

        inline long send(const void* const buffer, const std::size_t size)
        {
            if (is_valid() == false)
            {
                return -1;
            }

            return ::sendto(
                _data, reinterpret_cast<const char*>(buffer),
                static_cast<int>(size), 0,
                reinterpret_cast<const sockaddr*>(&_addr), _addr_len
            );
        }

        inline long receive(void* const buffer, const std::size_t size)
        {
            if (is_valid() == false || size < 1024)
            {
                return -1;
            }
            else if (_recv_buffer.empty())
            {
                return 0;
            }

            auto _min_size = size < 1024 ? size : 1024;
            memcpy(buffer, _recv_buffer.front().data(), _min_size);
            _recv_buffer.pop_front();
            return _min_size;
        }
    };
} // namespace hsd::network_detail