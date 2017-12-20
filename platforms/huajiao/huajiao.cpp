#include "utils/others.hpp"
#include <cassert>
#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <string>
#include "huajiao_config.hpp"
#include "huajiao.hpp"
#include "utils/md5.hpp"
#include "utils/rc4.hpp"
#include "utils/gzip.hpp"
#include <json11/json11.hpp>
#include "utils/log.hpp"

bool platform_huajiao::is_room_valid(std::string roomid)
{
    if (!str_is_num(roomid) || roomid.length() != 9) {
        return false;
    }
    return true;
}

void platform_huajiao::start(std::string roomid)
{
    conn_info.roomId = std::move(roomid);
    m_roomid = conn_info.roomId;
    std::error_code ec;
    auto conn = client.get_connection(huajiao_config.wsServer, ec);
    if (CLIENT_REPORT_WHEN_ERROR(ec)) {
        return;
    }
    conn->set_open_handler(std::bind(&platform_huajiao::on_client_open,
                                     std::dynamic_pointer_cast<platform_huajiao>(shared_from_this()),
                                     std::placeholders::_1));
    conn->set_message_handler(std::bind(&platform_huajiao::on_client_message,
                                        std::dynamic_pointer_cast<platform_huajiao>(shared_from_this()),
                                        std::placeholders::_1,
                                        std::placeholders::_2));
    conn->set_close_handler(std::bind(&platform_huajiao::on_client_close,
                                      std::dynamic_pointer_cast<platform_huajiao>(shared_from_this()),
                                      std::placeholders::_1));
    client.connect(conn);
    chdl = conn->get_handle();
}

void platform_huajiao::close()
{
    std::error_code ec;
    client.close(chdl, websocketpp::close::status::normal, "close", ec);
    CLIENT_REPORT_WHEN_ERROR(ec);
}

void platform_huajiao::on_client_open(connection_hdl hdl)
{
    auto hspacket = new_handshake_packet();
    std::error_code ec;
    client.send(hdl, hspacket.data(), hspacket.size(), opcode::BINARY, ec);
    CLIENT_CLOSE_AND_REPORT_WHEN_ERROR_WS(ec, chdl);
}

void platform_huajiao::on_client_message(connection_hdl hdl, message_ptr msg)
{
    if (msg->get_opcode() != opcode::BINARY) {
        return;
    }
    auto text_msg = handle_binary_message(msg->get_payload().data(), msg->get_payload().size());
    std::error_code ec;
    if (conn_info.handshake && !conn_info.bLogin) {
        auto loginpacket = new_login_packet();
        client.send(hdl, loginpacket.data(), loginpacket.size(), opcode::BINARY, ec);
        CLIENT_CLOSE_AND_REPORT_WHEN_ERROR_WS(ec, chdl);
        return;
    }
    if (conn_info.handshake && conn_info.bLogin && !conn_info.bJoin) {
        auto jcpacket = new_join_chatroom_packet();
        conn_info.bJoin = true;
        client.send(hdl, jcpacket.data(), jcpacket.size(), opcode::BINARY, ec);
        CLIENT_CLOSE_AND_REPORT_WHEN_ERROR_WS(ec, chdl);
        return;
    }
    if (!text_msg.empty()) {
        publish(text_msg);
    }
}

void platform_huajiao::on_client_close(connection_hdl hdl)
{
    (void)(hdl);
    publish("huajiao danmu connection closed!");
    m_close_callback(std::string().append(m_platform_name).append("_").append(m_roomid));
}

