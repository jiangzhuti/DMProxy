#include <websocketpp/server.hpp>
#include <websocketpp/client.hpp>
#include <websocketpp/config/asio_no_tls.hpp>
#include <websocketpp/config/asio_no_tls_client.hpp>
#include <map>
#include <set>
#include <sstream>
#include <memory>
#include <utility>
#include <tuple>
#include <boost/program_options.hpp>
#include <boost/bimap.hpp>
#include <boost/bimap/set_of.hpp>
#include <boost/bimap/unordered_set_of.hpp>
#include <boost/bimap/multiset_of.hpp>
#include <boost/bimap/vector_of.hpp>
#include <boost/bimap/tags/tagged.hpp>
#include "platforms/platforms.hpp"
#include "utils/others.hpp"
#include "utils/rw_lock.hpp"

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
typedef websocketpp::connection_hdl connection_hdl;

//use 'static' to prevent clang for generating -Wmissing-variable-declarations warnings
static int server_threads, client_threads;
static uint16_t server_port;
static std::string uri;

static server dmp_server;
static client dmp_client;
static websocketpp::lib::asio::io_service server_io_service;
static websocketpp::lib::asio::io_service client_io_service;

using namespace boost::bimaps; //这个组合起来实在是太长

//1.
//use std::owner_less to compare connection_hdl (aka weak_ptr)
//see https://stackoverflow.com/questions/33445976/is-it-safe-to-store-a-changing-object-in-a-stl-set/
//2.
//weak_ptr also has no hash function, so do not use in unordered map/set
//see https://stackoverflow.com/questions/4750504/why-was-stdhash-not-defined-for-stdweak-ptr-in-c0x
//3.
//STL Containers are thread-safe for concurrent read, so need a rw lock
//see https://stackoverflow.com/questions/7455982/is-stl-vector-concurrent-read-thread-safe


typedef boost::bimap<
                     unordered_set_of<tagged<std::string, struct tag_roomid>>,
                     set_of<tagged<connection_hdl, struct tag_c_conn>, std::owner_less<connection_hdl>>
                    > roomid_conn_bm;
static roomid_conn_bm rc_bm;
static rw_mutex_t rc_rw_mtx;

typedef boost::bimap<
                     multiset_of<tagged<websocketpp::connection_hdl, struct tag_c_conn>, std::owner_less<websocketpp::connection_hdl>>, //client conn
                     set_of<tagged<websocketpp::connection_hdl, struct tag_s_conn>, std::owner_less<websocketpp::connection_hdl>> //server_conns
                    > client_server_conn_bm;
static client_server_conn_bm cs_bm;
static rw_mutex_t cs_rw_mtx;

using websocketpp::lib::bind;

namespace po = boost::program_options;

#define PRINT_ERROR(ec) \
    std::cerr << __func__ << ":" << __LINE__ << " error occured because: " << ec.message() << std::endl;

void handle_platform_packet(const platform_packet_t &packet, websocketpp::connection_hdl c_hdl)
{
    websocketpp::lib::error_code ec;
    const void *data;
    size_t size = 0;
    auto opcode = websocketpp::frame::opcode::TEXT;
    if (std::get<3>(packet) == platform_packet_direct::NONE || std::get<2>(packet) == platform_packet_encap::NONE) {
        return;
    }
    if (std::get<2>(packet) == platform_packet_encap::TEXT) {
        data = std::get<0>(packet).data();
        size = std::get<0>(packet).length();
        opcode = websocketpp::frame::opcode::TEXT;
    } else if (std::get<2>(packet) == platform_packet_encap::BINARY) {
        data = std::get<1>(packet).data();
        size = std::get<1>(packet).size();
        opcode = websocketpp::frame::opcode::BINARY;
    }
    if (std::get<3>(packet) == platform_packet_direct::CLIENT) {
        dmp_client.send(c_hdl, data, size, opcode, ec);
        if (ec) {
            PRINT_ERROR(ec)
        }
    } else if (std::get<3>(packet) == platform_packet_direct::SERVER) {
        rlock_t cs_rlock(cs_rw_mtx);
        auto range = cs_bm.by<tag_c_conn>().equal_range(c_hdl); //find server conns
        for (auto conn = range.first; conn != range.second; conn++) {
            websocketpp::lib::error_code ec;
            dmp_server.send(conn->second, data, size, opcode, ec);
            if (ec) {
                PRINT_ERROR(ec)
            }
        }
    } else {
        //never reached
    }
}

