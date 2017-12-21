#ifndef HUAJIAO_HPP
#define HUAJIAO_HPP

#include <string>
#include "utils/others.hpp"
#include "utils/md5.hpp"
#include "platforms/platform_base.hpp"
#include "huajiao_config.hpp"
#include "utils/rw_lock.hpp"
#include "network/dmp_cs.hpp"
#include "protocol/ChatRoom.pb.h"
#include "protocol/CommunicationData.pb.h"

class platform_huajiao final : public platform_base
{
public:
    platform_huajiao(boost::asio::io_service& ios, std::string roomid) : platform_base(ios, "huajiao", roomid)
    {
    }
    void start();
    void close();
    bool is_room_valid();
private:
    void on_client_open(connection_hdl hdl);
    void on_client_fail(connection_hdl hdl);
    void on_client_close(connection_hdl hdl);
    void on_client_message(connection_hdl hdl, message_ptr msg);

    inline std::string make_verf_code(const std::string& text)
    {
        std::string buf(text + "360tantan@1408$");
        return md5_str(buf).substr(24, 8);
    }

    void print_hex(const char* str, size_t len)
    {
        for (size_t i = 0; i < len; ++i)
            printf("%02X", str[i]);
        printf("\n");
    }

    uint32_t swap_uint32(uint32_t value)
    {
        return ((value & 0x000000FF) << 24) | ((value & 0x0000FF00) << 8) | ((value & 0x00FF0000) >> 8) | ((value & 0xFF000000) >> 24);
    }

    void new_request_message(PACKET_TYPE msgid, void* req_object, qihoo::protocol::messages::Message* message);
    std::vector<uint8_t> new_handshake_packet();
    void parse_handshake_packet(const void *data, size_t size);
    std::vector<uint8_t> new_login_packet();
    void parse_login_response_packet(const void *data, size_t size);
    std::vector<uint8_t> new_join_chatroom_packet();
    void handle_binary_message(const void *data, size_t size);
    void parse_message_packet(const void *data, size_t size);
    void parse_new_message_notify(const qihoo::protocol::messages::Message& message);
    void parse_service_resp(const qihoo::protocol::messages::Message& message);
    void parse_address_book_message(const void* data, uint32_t size, qihoo::protocol::messages::Message* message);
    void parse_chat_room_message(const std::string& data, qihoo::protocol::chatroom::ChatRoomPacket* message);
    void parse_chat_room_new_message(const void* data, uint32_t size, qihoo::protocol::chatroom::ChatRoomNewMsg* message);
    void parse_json_message(const std::string& message);
    huajiao_conn_info_t conn_info;
    //正常并不会出现数据争用的情况
    //因此这个锁并没有用到
    rw_mutex_t conn_info_rw_mtx;
    connection_hdl chdl;
};

#endif
