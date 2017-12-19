#include <map>
#include <set>
#include <sstream>
#include <memory>
#include <utility>
#include <tuple>
#include <boost/program_options.hpp>
#include <boost/asio.hpp>
#include "network/dmp_cs.hpp"
#include "platforms/platforms.hpp"
#include "utils/others.hpp"
#include "utils/rw_lock.hpp"

//use 'static' to prevent clang for generating -Wmissing-variable-declarations warnings
static int server_threads, client_threads;
static uint16_t server_port;
static std::string uri;

static boost::asio::io_service server_io_service;
static boost::asio::io_service platform_io_service;

//STL Containers are thread-safe for concurrent read, so need a rw lock
//see https://stackoverflow.com/questions/7455982/is-stl-vector-concurrent-read-thread-safe

typedef std::map<std::string, platform_base_ptr_t> roomstr_platform_map;
static roomstr_platform_map rp_map;
static rw_mutex_t rp_rw_mtx;

namespace po = boost::program_options;

void on_platform_close(std::string roomstr)
{
    wlock_t wlock(rp_rw_mtx);
    rp_map.erase(roomstr);
}

void on_server_close(std::string roomstr, connection_hdl hdl)
{
    wlock_t wlock(rp_rw_mtx);
    if (rp_map.count(roomstr) == 0) {
        return;
    }
    auto pbase = rp_map[roomstr];
    pbase->erase_listener(hdl);
    if (pbase->listeners_count() == 0) {
        pbase->close();
        rp_map.erase(roomstr);
    }
}

void on_server_message(std::string old_roomstr, connection_hdl hdl, message_ptr msg)
{
    if (msg->get_opcode() != opcode::TEXT) {
        //ignored;
        return;
    }
    //payload format :: "platform-tag" + "_" + "roomid"
    std::string roomstr = msg->get_payload();
    auto pos = roomstr.find_first_of('_');
    if (!(roomstr.length() > 3 && pos > 0 && pos < roomstr.length() - 1)) {
        server.send(hdl, std::string("format error!"), opcode::TEXT);
        return;
    }
    std::string tag = std::string(roomstr, 0, pos);
    std::string roomid = std::string(roomstr, pos + 1);

    platform_base_ptr_t pbase;
    wlock_t rp_wlock(rp_rw_mtx);
    if (old_roomstr == roomstr)
        return;
    if (rp_map.count(old_roomstr) != 0) {
        auto old_pbase = rp_map[old_roomstr];
        old_pbase->erase_listener(hdl);
        if (old_pbase->listeners_count() == 0) {
            old_pbase->close();
            rp_map.erase(old_roomstr);
        }
    }
    if (rp_map.count(roomstr) != 0) {
        pbase = rp_map[roomstr];
        if (!pbase->have_listener(hdl)) {
            pbase->add_listener(hdl);
            return;
        }
    } else {
        std::cerr << "1111";
        pbase = platform_get_instance(tag, platform_io_service);
        std::cerr << "2222";
        if (pbase == nullptr) {
            server.send(hdl, std::string("platform ") + tag + std::string(" is not valid!"), opcode::TEXT);
            return;
        }
        websocketpp::lib::error_code ec;
        if (!pbase->is_room_valid(roomid)) {
            server.send(hdl, std::string("roomid invalid!"), opcode::TEXT);
            return;
        }
    }
    std::cerr << "3333";
    auto conn_ptr = server.get_con_from_hdl(hdl);
    conn_ptr->set_close_handler(std::bind(on_server_close, roomstr, std::placeholders::_1));
    pbase->add_listener(hdl);
    pbase->set_close_callback(on_platform_close);\
    rp_map[roomstr] = pbase;
    pbase->start(roomid);

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
        server.clear_access_channels(websocketpp::log::alevel::all);
        server.clear_error_channels(websocketpp::log::alevel::all);
        server.init_asio(&server_io_service);
        server.set_reuse_addr(true);
        server.set_message_handler(std::bind(on_server_message, std::string(), std::placeholders::_1, std::placeholders::_2));
        server.set_socket_init_handler([](connection_hdl, boost::asio::ip::tcp::socket & s)
                                           {
                                               boost::asio::ip::tcp::no_delay option(true);
                                               s.set_option(option);
                                           });
        server.set_listen_backlog(8192);
        server.listen(server_port);
        server.start_accept();

        client.clear_access_channels(websocketpp::log::alevel::all);
        client.clear_error_channels(websocketpp::log::alevel::all);
        client.init_asio(&platform_io_service);

        boost::asio::io_service::work platform_work(platform_io_service);

        typedef websocketpp::lib::shared_ptr<std::thread> thread_ptr;
        std::vector<thread_ptr> s_ts, c_ts;
        for (auto i = 0; i < server_threads; i++) {
            s_ts.push_back(std::make_shared<std::thread>([&]()
                                                         {
                                                             server_io_service.run();
                                                             std::cout << "server thread returned\n";
                                                         }));
        }
        for (auto i = 0; i < client_threads; i++) {
            c_ts.push_back(std::make_shared<std::thread>([&]()
                                                         {
                                                             platform_io_service.run();
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
