#include <websocketpp/server.hpp>
#include <websocketpp/client.hpp>
#include <websocketpp/config/asio_no_tls.hpp>
#include <websocketpp/config/asio_no_tls_client.hpp>
#include <map>
#include <set>
#include <sstream>
#include <boost/program_options.hpp>

using websocketpp::lib::bind;
using websocketpp::lib::placeholders::_1;
using websocketpp::lib::placeholders::_2;

struct dmproxy_config : public websocketpp::config::asio {
    // pull default settings from our core config
    typedef websocketpp::config::asio core;

    typedef core::concurrency_type concurrency_type;
    typedef core::request_type request_type;
    typedef core::response_type response_type;
    typedef core::message_type message_type;
    typedef core::con_msg_manager_type con_msg_manager_type;
    typedef core::endpoint_msg_manager_type endpoint_msg_manager_type;

    typedef core::alog_type alog_type;
    typedef core::elog_type elog_type;
    typedef core::rng_type rng_type;
    typedef core::endpoint_base endpoint_base;

    static bool const enable_multithreading = true;

    struct transport_config : public core::transport_config {
        typedef core::concurrency_type concurrency_type;
        typedef core::elog_type elog_type;
        typedef core::alog_type alog_type;
        typedef core::request_type request_type;
        typedef core::response_type response_type;

        static bool const enable_multithreading = true;
    };
};

typedef websocketpp::server<dmproxy_config> server;
typedef websocketpp::client<websocketpp::config::asio_client> client;
typedef server::message_ptr message_ptr;

server dmp_server;

std::map<uint64_t, client *> room_clients;
std::map<client *, std::set<websocketpp::connection_hdl>> client_conns;

void on_server_message(server* s, websocketpp::connection_hdl hdl, message_ptr msg) {
//    if (msg->get_opcode() == websocketpp::frame::opcode::text) {
//        uint64_t roomid;
//        std::string payload = msg->get_payload();
//        std::stringstream sstream;
//        sstream << payload;
//        sstream >> roomid;

//    }
    s->send(hdl, msg->get_payload(), msg->get_opcode());
    //msg->get_payload() --> roomid
    //if room_client.has(roomid)
    //client *c = room_client[roomid]
    //else
    //client *c = new client
    //init c
    //end if
    //client_conns[c] = std::set();
    //client_conns[c].insert(hdl);
}

void on_client_message(client* c, websocketpp::connection_hdl hdl, message_ptr msg)
{

}

void on_socket_init(websocketpp::connection_hdl, boost::asio::ip::tcp::socket & s) {
    boost::asio::ip::tcp::no_delay option(true);
    s.set_option(option);
}
int main(int argc, char *argv[])
{
    size_t num_threads = 1;
    short port = 9002;

    if (argc == 3) {
        port = atoi(argv[1]);
        num_threads = atoi(argv[2]);
    }

    websocketpp::lib::asio::io_service server_io_service;

    try {
        dmp_server.init_asio(&server_io_service);
        dmp_server.set_reuse_addr(true);
        dmp_server.set_message_handler(bind(&on_server_message,&dmp_server,::_1,::_2));
        dmp_server.set_socket_init_handler(bind(&on_socket_init, ::_1, ::_2));
        dmp_server.set_listen_backlog(8192);
        dmp_server.listen(port);
        dmp_server.start_accept();

        if (num_threads == 1) {
            server_io_service.run();
        } else {
            typedef websocketpp::lib::shared_ptr<websocketpp::lib::thread> thread_ptr;
            std::vector<thread_ptr> ts;
            for (size_t i = 0; i < num_threads; i++) {
                ts.push_back(websocketpp::lib::make_shared<websocketpp::lib::thread>([&](){server_io_service.run();}));
            }
            for (auto i : ts) {
                i->join();
            }
        }

    } catch (websocketpp::exception const &e) {
        std::cout << "Exception:" << e.what() <<std::endl;
    }
    return 0;
}
