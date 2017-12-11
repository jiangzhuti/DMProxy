#ifndef PARSE_PACK_HPP
#define PARSE_PACK_HPP

#include "protocol/ChatRoom.pb.h"
#include "protocol/CommunicationData.pb.h"
#include <cstdint>
#include <string>
#include "global.hpp"

std::string HandleBinaryMessage(const void* data, uint32_t size, conn_info_t &ci);
std::string parse_service_resp(const qihoo::protocol::messages::Message& message);
std::string parse_new_message_notify(const qihoo::protocol::messages::Message& message);
std::string parse_hand_shake_pack(const void* data, uint32_t size, conn_info_t &ci);
std::string parse_longin_response_pack(const void* data, uint32_t size, conn_info_t &ci);
std::string parse_message_pack(const void* data, uint32_t size, conn_info_t &ci);
void parse_chat_room_message(const std::string& data, qihoo::protocol::chatroom::ChatRoomPacket *message);
void parse_address_book_message(const void* data, uint32_t size, qihoo::protocol::messages::Message *message);
void parse_chat_room_new_message(const void* data, uint32_t size, qihoo::protocol::chatroom::ChatRoomNewMsg *message);
std::string parse_json_message(const std::string& message);


#endif // PARSE_PACK_HPP
