#include "bilibili.hpp"
#include <istream>
#include <ostream>
#include <string>
#include <sstream>
#include <random>
#include <boost/property_tree/xml_parser.hpp>
#include <boost/asio/detail/socket_ops.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <sstream>
#include "json11/json11.hpp"

namespace property_tree = boost::property_tree;

bool platform_bilibili::is_room_valid()
{
    return true;
}

void platform_bilibili::start()
{
    std::ostream req_stream(&m_request);

    req_stream << "GET " << m_live_api_path << m_roomid << " HTTP/1.0\r\n";
    req_stream << "Host: " << m_live_api_host << "\r\n";
    req_stream << "Accept: */*\r\n";
    req_stream << "Connection: close\r\n\r\n";

    tcp::resolver::query query(m_live_api_host, "http");
    m_resolver.async_resolve(query, std::bind(&platform_bilibili::on_http_resolve,
                                              std::dynamic_pointer_cast<platform_bilibili>(shared_from_this()),
                                              std::placeholders::_1,
                                              std::placeholders::_2));
}

void platform_bilibili::close()
{
    boost::system::error_code ec;
    m_socket.close(ec);
    m_hb_timer.cancel(ec);
}

void platform_bilibili::on_http_resolve(boost::system::error_code ec, tcp::resolver::iterator iter)
{
    if (CLIENT_REPORT_WHEN_ERROR(ec)) {
        return;
    }
    m_ep = *iter;
    m_socket.async_connect(m_ep, std::bind(&platform_bilibili::on_http_connect,
                                           std::dynamic_pointer_cast<platform_bilibili>(shared_from_this()),
                                           std::placeholders::_1));
}

void platform_bilibili::on_http_connect(boost::system::error_code ec)
{
    if (CLIENT_REPORT_WHEN_ERROR(ec)) {
        return;
    }
    auto self = shared_from_this();
    boost::asio::async_write(m_socket,
                             m_request,
                             [this, self](boost::system::error_code ec, size_t size)
                             {
                                 (void)size;
                                 if (CLIENT_REPORT_WHEN_ERROR(ec)) {
                                     m_socket.close(ec);
                                     return;
                                 }
                                 boost::asio::async_read_until(m_socket,
                                                               m_response,
                                                               "\r\n",
                                                               std::bind(&platform_bilibili::handle_http_read_status_line,
                                                                         std::dynamic_pointer_cast<platform_bilibili>(shared_from_this()),
                                                                         std::placeholders::_1));
                             });
}

void platform_bilibili::handle_http_read_status_line(boost::system::error_code ec)
{
    if (CLIENT_REPORT_WHEN_ERROR(ec)) {
        m_socket.close(ec);
        return;
    }
    std::istream res_stream(&m_response);
    std::string http_version;
    res_stream >> http_version;
    unsigned status_code;
    res_stream >> status_code;
    std::string status_message;
    std::getline(res_stream, status_message);
    if (!res_stream || http_version.substr(0, 5) != "HTTP/") {
        ec.assign(boost::system::errc::bad_message, boost::system::system_category());
        CLIENT_REPORT_WHEN_ERROR(ec);
        return;
    }
    if (status_code != 200) {
        ec.assign(boost::system::errc::bad_message, boost::system::system_category());
        CLIENT_REPORT_WHEN_ERROR(ec);
        return;
    }
    boost::asio::async_read_until(m_socket,
                                  m_response,
                                  "\r\n\r\n",
                                  std::bind(&platform_bilibili::handle_http_read_header,
                                            std::dynamic_pointer_cast<platform_bilibili>(shared_from_this()),
                                            std::placeholders::_1));
}

void platform_bilibili::handle_http_read_header(boost::system::error_code ec)
{
    if (CLIENT_REPORT_WHEN_ERROR(ec)) {
        m_socket.close(ec);
        return;
    }
//no use
//    std::istream res_stream(&m_response);
//    std::string header;
    //just clear
    m_response.consume(m_response.size());
    boost::asio::async_read(m_socket,
                            m_response,
                            boost::asio::transfer_at_least(1),
                            std::bind(&platform_bilibili::handle_http_read_content,
                                      std::dynamic_pointer_cast<platform_bilibili>(shared_from_this()),
                                      std::placeholders::_1));
}

void platform_bilibili::handle_http_read_content(boost::system::error_code ec)
{
    if (ec == boost::asio::error::eof) {
        m_socket.close(ec);
        if (CLIENT_REPORT_WHEN_ERROR(ec)) {
            return;
        }
        parse_live_msg();
        return;
    }
    if (CLIENT_REPORT_WHEN_ERROR(ec)) {
        m_socket.close(ec);
        return;
    }
    auto data = m_response.data();
    m_live_msg.append(std::string(boost::asio::buffers_begin(data), boost::asio::buffers_end(data)));
    boost::asio::async_read(m_socket,
                            m_response,
                            boost::asio::transfer_at_least(1),
                            std::bind(&platform_bilibili::handle_http_read_content,
                                      std::dynamic_pointer_cast<platform_bilibili>(shared_from_this()),
                                      std::placeholders::_1));
}