void on_client_message(platform_base_ptr_t pbase, websocketpp::connection_hdl hdl, message_ptr msg)
{
    platform_packet_t packet;
    if (msg->get_opcode() == websocketpp::frame::opcode::BINARY) {
        packet = pbase->handle_binary_message(msg->get_payload().data(), msg->get_payload().length());
    } else {
        packet = pbase->handle_text_message(msg->get_payload());
    }
    handle_platform_packet(packet, hdl);
}

void on_client_open(platform_base_ptr_t pbase, std::string roomid, websocketpp::connection_hdl hdl)
{
    auto packet = pbase->handle_open(roomid);
    handle_platform_packet(packet, hdl);

}

void on_client_close(websocketpp::connection_hdl hdl)
{
    wlock_t rc_wlock(rc_rw_mtx);
    wlock_t cs_wlock(cs_rw_mtx);
    auto range = cs_bm.by<tag_c_conn>().equal_range(hdl);
    for (auto conn = range.first; conn != range.second; conn++) {
        websocketpp::lib::error_code ec;
        //每一个conn可能会请求其他的roomid，因此不能关停conn
        dmp_server.send(conn->second, std::string("dm connection closed"), websocketpp::frame::opcode::TEXT, ec);
        if (ec) {
            PRINT_ERROR(ec)
        }
    }
    cs_bm.by<tag_c_conn>().erase(hdl);
    rc_bm.by<tag_c_conn>().erase(hdl);
}

void client_conn_close_with_check(websocketpp::connection_hdl hdl)
{
    if (cs_bm.by<tag_c_conn>().count(hdl) == 0) {
        rc_bm.by<tag_c_conn>().erase(hdl);
        websocketpp::lib::error_code ec;
        dmp_client.close(hdl, websocketpp::close::status::normal, std::string("close"), ec);
        if (ec) {
            PRINT_ERROR(ec)
        }
    }
}

void on_server_close(websocketpp::connection_hdl hdl)
{
    wlock_t rc_wlock(rc_rw_mtx);
    wlock_t cs_wlock(cs_rw_mtx);
    auto c_hdl = cs_bm.by<tag_s_conn>().find(hdl)->second;
    cs_bm.by<tag_s_conn>().erase(hdl);
    client_conn_close_with_check(c_hdl);
}

