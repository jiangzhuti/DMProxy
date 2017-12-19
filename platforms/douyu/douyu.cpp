#include "douyu.hpp"
#include "utils/log.hpp"
#include "stt.hpp"
#include "utils/others.hpp"

bool platform_douyu::is_room_valid(std::string roomid)
{
    //return str_is_num(roomid);
    //可以有字母
    return true;
}

void platform_douyu::start(std::string roomid)
{
    m_roomid = std::move(roomid);
    auto fp = std::bind(&platform_douyu::on_connect,
                        std::dynamic_pointer_cast<platform_douyu>(shared_from_this()),
                        std::placeholders::_1);
    m_socket.async_connect(m_ep, fp);
}

void platform_douyu::close()
{
    boost::system::error_code ec;
    m_socket.close(ec);
    if (ec) {
        PRINT_ERROR(ec)
    }
}

void platform_douyu::on_connect(boost::system::error_code ec)
{
    if (ec) {
        PRINT_ERROR(ec)
    }
    do_write(new_login_packet());
    do_read_header();
}

void platform_douyu::do_write(std::vector<uint8_t> *packet)
{
    std::cerr << __func__  << std::endl;
    //see this:
    //https://stackoverflow.com/questions/19368012/whats-the-reason-of-using-auto-selfshared-from-this-variable-in-lambda-func
    auto self = shared_from_this();
    boost::asio::async_write(m_socket,
                             boost::asio::buffer(packet->data(), packet->size()),
                             [this, self, packet](boost::system::error_code ec, std::size_t /*length*/)
                             {
                                 delete packet;
                                 if (!ec) {
                                        //nothing to do
                                 } else {
                                     PRINT_ERROR(ec)
                                 }
                             });
}

void platform_douyu::do_read_header()
{
    std::cerr << __func__ << std::endl;
    std::vector<uint8_t> *header = new std::vector<uint8_t>(HEADER_SIZE);
    boost::asio::async_read(m_socket,
                            boost::asio::buffer(header->data(), HEADER_SIZE),
                            std::bind(&platform_douyu::handle_header,
                                      std::dynamic_pointer_cast<platform_douyu>(shared_from_this()),
                                      header,
                                      std::placeholders::_1,
                                      std::placeholders::_2));
}

void platform_douyu::do_read_data(uint32_t data_size)
{
    std::cerr << __func__ << std::endl;
    std::vector<uint8_t> *data = new std::vector<uint8_t>(data_size);
    boost::asio::async_read(m_socket,
                            boost::asio::buffer(data->data(), data_size),
                            std::bind(&platform_douyu::handle_data,
                                      std::dynamic_pointer_cast<platform_douyu>(shared_from_this()),
                                      data,
                                      std::placeholders::_1,
                                      std::placeholders::_2));
}

void platform_douyu::handle_header(std::vector<uint8_t> *header, boost::system::error_code ec, size_t size)
{
    std::cerr << __func__ << std::endl;;
    if (!ec) {
        uint32_t data_len = *(uint32_t *)(header->data());
        uint16_t msg_type = *(uint16_t *)(header->data() + 8);
		delete header;
        do_read_data(data_len - 8);
    } else {
        PRINT_ERROR(ec)
    }
}

void platform_douyu::handle_data(std::vector<uint8_t> *data, boost::system::error_code ec, size_t size)
{
    std::cerr << __func__ << std::endl;
    do_read_header();
	if (!ec) {
        if (data->at(data->size() - 1) != '\0') {
			//error
			//delete data
		}
        const char *msg_ptr = reinterpret_cast<const char *>(data->data());
        STT_t stt = parse_stt(msg_ptr);
        std::string type = stt["type"];
        if (msg_handlers.count(type) == 0) {
            delete data;
            return;
        }
        auto msg_handler = msg_handlers[type];
        std::string result;
        std::vector<uint8_t> *packet;
        enum ACTION action;
        std::tie(result, packet, action) = msg_handler(std::dynamic_pointer_cast<platform_douyu>(shared_from_this()), msg_ptr, data->size());
        switch (action) {
        case ACTION::ACT_PUBLISH:
            std::cerr << "publish:" << result << std::endl;;
            publish(result);
            break;
        case ACTION::ACT_DO_WRITE:
            do_write(packet);
            break;
        case ACTION::ACT_NOP:
            //nop
            break;
        case ACTION::ACT_TERMINATE:
            //close
            break;

        }
	} else {
		PRINT_ERROR(ec)
	}
	delete data;
}

void platform_douyu::handle_heartbeat_timer(const boost::system::error_code& ec)
{
    if (!ec) {
        do_write(new_heartbeat_packet());
        m_hb_timer.expires_from_now(boost::posix_time::seconds(45));
        m_hb_timer.async_wait(std::bind(&platform_douyu::handle_heartbeat_timer,
                                        std::dynamic_pointer_cast<platform_douyu>(shared_from_this()),
                                        std::placeholders::_1));
    } else {
        PRINT_ERROR(ec)
    }
}

auto platform_douyu::handle_loginres_msg(const char *msg, size_t size) -> decltype (platform_douyu::handle_loginres_msg(nullptr, 1))
{
    (void)size;
    std::cerr << __func__ << std::endl;
    auto self = shared_from_this();
    STT_t stt = parse_stt(msg);
    std::string result;
    std::vector<uint8_t> *packet;
    if (stt["type"] == "loginres") {
        m_hb_timer.expires_from_now(boost::posix_time::seconds(1));
        m_hb_timer.async_wait(std::bind(&platform_douyu::handle_heartbeat_timer,
                                        std::dynamic_pointer_cast<platform_douyu>(shared_from_this()),
                                        std::placeholders::_1));
        packet = new_joingroup_packet();
        return std::make_tuple(result, packet, ACTION::ACT_DO_WRITE);
    } else {
        std::cerr << "error at func:" << __func__ << ", line:" << __LINE__ << std::endl;
        //impossible
    }
}

auto platform_douyu::handle_chatmsg_msg(const char *msg, size_t size) -> decltype (platform_douyu::handle_chatmsg_msg(nullptr, 1))
{
    (void)size;
    std::cerr << __func__ << std::endl;
    std::cerr << "chatmsg:" << msg << std::endl;
    STT_t stt = parse_stt(msg);
    std::string result;
    std::vector<uint8_t> *packet;

    if (stt["type"] == "chatmsg") {
        result.append("uid:").append(stt["uid"]).append("\n");
        result.append("nickname:").append(stt["nn"]).append("\n");
        result.append("text:").append(stt["txt"]).append("\n");
        return std::make_tuple(result, packet, ACTION::ACT_PUBLISH);
    } else {
        std::cerr << "error at func:" << __func__ << ", line:" << __LINE__ << std::endl;
        //impossible
    }
}

std::vector<uint8_t>* platform_douyu::new_login_packet()
{
    return make_stt({
                        {"type", "loginreq"},
                        {"roomid", m_roomid}
                    });
}

std::vector<uint8_t>* platform_douyu::new_joingroup_packet()
{
    return make_stt({
                        {"type", "joingroup"},
                        {"rid", m_roomid},
                        {"gid", "-9999"}
                    });
}

std::vector<uint8_t>* platform_douyu::new_logout_packet()
{
    return make_stt({
                        {"type", "logout"}
                    });
}

std::vector<uint8_t>* platform_douyu::new_heartbeat_packet()
{
    return make_stt({
                        {"type", "mrkl"}
                    });
}
