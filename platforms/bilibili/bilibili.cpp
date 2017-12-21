#include "bilibili.hpp"
#include <istream>
#include <ostream>
#include <string>
#include <sstream>
#include <boost/property_tree/xml_parser.hpp>

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
    property_tree::ptree tree;
    std::istringstream iss_live_msg(m_live_msg);
    property_tree::read_xml(iss_live_msg, tree);
    m_dm_server = tree.get<std::string>("dm_server_list");
    std::cerr << m_dm_server;
}
