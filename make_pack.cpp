#include "make_pack.hpp"

#include "utils/rc4.hpp"
#include <iostream>
#include <sstream>
#include <vector>
#include "utils/others.hpp"

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

void new_request_message(PACKET_TYPE msgid, void* req_object,
    qihoo::protocol::messages::Message* message)
{
    message->set_msgid(msgid);
    message->set_sn(g_user_info.sn);
    message->set_sender(g_user_info.sender);
    message->set_sender_type(g_config.senderType);

    auto* req = new qihoo::protocol::messages::Request();

    switch (msgid) {
    case LoginReq:
        req->set_allocated_login(
            reinterpret_cast<qihoo::protocol::messages::LoginReq*>(req_object));
        break;
    case ChatReq:
        req->set_allocated_chat(
            reinterpret_cast<qihoo::protocol::messages::ChatReq*>(req_object));
        break;
    case GetInfoReq:
        req->set_allocated_get_info(
            reinterpret_cast<qihoo::protocol::messages::GetInfoReq*>(req_object));
        break;
    case LogoutReq:
        req->set_allocated_logout(
            reinterpret_cast<qihoo::protocol::messages::LogoutReq*>(req_object));
        break;
    case InitLoginReq:
        req->set_allocated_init_login_req(
            reinterpret_cast<qihoo::protocol::messages::InitLoginReq*>(
                req_object));
        break;
    case Service_Req:
        req->set_allocated_service_req(
            reinterpret_cast<qihoo::protocol::messages::Service_Req*>(req_object));
        break;
    case Ex1QueryUserStatusReq:
        req->set_allocated_e1_query_user(
            reinterpret_cast<qihoo::protocol::messages::Ex1QueryUserStatusReq*>(
                req_object));
        break;
    case RestoreSessionReq:
        break;
    case QueryInfoReq:
        break;
    case QueryUserStatusReq:
        break;
    case QueryUserRegReq:
        break;
    case ExQueryUserStatusReq:
        break;
    case QueryPeerMsgMaxIdReq:
        break;
    case QueryConvSummaryReq:
        break;
    case UpdateSessionReq:
        break;
    default:
        break;
    }

    message->set_allocated_req(req);
}

std::vector<uint8_t> new_hand_shake_pack()
{
    auto* init_login_req = new qihoo::protocol::messages::InitLoginReq();
    init_login_req->set_client_ram(g_user_info.client_ram);
    init_login_req->set_sig(g_user_info.sign);

    qihoo::protocol::messages::Message msg;
    new_request_message(InitLoginReq, init_login_req, &msg);

    std::string msgc = msg.SerializeAsString();

    std::cout << "\n[HandshakePack] packet = ";
    print_hex(msgc.c_str(), msg.ByteSizeLong());

    char szHeader[12] = { 113, 104, 16, 101, 8, 32, 0, 0, 0, 0, 0, 0 };

    uint32_t length = uint32_t(msgc.length() + 12 + 4);
    uint32_t ulength = swap_uint32(length);

    std::vector<uint8_t> result(length);
    rc4_ptr(reinterpret_cast<const uint8_t*>(msgc.data()), msgc.length(),
        reinterpret_cast<const uint8_t*>(g_config.defaultKey.data()),
        g_config.defaultKey.length(), result.data() + 12 + 4);
    memcpy(result.data(), szHeader, 12);
    memcpy(result.data() + 12, &ulength, 4);

    std::cout << "[HandshakePack] encrypt packet = ";
    print_hex((const char*)result.data(), result.size());

    return result;
}

std::vector<uint8_t> new_login_pack()
{
    auto* login = new qihoo::protocol::messages::LoginReq();
    login->set_app_id(g_config.appId);
    login->set_server_ram(g_user_info.server_ram);

    std::stringstream secret_ram_stream;
    secret_ram_stream.write(g_user_info.server_ram.c_str(),
        g_user_info.server_ram.length());
    secret_ram_stream.write(random_string(8).c_str(), 8);

    std::string secret_ram = rc4_str((const uint8_t*)secret_ram_stream.str().data(),
        secret_ram_stream.str().length(),
        (const uint8_t*)g_user_info.password.data(),
        g_user_info.password.length());
    login->set_secret_ram(secret_ram);

    std::string verf_code = make_verf_code(g_user_info.userid);
    login->set_verf_code(verf_code);

    login->set_net_type(4);
    login->set_mobile_type(MOBILE_PC);
    login->set_not_encrypt(true);
    login->set_platform("h5");

    qihoo::protocol::messages::Message msg;
    new_request_message(LoginReq, login, &msg);

    std::string msgc = msg.SerializeAsString();

    std::cout << "\n[LoginReq] packet = ";
    print_hex(msgc.c_str(), msg.ByteSizeLong());

    uint32_t length = msgc.length() + 4;
    uint32_t ulength = swap_uint32(length);
    std::vector<uint8_t> result(length);
    rc4_ptr((const uint8_t*)msgc.data(), msgc.length(),
        (const uint8_t*)g_config.defaultKey.data(),
        g_config.defaultKey.length(), result.data() + 4);

    memcpy(result.data(), &ulength, 4);

    std::cout << "[LoginReq] encrypt packet = ";
    print_hex((char*)result.data(), result.size());

    return result;
}

//_sendJoinChatroomPack
std::vector<uint8_t> new_join_chat_room_pack()
{
    auto* room = new qihoo::protocol::chatroom::ChatRoom();
    room->set_roomid(g_user_info.roomId);

    auto* applyjoinchatroomreq = new qihoo::protocol::chatroom::ApplyJoinChatRoomRequest();
    applyjoinchatroomreq->set_roomid(g_user_info.roomId);
    applyjoinchatroomreq->set_userid_type(0);
    applyjoinchatroomreq->set_allocated_room(room);

    auto* to_server_data = new qihoo::protocol::chatroom::ChatRoomUpToServer();
    to_server_data->set_payloadtype(102);
    to_server_data->set_allocated_applyjoinchatroomreq(applyjoinchatroomreq);

    auto* packet = new qihoo::protocol::chatroom::ChatRoomPacket();

    std::string uuid = md5_str(random_string(20));
    packet->set_uuid(uuid);

    packet->set_client_sn(g_user_info.sn);
    packet->set_roomid(g_user_info.roomId);
    std::cout << g_user_info.roomId << ":222" << std::endl;;
    packet->set_appid(g_config.appId);
    packet->set_allocated_to_server_data(to_server_data);

    auto* service_req = new qihoo::protocol::messages::Service_Req();
    service_req->set_service_id(10000006);
    service_req->set_request(packet->SerializePartialAsString());

    qihoo::protocol::messages::Message msg;
    new_request_message(Service_Req, service_req, &msg);

    std::string msgc = msg.SerializeAsString();

    std::cout << "\n[Service_Req] packet = ";
    print_hex(msgc.c_str(), msg.ByteSize());

    uint32_t length = msg.ByteSizeLong() + 4;
    uint32_t ulength = swap_uint32(length);

    std::vector<uint8_t> result(length);
    memcpy(result.data(), &ulength, 4);
    memcpy(result.data() + 4, msgc.data(), msgc.length());

    std::cout << "[Service_Req] encrypt packet = ";
    print_hex((char *)result.data(), result.size());

    return result;
}