void on_server_message(websocketpp::connection_hdl hdl, message_ptr msg)
{
    if (msg->get_opcode() != websocketpp::frame::opcode::TEXT) {
        //ignored;
        return;
    }
    //payload format :: "platform-tag" + "_" + "roomid"
    std::string payload = msg->get_payload();
    auto pos = payload.find_first_of('_');
    if (!(payload.length() > 3 && pos > 0 && pos < payload.length() - 1)) {
        dmp_server.send(hdl, std::string("format error!"), websocketpp::frame::opcode::TEXT);
        return;
    }
    std::string tag = std::string(payload, 0, pos);
    std::string roomid = std::string(payload, pos + 1);

    platform_base_ptr_t pbase = platform_get_instance(tag);
    if (pbase == nullptr) {
        dmp_server.send(hdl, std::string("platform ") + tag + std::string(" is not valid!"), websocketpp::frame::opcode::TEXT);
        return;
    }
    websocketpp::lib::error_code ec;
    if (!pbase->is_roomid_valid(roomid)) {
        dmp_server.send(hdl, std::string("roomid invalid!"), websocketpp::frame::opcode::TEXT);
        return;
    }
    websocketpp::connection_hdl c_hdl, old_c_hdl;
    wlock_t rc_wlock(rc_rw_mtx);
    wlock_t cs_wlock(cs_rw_mtx);
    //check if the server conn already bound to a client conn
    if (cs_bm.by<tag_s_conn>().count(hdl) != 0) { //todo. should assert == 1
        //find client conn
        old_c_hdl = cs_bm.by<tag_s_conn>().find(hdl)->second;
        //if same room id
        if (roomid == rc_bm.by<tag_c_conn>().find(old_c_hdl)->second) {
            //just return
            return;
        } else { //request a new room id
            //1. erase server conn from its client bound
            //2. check if no exist requests to the old room id
            cs_bm.by<tag_s_conn>().erase(hdl);
            client_conn_close_with_check(old_c_hdl);
        }
    }
    //if new room id exsits
    if (rc_bm.by<tag_roomid>().count(roomid) != 0) {
        //find client conn
        c_hdl = rc_bm.by<tag_roomid>().find(roomid)->second;
    } else {
        //create new client conn
        client::connection_ptr c_conn = dmp_client.get_connection(pbase->get_dm_url(), ec);
        if (ec) {
            PRINT_ERROR(ec)
            dmp_server.send(hdl, std::string("open connection failed!"), websocketpp::frame::opcode::TEXT);
            return;
        }
        c_conn->set_open_handler(bind(on_client_open, pbase, roomid, websocketpp::lib::placeholders::_1));
        c_conn->set_message_handler(bind(on_client_message, pbase, websocketpp::lib::placeholders::_1, websocketpp::lib::placeholders::_2));
        c_conn->set_close_handler(on_client_close);

        dmp_client.connect(c_conn);
        c_hdl = c_conn->get_handle();
        rc_bm.by<tag_roomid>().insert(std::make_pair(roomid, c_hdl));
    }
    //bind server conn to client conn
    cs_bm.by<tag_c_conn>().insert(std::make_pair(c_hdl, hdl));
}

int main(int argc, char *argv[])
{
    po::options_description desc("Allowed Options:");
    desc.add_options()
            ("help", "Produce help message")
            ("server-port", po::value<uint16_t>(&server_port)->default_value(9001), "Set the DMProxy server port, default to 9001")
            ("server-threads", po::value<int>(&server_threads)->default_value(3), "Set the number of the DMProxy server io_service threads, defalut to 3")
            ("client-threads", po::value<int>(&client_threads)->default_value(3), "Set the number of the DMProxy client io_service threads, defalut to 3");
    po::variables_map vm;
    po::store(po::parse_command_line(argc, argv, desc), vm);
    po::notify(vm);
    if (vm.count("help")) {
        std::cout << desc << std::endl;
        return 1;
    }

    platforms_init();

    try {
        dmp_server.clear_access_channels(websocketpp::log::alevel::all);
        dmp_server.clear_error_channels(websocketpp::log::alevel::all);
        dmp_server.init_asio(&server_io_service);
        dmp_server.set_reuse_addr(true);
        dmp_server.set_message_handler(on_server_message);
        dmp_server.set_close_handler(on_server_close);
        dmp_server.set_socket_init_handler([](websocketpp::connection_hdl, boost::asio::ip::tcp::socket & s)
                                           {
                                               boost::asio::ip::tcp::no_delay option(true);
                                               s.set_option(option);
                                           });
        dmp_server.set_listen_backlog(8192);
        dmp_server.listen(server_port);
        dmp_server.start_accept();

        dmp_client.clear_access_channels(websocketpp::log::alevel::all);
        dmp_client.clear_error_channels(websocketpp::log::alevel::all);
        dmp_client.init_asio(&client_io_service);

        websocketpp::lib::asio::io_service::work client_work(client_io_service);

        typedef websocketpp::lib::shared_ptr<websocketpp::lib::thread> thread_ptr;
        std::vector<thread_ptr> s_ts, c_ts;
        for (auto i = 0; i < server_threads; i++) {
            s_ts.push_back(websocketpp::lib::make_shared<websocketpp::lib::thread>([&]()
                                                                                   {
                                                                                       server_io_service.run();
                                                                                       std::cout << "server thread returned\n";
                                                                                   }));
        }
        for (auto i = 0; i < client_threads; i++) {
            c_ts.push_back(websocketpp::lib::make_shared<websocketpp::lib::thread>([&]()
                                                                                   {
                                                                                       client_io_service.run();
                                                                                       std::cout << "client thread returned\n";
                                                                                   }));
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
