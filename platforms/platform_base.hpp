#ifndef PLATFORM_BASE_HPP
#define PLATFORM_BASE_HPP

#include <utility>
#include <tuple>
#include <string>
#include <vector>
#include <cstdint>
#include <functional>
#include <memory>
#include <set>
#include "network/dmp_cs.hpp"
#include "utils/rw_lock.hpp"


class platform_base : public std::enable_shared_from_this<platform_base>
{
    typedef std::function<void (std::string roomstr)> close_callback_t;
    using error_code = websocketpp::lib::error_code;
public:
    platform_base(boost::asio::io_service &ios, std::string platform_name) : m_close_callback(nullptr),
                                                                             m_ios(ios),
                                                                             m_platform_name(platform_name)
    {    }
    void add_listener(connection_hdl conn_hdl);
    void erase_listener(connection_hdl conn_hdl);
    bool have_listener(connection_hdl conn_hdl);
    size_t listeners_count();
    void set_close_callback(close_callback_t cb);
    virtual void start(std::string roomid) = 0;
    virtual void close() = 0;
    virtual bool is_room_valid(std::string roomid) = 0;
    virtual ~platform_base();
private:
    std::set<connection_hdl, std::owner_less<connection_hdl>> m_listeners;
    rw_mutex_t m_lmtx;
protected:
    void publish(std::string dm_msg);
    close_callback_t m_close_callback;
    boost::asio::io_service &m_ios;
    const std::string m_platform_name;
    std::string m_roomid;

};

typedef std::shared_ptr<platform_base> platform_base_ptr_t;

typedef std::function<platform_base_ptr_t(boost::asio::io_service&)> platform_creator_t;

#endif // PLATFORM_BASE_HPP
