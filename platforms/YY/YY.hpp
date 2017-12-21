#ifndef PLATFORM_YY_HPP
#define PLATFORM_YY_HPP

#include "platforms/platform_base.hpp"
#include "network/dmp_cs.hpp"
#include <string>
#include <map>

class platform_YY final : public platform_base
{
public:
    platform_YY(boost::asio::io_service& ios, std::string roomid) : platform_base(ios, "YY", roomid),
                                                                    m_ws_url("ws://tvgw.yy.com:26101/websocket")
    {}
    void start();
    void close();
    bool is_room_valid();

private:

    void on_client_open(connection_hdl hdl);
    void on_client_fail(connection_hdl hdl);
    void on_client_message(connection_hdl hdl, message_ptr msg);
    void on_client_close(connection_hdl hdl);
    void handle_heartbeat(std::error_code ec);

    std::string new_login_packet();
    std::string new_init_packet();
    std::string m_ws_url;
    connection_hdl m_hdl;
    std::map<std::string, std::string> m_login_info;
    std::map<std::string, std::string> m_init_info;
    dmp_client::timer_ptr m_hb_timer;
    std::map<std::string, std::string> m_hb_info;
};

#endif // PLATFORM_YY_HPP
