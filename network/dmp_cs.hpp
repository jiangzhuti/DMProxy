#ifndef DMP_CS_HPP
#define DMP_CS_HPP

#include <websocketpp/server.hpp>
#include <websocketpp/client.hpp>
#include <websocketpp/config/asio_no_tls.hpp>
#include <websocketpp/config/asio_no_tls_client.hpp>
#include <system_error>
#include <iostream>

typedef websocketpp::server<websocketpp::config::asio>           dmp_server;
typedef websocketpp::client<websocketpp::config::asio_client>    dmp_client;
typedef websocketpp::config::asio::message_type::ptr             message_ptr;
typedef websocketpp::connection_hdl                              connection_hdl;
using   opcode = websocketpp::frame::opcode::value;
typedef dmp_server::connection_ptr                               s_connection_ptr;

extern dmp_server server;
extern dmp_client client;

#define SERVER_CLOSE_AND_REPORT_WHEN_ERROR(ec, hdl) \
    ({ \
        if (ec) { \
            std::cerr << __func__ << ":" << __LINE__ << " error occured because: " \
                      << ec.message() << ", server connection closed." << std::endl; \
            std::error_code tmp_ec; \
            server.close(hdl, websocketpp::close::status::normal, std::string("connection closed because:").append(ec.message()), tmp_ec); \
        } \
        ec; \
    })



#define CLIENT_REPORT_WHEN_ERROR(ec) \
    ({ \
        if (ec) { \
            std::cerr << __func__ << ":" << __LINE__ << " error occured because: " \
                      << ec.message() << ", server connection closed." << std::endl; \
            publish(std::string("connection closed because:").append(ec.message())); \
            m_close_callback(std::string().append(m_platform_name).append("_").append(m_roomid)); \
        } \
        ec; \
    })

#define CLIENT_CLOSE_AND_REPORT_WHEN_ERROR_WS(ec, hdl) \
    ({ \
        CLIENT_REPORT_WHEN_ERROR(ec); \
        if (ec) { \
            std::error_code tmp_ec; \
            client.close(hdl, websocketpp::close::status::normal, "close", tmp_ec); \
        } \
        ec;\
    })


#endif // DMP_CS_HPP