void platform_bilibili::parse_live_msg()
{
    boost::system::error_code ec;

    property_tree::ptree tree;
    std::istringstream iss_live_msg(m_live_msg);
    property_tree::read_xml(iss_live_msg, tree);
    std::string state = tree.get<std::string>("state");
    if (state != "LIVE") {
        ec.assign(boost::system::errc::bad_message, boost::system::system_category());
        CLIENT_REPORT_WHEN_ERROR(ec);
        m_socket.close(ec);
        return;
    }
    m_dm_server = tree.get<std::string>("server");
    //what dm_server_list means??
    m_dm_tcp_port = tree.get<std::string>("dm_port");
    m_dm_ws_port = tree.get<std::string>("dm_ws_port");
    m_socket.close(ec);
    tcp::resolver::query query(m_dm_server, m_dm_tcp_port);
    m_resolver.async_resolve(query, std::bind(&platform_bilibili::on_tcp_resolve,
                                              std::dynamic_pointer_cast<platform_bilibili>(shared_from_this()),
                                              std::placeholders::_1,
                                              std::placeholders::_2));
}

std::vector<uint8_t>* platform_bilibili::new_request_packet()
{
    uint64_t uid;
    uid = 1e14;
    uid += (2e14 * m_dist(m_rd_gen));
//    int uid = 1e8 * m_dist(m_rd_gen);
//    boost::property_tree::ptree tree;
//    tree.put("roomid", std::stoul(m_roomid));
//    tree.put("uid", uid);

//    std::string req_data = json11::Json({
//                                            {"roomid", std::stoi(m_roomid)},
//                                            {"uid", uid}
//                                        }).dump();
//    std::stringstream req_stream;
//    boost::property_tree::write_json(req_stream, tree);
//    std::string req_data = req_stream.str();
    std::string req_data;
    req_data.append("{\"roomid\":").append(m_roomid).append(", \"uid\":").append(std::to_string(uid)).append("}");
    auto packet = new std::vector<uint8_t>(4 + 4 + 4 + 4 + req_data.length());
    uint8_t *packet_buf = packet->data();
    *(uint32_t *)packet_buf = boost::asio::detail::socket_ops::host_to_network_long(packet->size());
    packet_buf += 4;
    *packet_buf = 0;
    packet_buf += 1;
    *packet_buf = 16;
    packet_buf += 1;
    *packet_buf = 0;
    packet_buf += 1;
    *packet_buf = 1;
    packet_buf += 1;
    *(uint32_t *)packet_buf = boost::asio::detail::socket_ops::host_to_network_long(7);
    packet_buf += 4;
    *packet_buf = 0;
    packet_buf += 1;
    *packet_buf = 0;
    packet_buf += 1;
    *packet_buf = 0;
    packet_buf += 1;
    *packet_buf = 1;
    packet_buf += 1;
    memcpy(packet_buf, req_data.data(), req_data.length());
    return packet;
}

std::vector<uint8_t>* platform_bilibili::new_heartbeat_packet()
{
    auto packet = new std::vector<uint8_t>(16);
    uint8_t *packet_buf = packet->data();
    *(uint32_t *)packet_buf = boost::asio::detail::socket_ops::host_to_network_long(16);
    packet_buf += 4;
    *packet_buf = 0;
    packet_buf += 1;
    *packet_buf = 16;
    packet_buf += 1;
    *packet_buf = 0;
    packet_buf += 1;
    *packet_buf = 1;
    packet_buf += 1;
    *(uint32_t *)packet_buf = boost::asio::detail::socket_ops::host_to_network_long(2);
    packet_buf += 4;
    *packet_buf = 0;
    packet_buf += 1;
    *packet_buf = 0;
    packet_buf += 1;
    *packet_buf = 0;
    packet_buf += 1;
    *packet_buf = 1;
    packet_buf += 1;
    return packet;
}

void platform_bilibili::on_tcp_resolve(boost::system::error_code ec, tcp::resolver::iterator iter)
{
    if (CLIENT_REPORT_WHEN_ERROR(ec)) {
        return;
    }
    m_ep = *iter;
    m_socket.async_connect(m_ep, std::bind(&platform_bilibili::on_tcp_connect,
                                           std::dynamic_pointer_cast<platform_bilibili>(shared_from_this()),
                                           std::placeholders::_1));
}

void platform_bilibili::on_tcp_connect(boost::system::error_code ec)
{
    if (CLIENT_REPORT_WHEN_ERROR(ec)) {
        return;
    }
    do_write(new_request_packet());
    do_read_header();
    m_hb_timer.expires_from_now(boost::posix_time::seconds(1));
    m_hb_timer.async_wait(std::bind(&platform_bilibili::handle_heartbeat_timer,
                                    std::dynamic_pointer_cast<platform_bilibili>(shared_from_this()),
                                    std::placeholders::_1));
}

