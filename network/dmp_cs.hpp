#ifndef DMP_CS_HPP
#define DMP_CS_HPP

#include <websocketpp/server.hpp>
#include <websocketpp/client.hpp>
#include <websocketpp/config/asio_no_tls.hpp>
#include <websocketpp/config/asio_no_tls_client.hpp>

typedef websocketpp::server<websocketpp::config::asio>           dmp_server;
typedef websocketpp::client<websocketpp::config::asio_client>    dmp_client;
typedef websocketpp::config::asio::message_type::ptr             message_ptr;
typedef websocketpp::connection_hdl                              connection_hdl;
using   opcode = websocketpp::frame::opcode::value;
typedef dmp_server::connection_ptr                               s_connection_ptr;


extern dmp_server server;
extern dmp_client client;

#endif // DMP_CS_HPP
