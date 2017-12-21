#ifndef PLATFORM_BILIBILI_HPP
#define PLATFORM_BILIBILI_HPP

#include "platforms/platform_base.hpp"
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/streambuf.hpp>
#include <string>
#include <random>

class platform_bilibili final : public platform_base
{
    using tcp = boost::asio::ip::tcp;
public:
    platform_bilibili(boost::asio::io_service& ios, std::string roomid) : platform_base(ios, "bilibili", roomid),
                                                                          m_live_api_host("live.bilibili.com"),
                                                                          m_live_api_path("/api/player?id=cid:"),
                                                                          m_resolver(ios),
                                                                          m_socket(ios),
                                                                          m_dist(0.0, 1.0),
                                                                          m_hb_timer(ios)
    {
        m_rd_gen = std::default_random_engine(m_rd());
    }
    void start();
    void close();
    bool is_room_valid();
private:
    void on_http_resolve(boost::system::error_code ec, tcp::resolver::iterator iter);
    void on_http_connect(boost::system::error_code ec);
    void handle_http_read_status_line(boost::system::error_code ec);
    void handle_http_read_header(boost::system::error_code ec);
    void handle_http_read_content(boost::system::error_code ec);
    void parse_live_msg();

    void on_tcp_resolve(boost::system::error_code ec, tcp::resolver::iterator iter);
    void on_tcp_connect(boost::system::error_code ec);
    void do_write(std::vector<uint8_t> *packet);
    void do_read_header();
    void do_read_data(uint32_t data_size);
    void handle_header(std::vector<uint8_t> *header, boost::system::error_code ec, size_t size);
    void handle_data(std::vector<uint8_t> *data, boost::system::error_code ec, size_t size);
    void handle_heartbeat_timer(boost::system::error_code ec);

    std::vector<uint8_t>* new_request_packet();
    std::vector<uint8_t>* new_heartbeat_packet();


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
    std::string m_dm_tcp_port;
    std::random_device m_rd;
    std::default_random_engine m_rd_gen;
    std::uniform_real_distribution<double> m_dist;
    boost::asio::deadline_timer m_hb_timer;
};

#endif // PLATFORM_BILIBILI_HPP