void platform_bilibili::do_write(std::vector<uint8_t> *packet)
{
    //see this:
    //https://stackoverflow.com/questions/19368012/whats-the-reason-of-using-auto-selfshared-from-this-variable-in-lambda-func
    auto self = shared_from_this();
    boost::asio::async_write(m_socket,
                             boost::asio::buffer(packet->data(), packet->size()),
                             [this, self, packet](boost::system::error_code ec, std::size_t /*length*/)
                             {
                                 if (CLIENT_REPORT_WHEN_ERROR(ec)) {
                                     m_socket.close(ec);
                                     m_hb_timer.cancel(ec);
                                 }
                                 delete packet;
                             });
}

void platform_bilibili::do_read_header()
{
    std::vector<uint8_t> *header = new std::vector<uint8_t>(16);
    boost::asio::async_read(m_socket,
                            boost::asio::buffer(header->data(), 16),
                            std::bind(&platform_bilibili::handle_header,
                                      std::dynamic_pointer_cast<platform_bilibili>(shared_from_this()),
                                      header,
                                      std::placeholders::_1,
                                      std::placeholders::_2));
}

void platform_bilibili::do_read_data(uint32_t data_size)
{
    std::vector<uint8_t> *data = new std::vector<uint8_t>(data_size);
    boost::asio::async_read(m_socket,
                            boost::asio::buffer(data->data(), data_size),
                            std::bind(&platform_bilibili::handle_data,
                                      std::dynamic_pointer_cast<platform_bilibili>(shared_from_this()),
                                      data,
                                      std::placeholders::_1,
                                      std::placeholders::_2));
}

void platform_bilibili::handle_header(std::vector<uint8_t> *header, boost::system::error_code ec, size_t size)
{
    (void)size;
    if (CLIENT_REPORT_WHEN_ERROR(ec)) {
        m_socket.close(ec);
        m_hb_timer.cancel(ec);
        delete header;
        return;
    }
    uint32_t *data_buf = (uint32_t *)(header->data());
    uint32_t data_len = boost::asio::detail::socket_ops::network_to_host_long(*data_buf);
    uint8_t data_type = *(((uint8_t *)(data_buf)) + 11);
    if (data_len <= 16) {
        do_read_header();
    } else if (data_len > 16 && data_type == 5) {
        do_read_data(data_len - 16);
    } else {
        auto tmpbuf = new std::vector<uint8_t>(data_len - 16);
        auto self = shared_from_this();
        boost::asio::async_read(m_socket,
                                boost::asio::buffer(tmpbuf->data(), tmpbuf->size()),
                                [tmpbuf, this, self](boost::system::error_code ec, size_t size)
                                {
                                    (void)size;
                                    delete tmpbuf;
                                    if (CLIENT_REPORT_WHEN_ERROR(ec)) {
                                        m_socket.close(ec);
                                        m_hb_timer.cancel(ec);
                                        return;
                                    }
                                    do_read_header();
                                });
    }
    delete header;
}

void platform_bilibili::handle_data(std::vector<uint8_t> *data, boost::system::error_code ec, size_t size)
{
    do_read_header();
    (void)size;
    if (CLIENT_REPORT_WHEN_ERROR(ec)) {
        m_socket.close(ec);
        m_hb_timer.cancel(ec);
        delete data;
        return;
    }
    data->push_back('\0');
    const char* msg_ptr = reinterpret_cast<const char*>(data->data());
    std::string err;
    auto msg_json = json11::Json::parse(msg_ptr, err);
    if (!err.empty()) {
        ec.assign(boost::system::errc::bad_message, boost::system::system_category());
        CLIENT_REPORT_WHEN_ERROR(ec);
        m_socket.close(ec);
        m_hb_timer.cancel(ec);
        delete data;
        return;
    }
    std::string cmd = msg_json["cmd"].string_value();
    if (cmd != "DANMU_MSG") {
        delete data;
        return;
    }
    auto info_array = msg_json["info"].array_items();
    std::string text = info_array[1].string_value();
    std::string uid = info_array[2].array_items()[0].string_value();
    std::string nickname = info_array[2].array_items()[1].string_value();
    publish_json(uid, nickname, text);
    delete data;
}

void platform_bilibili::handle_heartbeat_timer(boost::system::error_code ec)
{
    if (CLIENT_REPORT_WHEN_ERROR(ec)) {
        m_socket.close(ec);
        return;
    }
    do_write(new_heartbeat_packet());
    m_hb_timer.expires_from_now(boost::posix_time::seconds(30));
    m_hb_timer.async_wait(std::bind(&platform_bilibili::handle_heartbeat_timer,
        std::dynamic_pointer_cast<platform_bilibili>(shared_from_this()),
        std::placeholders::_1));
}
