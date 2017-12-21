#ifndef PLATFORM_BILIBILI_HPP
#define PLATFORM_BILIBILI_HPP

#include "platforms/platform_base.hpp"
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/streambuf.hpp>
#include <string>

class platform_bilibili final : public platform_base
{
    using tcp = boost::asio::ip::tcp;
public:
    platform_bilibili(boost::asio::io_service& ios, std::string roomid) : platform_base(ios, "bilibili", roomid),
                                                                          m_live_api_host("live.bilibili.com"),
                                                                          m_live_api_path("/api/player?id=cid:"),
                                                                          m_resolver(ios),
                                                                          m_socket(ios)
    {}
    void start();
    void close(){};
    bool is_room_valid();
private:
    void on_http_resolve(boost::system::error_code ec, tcp::resolver::iterator iter);
    void on_http_connect(boost::system::error_code ec);
    void handle_http_read_status_line(boost::system::error_code ec);
    void handle_http_read_header(boost::system::error_code ec);
    void handle_http_read_content(boost::system::error_code ec);

    void parse_live_msg();

    connection_hdl m_chdl;
    std::string m_live_api_host;
    std::string m_live_api_path;
    tcp::resolver m_resolver;
    tcp::socket m_socket;
    tcp::endpoint m_ep;
    boost::asio::streambuf m_request;
    boost::asio::streambuf m_response;
    std::string m_live_msg;
    std::string m_dm_server;
    std::string m_dm_ws_port;
};

#endif // PLATFORM_BILIBILI_HPP
