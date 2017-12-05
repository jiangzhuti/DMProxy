#include "make_pack.hpp"

#include <sstream>

void new_request_message(PACKET_TYPE msgid, void* req_object, qihoo::protocol::messages::Message *message)
{
    message->set_msgid(msgid);
    message->set_sn(g_user_info.sn);
    message->set_sender(g_user_info.sender);
    message->set_sender_type(g_config.senderType);

    auto *req = new qihoo::protocol::messages::Request();

    switch (msgid)
    {
        case LoginReq:
            req->set_allocated_login(reinterpret_cast<qihoo::protocol::messages::LoginReq*>(req_object));
            break;
        case ChatReq:
            req->set_allocated_chat(reinterpret_cast<qihoo::protocol::messages::ChatReq*>(req_object));
            break;
        case GetInfoReq:
            req->set_allocated_get_info(reinterpret_cast<qihoo::protocol::messages::GetInfoReq*>(req_object));
            break;
        case LogoutReq:
            req->set_allocated_logout(reinterpret_cast<qihoo::protocol::messages::LogoutReq*>(req_object));
            break;
        case InitLoginReq:
            req->set_allocated_init_login_req(reinterpret_cast<qihoo::protocol::messages::InitLoginReq*>(req_object));
            break;
        case Service_Req:
            req->set_allocated_service_req(reinterpret_cast<qihoo::protocol::messages::Service_Req*>(req_object));
            break;
        case Ex1QueryUserStatusReq:
            req->set_allocated_e1_query_user(reinterpret_cast<qihoo::protocol::messages::Ex1QueryUserStatusReq*>(req_object));
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

std::string new_hand_shake_pack()
{
    auto *init_login_req = new qihoo::protocol::messages::InitLoginReq();
    init_login_req->set_client_ram(g_user_info.client_ram);
    init_login_req->set_sig(g_user_info.sign);

    qihoo::protocol::messages::Message msg;
    new_request_message(InitLoginReq, init_login_req, &msg);

    std::string msgc = msg.SerializeAsString();

    std::cout <<  "\n[HandshakePack] packet = ";
    print_hex(msgc.c_str(), msg.ByteSizeLong());

    std::string out_result;
    rc4_xx(msgc, g_config.defaultKey, &out_result);

    char szHeader[12] = { 113,104,16,101,8,32,0,0,0,0,0,0 };

    uint32_t length = uint32_t(out_result.length() + 12 + 4);
    uint32_t ulength = swap_uint32(length);

    std::stringstream mystream;
    mystream.write(szHeader, 12);
    mystream.write((char*)&ulength, 4);
    mystream.write(out_result.c_str(), out_result.length());

    std::string result = mystream.str();

    std::cout << "[HandshakePack] encrypt packet = ";
    print_hex(result.c_str(), result.length());

    return result;
}

std::string new_login_pack()
{
    auto *login = new qihoo::protocol::messages::LoginReq();
    login->set_app_id(g_config.appId);
    login->set_server_ram(g_user_info.server_ram);

    std::stringstream secret_ram_stream;
    secret_ram_stream.write(g_user_info.server_ram.c_str(), g_user_info.server_ram.length());
    secret_ram_stream.write(randomString(8).c_str(), 8);

    std::string secret_ram;
    rc4_xx(secret_ram_stream.str(), g_user_info.password, &secret_ram);
    login->set_secret_ram(secret_ram);

    std::string verf_code;
    make_verf_code(g_user_info.userid, &verf_code);
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

    std::string out_result;
    rc4_xx(msgc, g_config.defaultKey, &out_result);

    uint32_t length = out_result.length() + 4;
    uint32_t ulength = swap_uint32(length);

    std::stringstream mystream;
    mystream.write((char*)&ulength, 4);
    mystream.write(out_result.c_str(), out_result.length());
    std::string result = mystream.str();

    std::cout << "[LoginReq] encrypt packet = ";
    print_hex(result.c_str(), result.length());

    return result;
}

//_sendJoinChatroomPack
std::string new_join_chat_room_pack()
{
    auto *room = new qihoo::protocol::chatroom::ChatRoom();
    room->set_roomid(g_user_info.roomId);

    auto *applyjoinchatroomreq = new qihoo::protocol::chatroom::ApplyJoinChatRoomRequest();
    applyjoinchatroomreq->set_roomid(g_user_info.roomId);
    applyjoinchatroomreq->set_userid_type(0);
    applyjoinchatroomreq->set_allocated_room(room);

    auto *to_server_data = new qihoo::protocol::chatroom::ChatRoomUpToServer();
    to_server_data->set_payloadtype(102);
    to_server_data->set_allocated_applyjoinchatroomreq(applyjoinchatroomreq);

    auto *packet = new qihoo::protocol::chatroom::ChatRoomPacket();

    std::string uuid;
    md5_str(random_string(20), &uuid);
    packet->set_uuid(uuid);

    packet->set_client_sn(g_user_info.sn);
    packet->set_roomid(g_user_info.roomId);
    packet->set_appid(g_config.appId);
    packet->set_allocated_to_server_data(to_server_data);

    auto *service_req = new qihoo::protocol::messages::Service_Req();
    service_req->set_service_id(10000006);
    service_req->set_request(packet->SerializePartialAsString());

    qihoo::protocol::messages::Message msg;
    new_request_message(Service_Req, service_req, &msg);

    std::string msgc = msg.SerializeAsString();

    std::cout << "\n[Service_Req] packet = ";
    print_hex(msgc.c_str(), msg.ByteSize());

    uint32_t length = msg.ByteSizeLong() + 4;
    uint32_t ulength = swap_uint32(length);

    std::stringstream mystream;
    mystream.write((char*)&ulength, 4);
    mystream.write(msgc.c_str(), msgc.length());
    std::string result = mystream.str();

    std::cout << "[Service_Req] encrypt packet = ";
    print_hex(result.c_str(), (int)result.length());

    return result;
}