void platform_huajiao::new_request_message(PACKET_TYPE msgid, void* req_object, qihoo::protocol::messages::Message* message)
{
    message->set_msgid(msgid);
    message->set_sn(conn_info.sn);
    message->set_sender(conn_info.sender);
    message->set_sender_type(huajiao_config.senderType);

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

std::vector<uint8_t> platform_huajiao::new_handshake_packet()
{
    auto* init_login_req = new qihoo::protocol::messages::InitLoginReq();
    init_login_req->set_client_ram(conn_info.client_ram);
    init_login_req->set_sig(conn_info.sign);

    qihoo::protocol::messages::Message msg;
    new_request_message(InitLoginReq, init_login_req, &msg);

    std::string msgc = msg.SerializeAsString();

    char szHeader[12] = { 113, 104, 16, 101, 8, 32, 0, 0, 0, 0, 0, 0 };

    uint32_t length = uint32_t(msgc.length() + 12 + 4);
    uint32_t ulength = swap_uint32(length);

    std::vector<uint8_t> result(length);
    rc4_ptr(reinterpret_cast<const uint8_t*>(msgc.data()), msgc.length(),
        reinterpret_cast<const uint8_t*>(huajiao_config.defaultKey.data()),
        huajiao_config.defaultKey.length(), result.data() + 12 + 4);
    memcpy(result.data(), szHeader, 12);
    memcpy(result.data() + 12, &ulength, 4);

    return result;
}

std::vector<uint8_t> platform_huajiao::new_login_packet()
{
    auto* login = new qihoo::protocol::messages::LoginReq();
    login->set_app_id(huajiao_config.appId);
    login->set_server_ram(conn_info.server_ram);

    std::stringstream secret_ram_stream;
    secret_ram_stream.write(conn_info.server_ram.c_str(), conn_info.server_ram.length());
    secret_ram_stream.write(random_string(8).c_str(), 8);

    std::string secret_ram = rc4_str((const uint8_t*)secret_ram_stream.str().data(),
        secret_ram_stream.str().length(),
        (const uint8_t*)conn_info.password.data(),
        conn_info.password.length());
    login->set_secret_ram(secret_ram);

    std::string verf_code = make_verf_code(conn_info.userid);
    login->set_verf_code(verf_code);

    login->set_net_type(4);
    login->set_mobile_type(MOBILE_PC);
    login->set_not_encrypt(true);
    login->set_platform("h5");

    qihoo::protocol::messages::Message msg;
    new_request_message(LoginReq, login, &msg);
    std::string msgc = msg.SerializeAsString();
    uint32_t length = msgc.length() + 4;
    uint32_t ulength = swap_uint32(length);
    std::vector<uint8_t> result(length);
    rc4_ptr((const uint8_t*)msgc.data(), msgc.length(),
        (const uint8_t*)huajiao_config.defaultKey.data(),
        huajiao_config.defaultKey.length(), result.data() + 4);

    memcpy(result.data(), &ulength, 4);

    return result;
}

std::vector<uint8_t> platform_huajiao::new_join_chatroom_packet()
{
    auto* room = new qihoo::protocol::chatroom::ChatRoom();
    room->set_roomid(conn_info.roomId);

    auto* applyjoinchatroomreq = new qihoo::protocol::chatroom::ApplyJoinChatRoomRequest();
    applyjoinchatroomreq->set_roomid(conn_info.roomId);
    applyjoinchatroomreq->set_userid_type(0);
    applyjoinchatroomreq->set_allocated_room(room);

    auto* to_server_data = new qihoo::protocol::chatroom::ChatRoomUpToServer();
    to_server_data->set_payloadtype(102);
    to_server_data->set_allocated_applyjoinchatroomreq(applyjoinchatroomreq);

    auto* packet = new qihoo::protocol::chatroom::ChatRoomPacket();

    std::string uuid = md5_str(random_string(20));
    packet->set_uuid(uuid);

    packet->set_client_sn(conn_info.sn);
    packet->set_roomid(conn_info.roomId);
    packet->set_appid(huajiao_config.appId);
    packet->set_allocated_to_server_data(to_server_data);

    auto* service_req = new qihoo::protocol::messages::Service_Req();
    service_req->set_service_id(10000006);
    service_req->set_request(packet->SerializePartialAsString());

    qihoo::protocol::messages::Message msg;
    new_request_message(Service_Req, service_req, &msg);

    std::string msgc = msg.SerializeAsString();

    uint32_t length = msg.ByteSizeLong() + 4;
    uint32_t ulength = swap_uint32(length);

    std::vector<uint8_t> result(length);
    memcpy(result.data(), &ulength, 4);
    memcpy(result.data() + 4, msgc.data(), msgc.length());

    return result;
}

std::string platform_huajiao::handle_binary_message(const void *data, size_t size)
{
    if (conn_info.handshake) {
        if (conn_info.bLogin) {
            return parse_message_packet(data, size);
        } else {
            parse_login_response_packet(data, size);
        }
    } else {
        parse_handshake_packet(data, size);
    }
    return std::string();
}

std::string platform_huajiao::parse_message_packet(const void *data, size_t size)
{
    std::string dm_msg;
    std::stringstream sstream;
    if (size == 4 && (*(uint32_t*)data) == 0) {
        std::cout << "==================HeartbeatPack=================="
                  << std::endl;
    } else {
        char* pData = (char*)data + 4;

        qihoo::protocol::messages::Message message;
        parse_address_book_message(pData, size - 4, &message);

        switch (message.msgid()) {
        case LoginResp:
            parse_login_response_packet(data, size);
            break;
        case Service_Resp:
            parse_service_resp(message);
            break;
        case NewMessageNotify:
            dm_msg.append(parse_new_message_notify(message));
            break;
        default:
            sstream << "[Unknown unpack] : msgid = " << message.msgid() << std::endl;
            dm_msg.append(sstream.str());
            message.PrintDebugString();
            break;
        }
    }
    return dm_msg;
}

std::string platform_huajiao::parse_new_message_notify(const qihoo::protocol::messages::Message& message)
{
    std::stringstream sstream;
    auto notify = message.notify();

    qihoo::protocol::chatroom::ChatRoomPacket newmsgnotify;
    parse_chat_room_message(notify.newinfo_ntf().info_content(), &newmsgnotify);

    qihoo::protocol::chatroom::ChatRoomDownToUser user_data;
    user_data = newmsgnotify.to_user_data();

    uint32_t payloadtype = user_data.payloadtype();

//    sstream << "==================NewMessageNotify==================" << std::endl;;
//    sstream << "msgid            = " << message.msgid() << std::endl;
//    sstream << "payloadtype      = " << payloadtype << std::endl;

    if (user_data.result() == SuccessFul) {
        if (payloadtype == NewmsgNotify) {
//            sstream << "[ChatRoomNewMsg Response] ->" << std::endl;

            qihoo::protocol::chatroom::ChatRoomNewMsg chatroom_newmsg;
            chatroom_newmsg = user_data.newmsgnotify();

            int msgtype = chatroom_newmsg.msgtype();
//            sstream << "msgtype          = " << msgtype << std::endl;

            if (msgtype == 0 && chatroom_newmsg.memcount()) {
                sstream << parse_json_message(chatroom_newmsg.msgcontent());
            } else {
                sstream << "[Unknow Msgtype] into = " << chatroom_newmsg.DebugString() << std::endl;
            }
        } else if (payloadtype == MemberJoinNotify) {
            /* NOOOOOOOOOOP */
//            sstream << "[MemberJoinChatRoomNotify Response] -> "
//                      << std::endl;

//            qihoo::protocol::chatroom::MemberJoinChatRoomNotify memberjoinnotify;
//            memberjoinnotify = user_data.memberjoinnotify();
//            qihoo::protocol::chatroom::ChatRoom room;
//            room = memberjoinnotify.room();

//            std::string value = room.properties(1).value();
//            std::string userdata = room.members(0).userdata();

//            sstream << "userdata     = " << userdata << std::endl;
//            sstream << "value        = " << value << std::endl;
        } else if (payloadtype == MemberQuitNotify) {
            /* NOOOOOOOOOOOOP */
//            sstream << "[MemberQuitChatRoomNotify Response] -> "
//                      << std::endl;

//            qihoo::protocol::chatroom::MemberQuitChatRoomNotify memberquitnotify;
//            memberquitnotify = user_data.memberquitnotify();
//            qihoo::protocol::chatroom::ChatRoom room;
//            room = memberquitnotify.room();

//            std::string userId = room.members(0).userid();
//            std::string value = room.properties(0).value();

//            sstream << "userId       = " << userId << std::endl;
//            sstream << "value        = " << value << std::endl;
        } else if (payloadtype == MemberGzipNotify && user_data.multinotify().size() > 0) {
//            sstream << "[MemberGzipNotify Response] -> "
//                      << std::endl;

//            sstream << "MultiNotify Count = " << user_data.multinotify().size() << std::endl
//                      << std::endl;

            for (int i = 0; i < user_data.multinotify().size(); i++) {
                int type = user_data.multinotify(i).type();
                int regmemcount = user_data.multinotify(i).regmemcount();
                int memcount = user_data.multinotify(i).memcount();
                std::string data = user_data.multinotify(i).data();

//                sstream << "type             = " << type << std::endl;
//                sstream << "regmemcount      = " << regmemcount << std::endl;
//                sstream << "memcount         = " << memcount << std::endl
//                          << std::endl;

                std::vector<uint8_t> unpack;
                std::vector<uint8_t> src_data;
                src_data.assign(data.begin(), data.end());
                if (ungzip(src_data, unpack)) {
                    qihoo::protocol::chatroom::ChatRoomNewMsg newMsg;
                    qihoo::protocol::chatroom::CRUser crUser;

                    newMsg.ParseFromArray(unpack.data(), unpack.size());
                    crUser = newMsg.sender();

                    std::string roomid = newMsg.roomid();
                    int32_t msgType = newMsg.msgtype();
                    uint32_t msgId = newMsg.msgid();
                    std::string msgcontent = newMsg.msgcontent();
                    std::string userid = crUser.userid();
                    std::string name = crUser.name();
                    std::string userData = crUser.userdata();

//                    sstream << "roomid           = " << roomid << std::endl;
//                    sstream << "userid           = " << userid << std::endl;
//                    sstream << "userName         = " << name << std::endl;
//                    sstream << "msgType          = " << msgType << std::endl;
//                    sstream << "msgId            = " << msgId << std::endl;
//                    //std::cout << WHITE << "	msgContent      = " << msgcontent << std::endl;
//                    sstream << "userData         = " << userData << std::endl;

                    sstream << parse_json_message(msgcontent);
                }
            }
        } else {
            sstream << "[Unknow PayloadType] into = " << user_data.DebugString() << std::endl;
        }
    } else {
        sstream << "[Unpacket Error] debug = " << notify.DebugString() << std::endl;
    }
    return sstream.str();
}

void platform_huajiao::parse_service_resp(const qihoo::protocol::messages::Message& message)
{
    auto resp = message.resp();
    auto service_resp = resp.service_resp();
    auto response = service_resp.response();

    qihoo::protocol::chatroom::ChatRoomPacket packet;
    parse_chat_room_message(response, &packet);

    auto user_data = packet.to_user_data();
    uint32_t service_id = service_resp.service_id();
    uint32_t payloadtype = user_data.payloadtype();
    std::string reason = user_data.reason();

    std::stringstream sstream;

//    sstream << "==================Service_Resp==================" << std::endl;
//    sstream << "msgid            = " << message.msgid() << std::endl;
//    sstream << "service_id       = " << service_id << std::endl;
//    sstream << "payloadtype      = " << payloadtype << std::endl;
//    sstream << "reason           = " << reason << std::endl;

    if (payloadtype == ApplyJoinChatRoomResp) {
        if (user_data.result() == SuccessFul) {
            qihoo::protocol::chatroom::ApplyJoinChatRoomResponse applyjoinchatroomresp;
            applyjoinchatroomresp = user_data.applyjoinchatroomresp();
            qihoo::protocol::chatroom::ChatRoom room;
            room = applyjoinchatroomresp.room();

//            std::string roomid = room.roomid();
//            std::string userid = room.members(0).userid();
//            std::string roomtype = room.roomtype();

//            sstream << "[JoinChatRoom Response] -> "
//                      << std::endl;

//            sstream << "roomid           = " << roomid << std::endl;
//            sstream << "userid           = " << userid << std::endl;
//            sstream << "roomtype         = " << roomtype << std::endl;

//            if (!room.partnerdata().empty()) {
//                std::string partnerdata = room.partnerdata();
//                sstream << "partnerdata      = " << partnerdata << std::endl;
//            }
        }
    } else if (payloadtype == QuitChatRoomResp) {
        qihoo::protocol::chatroom::QuitChatRoomResponse quitchatroomresp;
        quitchatroomresp = user_data.quitchatroomresp();
        qihoo::protocol::chatroom::ChatRoom room;
        room = quitchatroomresp.room();

//        sstream << "[QuitChatRoom Response] " << std::endl;
    } else {
        sstream << "[Unknow PayloadType] into = " << user_data.DebugString() << std::endl;
    }
}

void platform_huajiao::parse_address_book_message(const void* data, uint32_t size, qihoo::protocol::messages::Message* message)
{
    message->ParseFromArray(data, size);
    //message.PrintDebugString();
}

//_parseChatroomMessage
void platform_huajiao::parse_chat_room_message(const std::string& data, qihoo::protocol::chatroom::ChatRoomPacket* message)
{
    message->ParseFromString(data);
    //message.PrintDebugString();
}

//_parseChatroomNewMessage
void platform_huajiao::parse_chat_room_new_message(const void* data, uint32_t size, qihoo::protocol::chatroom::ChatRoomNewMsg* message)
{
    message->ParseFromArray(data, size);
    //message.PrintDebugString();
}

void platform_huajiao::parse_handshake_packet(const void *data, size_t size)
{
    uint32_t length = size - 6;
    std::stringstream sstream;

    std::vector<uint8_t> out_result = rc4_vector((const uint8_t*)data + 6, length, reinterpret_cast<const uint8_t*>(huajiao_config.defaultKey.data()), huajiao_config.defaultKey.length());

    qihoo::protocol::messages::Message message;
    parse_address_book_message(out_result.data(), out_result.size(), &message);

    if (message.msgid() == InitLoginResp) {
        auto response = message.resp().init_login_resp();

        conn_info.server_ram = response.server_ram();
        conn_info.client_ram = response.client_ram();
        conn_info.handshake = true;
    } else {
        sstream << "[Unpacket Error] response msgid exception,msgid = " << message.msgid() << std::endl;
    }
}

void platform_huajiao::parse_login_response_packet(const void *data, size_t size)
{
    uint32_t length = size - 4;
    std::stringstream sstream;

    std::vector<uint8_t> out_result = rc4_vector((const uint8_t*)data + 4, length, reinterpret_cast<const uint8_t*>(conn_info.password.data()), conn_info.password.length());
    qihoo::protocol::messages::Message message;
    parse_address_book_message(out_result.data(), out_result.size(), &message);

    if (message.msgid() == LoginResp) {
//        sstream << "==================LoginPack -> LoginResp==================" << std::endl;
        auto login = message.resp().login();

//        sstream << "session_id           =   " << login.session_id() << std::endl;
//        sstream << "session_key          =   " << login.session_key() << std::endl;
//        sstream << "client_login_ip      =   " << login.client_login_ip() << std::endl;
//        sstream << "serverip             =   " << login.serverip() << std::endl;

        conn_info.session = login.session_key();
        conn_info.bLogin = true;
    } else {
        sstream << "[Unpacket Error] response msgid exception,msgid = " << message.msgid() << std::endl;
    }
}
std::string platform_huajiao::parse_json_message(const std::string& message)
{
    std::stringstream sstream;
    std::string err;
    auto json = json11::Json::parse(message, err);
    if (err.empty()) {
        std::string roomid = json["roomid"].string_value();
        int type = json["type"].int_value();
        std::string text = json["text"].string_value();

//        sstream << "roomid           = " << roomid << std::endl;
//        sstream << "type             = " << type << std::endl;

        if (type == 16) {
            int liveid = json["extends"]["liveid"].int_value();
            double userid = json["extends"]["userid"].number_value();
            sstream << "uid:"
                      << std::setiosflags(std::ios::fixed)
                      << std::setprecision(0)
                      << userid
                      << std::endl;
        } else {
            std::string liveid = json["extends"]["liveid"].string_value();
            std::string userid = json["extends"]["userid"].string_value();
            sstream << "uid:" << userid << std::endl;
        }

        std::string nickname = json["extends"]["nickname"].string_value();
        int level = json["extends"]["level"].int_value();
        int watches = json["extends"]["watches"].int_value();
        std::string distance = json["extends"]["distance"].string_value();
        int exp = json["extends"]["exp"].int_value();
        std::string credentials = json["verifiedinfo"]["credentials"].string_value();
        std::string realname = json["verifiedinfo"]["realname"].string_value();
        std::string avatar = json["extends"]["avatar"].string_value();

        sstream << "nickname:" << nickname << std::endl;
        sstream << "text:" << text << std::endl;
    }
    return sstream.str();
}
