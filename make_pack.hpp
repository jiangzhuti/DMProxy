#ifndef MAKE_PACK_HPP
#define MAKE_PACK_HPP

#include "protocol/ChatRoom.pb.h"
#include "protocol/CommunicationData.pb.h"

#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <string>

#include "global.hpp"
#include "utils/md5.hpp"


void new_request_message(PACKET_TYPE msgid, void* req_object, qihoo::protocol::messages::Message* message);

//_sendHandshakePack
std::vector<uint8_t> new_hand_shake_pack();

//_sendLoginPack
std::vector<uint8_t> new_login_pack();

//_sendJoinChatroomPack
std::vector<uint8_t> new_join_chat_room_pack();

#endif // MAKE_PACK_HPP
