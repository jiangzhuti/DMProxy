#include "huajiao.hpp"
#include "make_pack.hpp"
#include "utils/others.hpp"
#include <cassert>

platform_packet_t platform_huajiao::handle_text_message(const std::string &msg)
{
    (void)(msg);
    return std::make_tuple(std::string(), std::vector<uint8_t>(), platform_packet_encap::NONE, platform_packet_direct::NONE);
}

bool platform_huajiao::is_roomid_valid(std::string roomid)
{
    if (!str_is_num(roomid) || roomid.length() != 9) {
        return false;
    }
    return true;
}

std::string platform_huajiao::get_dm_url()
{
    return huajiao_config.wsServer;
}

platform_packet_t platform_huajiao::handle_open(std::string roomid)
{
    conn_info.roomId = std::move(roomid);
    std::vector<uint8_t> hspacket = new_hand_shake_pack(conn_info);
    return std::make_tuple(std::string(),
                           std::move(hspacket),
                           platform_packet_encap::BINARY,
                           platform_packet_direct::CLIENT);
}

platform_packet_t platform_huajiao::handle_binary_message(const void *data, size_t size)
{
    std::string msg = HandleBinaryMessage(data, size, conn_info);
    //assert(conn_info.handshake == true);
    if (conn_info.handshake && !conn_info.bLogin) {
        std::vector<uint8_t> loginpacket = new_login_pack(conn_info);
        return std::make_tuple(std::string(),
                               std::move(loginpacket),
                               platform_packet_encap::BINARY,
                               platform_packet_direct::CLIENT);
    }
    if (conn_info.handshake && conn_info.bLogin && !conn_info.bJoin) {
        std::vector<uint8_t> jcpacket = new_join_chat_room_pack(conn_info);
        conn_info.bJoin = true;
        return std::make_tuple(std::string(),
                               std::move(jcpacket),
                               platform_packet_encap::BINARY,
                               platform_packet_direct::CLIENT);
    }

    return std::make_tuple(std::move(msg),
                           std::vector<uint8_t>(),
                           platform_packet_encap::TEXT,
                           platform_packet_direct::SERVER);

}
