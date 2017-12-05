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
using websocketpp::lib::placeholders::_3;

namespace po = boost::program_options;

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


int server_threads, client_threads;
int server_port;
std::string uri;

server dmp_server;
websocketpp::lib::asio::io_service server_io_service;
websocketpp::lib::asio::io_service client_io_service;

std::map<uint64_t, client *> room_client;
std::map<server::connection_ptr, client *> conn_client;
std::map<client *, std::set<server::connection_ptr>> client_conns;

void on_client_message(client* c, websocketpp::connection_hdl hdl, message_ptr msg)
{
    auto conns = client_conns[c];
    for (auto i : conns) {
        dmp_server.send(i->get_handle(), msg->get_payload(), msg->get_opcode());
    }
}

void on_client_open(client* c, std::string payload, websocketpp::frame::opcode::value opcode, websocketpp::connection_hdl hdl)
{
    c->send(hdl, payload, opcode);
}

void on_server_message(server* s, websocketpp::connection_hdl hdl, message_ptr msg) {
    if (msg->get_opcode() == websocketpp::frame::opcode::text) {
        uint64_t roomid;
        std::string payload = msg->get_payload();
        websocketpp::frame::opcode::value opcode = msg->get_opcode();
        std::stringstream sstream;
        sstream << payload;
        sstream >> roomid;
        client *c, *old_c = nullptr;
        server::connection_ptr s_conn = s->get_con_from_hdl(hdl);
        client::connection_ptr c_conn;
        if (room_client.count(roomid) == 0) {
            c = new client();
            c->init_asio(&client_io_service);
            c->set_message_handler(bind(on_client_message, c, _1, _2));
            c->set_open_handler(bind(on_client_open, c, payload, opcode, _1));
            websocketpp::lib::error_code ec;
            c_conn = c->get_connection(uri, ec);
            if (ec) {
                std::cout << "Could not create connection because:" << ec.message() << std::endl;
            }
            c->connect(c_conn);
            room_client[roomid] = c;
            client_conns[c] = std::set<server::connection_ptr>();
        } else {
            c = room_client[roomid];
        }
        if (conn_client.count(s_conn) != 0) {
            old_c = conn_client[s_conn];
        }
        conn_client[s_conn] = c;
        if (old_c != nullptr) {
            client_conns[old_c].erase(s_conn);
        }
        client_conns[c].insert(s_conn);
    }
}

int main(int argc, char *argv[])
{
    po::options_description desc("Allowed Options:");
    desc.add_options()
            ("help", "Produce help message")
            ("server-port", po::value<int>(&server_port)->default_value(9001), "Set the DMProxy server port, default to 9001")
            ("server-threads", po::value<int>(&server_threads)->default_value(3), "Set the number of the DMProxy server io_service threads, defalut to 3")
            ("client-threads", po::value<int>(&client_threads)->default_value(3), "Set the number of the DMProxy client io_service threads, defalut to 3")
            ("dm-ip", po::value<std::string>(), "Set the danmu websocket ip")
            ("dm-port", po::value<int>(), "set the danmu websocket port");
    po::variables_map vm;
    po::store(po::parse_command_line(argc, argv, desc), vm);
    po::notify(vm);
    if (vm.count("help")) {
        std::cout << desc << std::endl;
        return 1;
    }
    if (vm.count("dm-ip") == 0 || vm.count("dm-port") == 0) {
        std::cout << "require dm-ip and dm-port" << std::endl;
        return 1;
    } else {
        std::string dmip = vm["dm-ip"].as<std::string>();
        int dmport = vm["dm-port"].as<int>();
        std::stringstream ss;
        ss << "ws://" << dmip << ":" << dmport;
        ss >> uri;
    }

    try {
        dmp_server.init_asio(&server_io_service);
        dmp_server.set_reuse_addr(true);
        dmp_server.set_message_handler(bind(&on_server_message,&dmp_server,::_1,::_2));
        dmp_server.set_socket_init_handler([](websocketpp::connection_hdl, boost::asio::ip::tcp::socket & s) {
                                               boost::asio::ip::tcp::no_delay option(true);
                                               s.set_option(option);
                                           });
        dmp_server.set_listen_backlog(8192);
        dmp_server.listen(server_port);
        dmp_server.start_accept();

        websocketpp::lib::asio::io_service::work client_work(client_io_service);

        typedef websocketpp::lib::shared_ptr<websocketpp::lib::thread> thread_ptr;
        std::vector<thread_ptr> s_ts, c_ts;
        for (auto i = 0; i < server_threads; i++) {
            s_ts.push_back(websocketpp::lib::make_shared<websocketpp::lib::thread>([&](){server_io_service.run(); std::cout << "server thread returned\n";}));
        }
        for (auto i = 0; i < client_threads; i++) {
            c_ts.push_back(websocketpp::lib::make_shared<websocketpp::lib::thread>([&](){client_io_service.run(); std::cout << "client thread returned\n";}));
        }
        for (auto i : s_ts) {
            i->join();
        }
        for (auto i : c_ts) {
            i->join();
        }

    } catch (websocketpp::exception const &e) {
        std::cout << "Exception:" << e.what() <<std::endl;
    }
    return 0;
}
