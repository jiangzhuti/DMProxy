#ifndef DOUYU_HPP
#define DOUYU_HPP

#include "platforms/platforms.hpp"
#include "network/dmp_cs.hpp"
#include <boost/asio.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <vector>
#include <tuple>
#include <functional>
#include "douyu_config.hpp"

class platform_douyu final : public platform_base
{
    using tcp = boost::asio::ip::tcp;
public:
    platform_douyu(boost::asio::io_service& ios) : platform_base(ios),
                                                       m_socket(ios),
                                                       m_resolver(ios),
                                                       m_blogin(false),
                                                       m_bjoin(false),
                                                       m_hb_timer(ios)
    {
        std::cerr << __func__ << std::endl;
        tcp::resolver::query query("openbarrage.douyutv.com", "8601");
        auto i = m_resolver.resolve(query);
        m_ep = *i;
        msg_handlers["loginres"] = &platform_douyu::handle_loginres_msg;
        msg_handlers["chatmsg"] = &platform_douyu::handle_chatmsg_msg;

    }
    void start(std::string roomid);
    void close();
    bool is_room_valid(std::string roomid);
private:
    enum class ACTION
    {
        ACT_DO_WRITE,
        ACT_PUBLISH,
        ACT_NOP,
        ACT_TERMINATE
    };
    typedef std::tuple<std::string, std::vector<uint8_t>*, ACTION> msg_action_t;
    typedef std::function<msg_action_t (std::shared_ptr<platform_douyu>, const char *msg, size_t size)> msg_handler_t;

    void on_client_close();
    void do_write(std::vector<uint8_t> *packet);
    void do_read_header();
    void do_read_data(uint32_t data_size);
    void handle_header(std::vector<uint8_t> *header, boost::system::error_code ec, size_t size);
    void handle_data(std::vector<uint8_t> *data, boost::system::error_code ec, size_t size);
    void handle_heartbeat_timer(const boost::system::error_code &ec);

    msg_action_t handle_loginres_msg(const char *msg, size_t size);
    msg_action_t handle_chatmsg_msg(const char *msg, size_t size);
	void handle_danmu_msg(const char *msg, size_t size);
	void handle_xxx_msg(const char *msg, size_t size);
    void on_connect(boost::system::error_code ec);
    std::vector<uint8_t>* new_login_packet();
    std::vector<uint8_t>* new_joingroup_packet();
    std::vector<uint8_t>* new_logout_packet();
    std::vector<uint8_t>* new_heartbeat_packet();
    tcp::socket m_socket;
    tcp::endpoint m_ep;
    tcp::resolver m_resolver;
    std::string m_roomid;
    bool m_blogin;
    bool m_bjoin;
    boost::asio::deadline_timer m_hb_timer;

    std::map<std::string, msg_handler_t> msg_handlers;
};

#endif // DOUYU_HPP
