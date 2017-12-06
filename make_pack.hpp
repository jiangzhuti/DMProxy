#ifndef MAKE_PACK_HPP
#define MAKE_PACK_HPP

#include "protocol/ChatRoom.pb.h"
#include "protocol/CommunicationData.pb.h"

#include <string>
#include <cstdio>
#include <cstdint>
#include <algorithm>
#include <random>

#include "global.hpp"
#include "utils/md5.hpp"

inline std::string make_verf_code(const std::string& text)
{
    std::string buf(text + "360tantan@1408$");
    return md5_str(buf).substr(24, 8);
}

void print_hex(const char *str, size_t len)
{
    for (size_t i = 0; i < len; ++i)
        printf("%02X", str[i]);
    printf("\n");
}

uint32_t swap_uint32(uint32_t value)
{
    return ((value & 0x000000FF) << 24) |
    ((value & 0x0000FF00) << 8) |
    ((value & 0x00FF0000) >> 8) |
    ((value & 0xFF000000) >> 24);
}

std::string random_string(const size_t length) {
    const std::string chars("0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz");
    std::default_random_engine rng(std::random_device{}());
    std::uniform_int_distribution<> dist(0, static_cast<int>(chars.size()) - 1);
    std::string result(length, '0');
    for (auto& chr : result)
        chr = chars[dist(rng)];
    return result;
}


void new_request_message(PACKET_TYPE msgid, void* req_object, qihoo::protocol::messages::Message *message);

//_sendHandshakePack
std::string new_hand_shake_pack();

//_sendLoginPack
std::string new_login_pack();

//_sendJoinChatroomPack
std::string new_join_chat_room_pack();

#endif // MAKE_PACK_HPP
