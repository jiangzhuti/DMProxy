#include "json11/json11.hpp"
#include "utils/gzip.hpp"
#include "utils/rc4.hpp"
#include "parse_pack.hpp"

#include <cstdint>
#include <iomanip>
#include <iostream>
#include <sstream>

std::string HandleBinaryMessage(const void* data, uint32_t size, huajiao_conn_info_t &ci)
{
    if (ci.handshake) {
        if (ci.bLogin) {
            return parse_message_pack(data, size, ci);
        } else {
            return parse_longin_response_pack(data, size, ci);
        }
    } else {
        return parse_hand_shake_pack(data, size, ci);
    }
}

//_processMessagePack
std::string parse_message_pack(const void* data, uint32_t size, huajiao_conn_info_t &ci)
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
            dm_msg.append(parse_longin_response_pack(data, size, ci));
            break;
        case Service_Resp:
            dm_msg.append(parse_service_resp(message));
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

//_processNewMessageNotifyMessage
std::string parse_new_message_notify(const qihoo::protocol::messages::Message& message)
{
    std::stringstream sstream;
    auto notify = message.notify();

    qihoo::protocol::chatroom::ChatRoomPacket newmsgnotify;
    parse_chat_room_message(notify.newinfo_ntf().info_content(), &newmsgnotify);

    qihoo::protocol::chatroom::ChatRoomDownToUser user_data;
    user_data = newmsgnotify.to_user_data();

    uint32_t payloadtype = user_data.payloadtype();

    sstream << "==================NewMessageNotify==================" << std::endl;;
    sstream << "msgid            = " << message.msgid() << std::endl;
    sstream << "payloadtype      = " << payloadtype << std::endl;

    if (user_data.result() == SuccessFul) {
        if (payloadtype == NewmsgNotify) {
            sstream << "[ChatRoomNewMsg Response] ->" << std::endl;

            qihoo::protocol::chatroom::ChatRoomNewMsg chatroom_newmsg;
            chatroom_newmsg = user_data.newmsgnotify();

            int msgtype = chatroom_newmsg.msgtype();
            sstream << "msgtype          = " << msgtype << std::endl;

            if (msgtype == 0 && chatroom_newmsg.memcount()) {
                sstream << parse_json_message(chatroom_newmsg.msgcontent());
            } else {
                sstream << "[Unknow Msgtype] into = " << chatroom_newmsg.DebugString() << std::endl;
            }
        } else if (payloadtype == MemberJoinNotify) {
            sstream << "[MemberJoinChatRoomNotify Response] -> "
                      << std::endl;

            qihoo::protocol::chatroom::MemberJoinChatRoomNotify memberjoinnotify;
            memberjoinnotify = user_data.memberjoinnotify();
            qihoo::protocol::chatroom::ChatRoom room;
            room = memberjoinnotify.room();

            std::string value = room.properties(1).value();
            std::string userdata = room.members(0).userdata();

            sstream << "userdata     = " << userdata << std::endl;
            sstream << "value        = " << value << std::endl;
        } else if (payloadtype == MemberQuitNotify) {
            sstream << "[MemberQuitChatRoomNotify Response] -> "
                      << std::endl;

            qihoo::protocol::chatroom::MemberQuitChatRoomNotify memberquitnotify;
            memberquitnotify = user_data.memberquitnotify();
            qihoo::protocol::chatroom::ChatRoom room;
            room = memberquitnotify.room();

            std::string userId = room.members(0).userid();
            std::string value = room.properties(0).value();

            sstream << "userId       = " << userId << std::endl;
            sstream << "value        = " << value << std::endl;
        } else if (payloadtype == MemberGzipNotify && user_data.multinotify().size() > 0) {
            sstream << "[MemberGzipNotify Response] -> "
                      << std::endl;

            sstream << "MultiNotify Count = " << user_data.multinotify().size() << std::endl
                      << std::endl;

            for (int i = 0; i < user_data.multinotify().size(); i++) {
                int type = user_data.multinotify(i).type();
                int regmemcount = user_data.multinotify(i).regmemcount();
                int memcount = user_data.multinotify(i).memcount();
                std::string data = user_data.multinotify(i).data();

                sstream << "type             = " << type << std::endl;
                sstream << "regmemcount      = " << regmemcount << std::endl;
                sstream << "memcount         = " << memcount << std::endl
                          << std::endl;

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

                    sstream << "roomid           = " << roomid << std::endl;
                    sstream << "userid           = " << userid << std::endl;
                    sstream << "userName         = " << name << std::endl;
                    sstream << "msgType          = " << msgType << std::endl;
                    sstream << "msgId            = " << msgId << std::endl;
                    //std::cout << WHITE << "	msgContent      = " << msgcontent << std::endl;
                    sstream << "userData         = " << userData << std::endl;

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

//_processService_RespMessage
std::string parse_service_resp(const qihoo::protocol::messages::Message& message)
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

    sstream << "==================Service_Resp==================" << std::endl;
    sstream << "msgid            = " << message.msgid() << std::endl;
    sstream << "service_id       = " << service_id << std::endl;
    sstream << "payloadtype      = " << payloadtype << std::endl;
    sstream << "reason           = " << reason << std::endl;

    if (payloadtype == ApplyJoinChatRoomResp) {
        if (user_data.result() == SuccessFul) {
            qihoo::protocol::chatroom::ApplyJoinChatRoomResponse applyjoinchatroomresp;
            applyjoinchatroomresp = user_data.applyjoinchatroomresp();
            qihoo::protocol::chatroom::ChatRoom room;
            room = applyjoinchatroomresp.room();

            std::string roomid = room.roomid();
            std::string userid = room.members(0).userid();
            std::string roomtype = room.roomtype();

            sstream << "[JoinChatRoom Response] -> "
                      << std::endl;

            sstream << "roomid           = " << roomid << std::endl;
            sstream << "userid           = " << userid << std::endl;
            sstream << "roomtype         = " << roomtype << std::endl;

            if (!room.partnerdata().empty()) {
                std::string partnerdata = room.partnerdata();
                sstream << "partnerdata      = " << partnerdata << std::endl;
            }
        }
    } else if (payloadtype == QuitChatRoomResp) {
        qihoo::protocol::chatroom::QuitChatRoomResponse quitchatroomresp;
        quitchatroomresp = user_data.quitchatroomresp();
        qihoo::protocol::chatroom::ChatRoom room;
        room = quitchatroomresp.room();

        sstream << "[QuitChatRoom Response] " << std::endl;
    } else {
        sstream << "[Unknow PayloadType] into = " << user_data.DebugString() << std::endl;
    }
    return sstream.str();
}

//_parseAddressBookMessage
void parse_address_book_message(const void* data, uint32_t size, qihoo::protocol::messages::Message* message)
{
    message->ParseFromArray(data, size);
    //message.PrintDebugString();
}

//_parseChatroomMessage
void parse_chat_room_message(const std::string& data, qihoo::protocol::chatroom::ChatRoomPacket* message)
{
    message->ParseFromString(data);
    //message.PrintDebugString();
}

//_parseChatroomNewMessage
void parse_chat_room_new_message(const void* data, uint32_t size, qihoo::protocol::chatroom::ChatRoomNewMsg* message)
{
    message->ParseFromArray(data, size);
    //message.PrintDebugString();
}

//_processHandShakePack
std::string parse_hand_shake_pack(const void* data, uint32_t size, huajiao_conn_info_t &ci)
{
    uint32_t length = size - 6;
    std::stringstream sstream;

    std::vector<uint8_t> out_result = rc4_vector((const uint8_t*)data + 6, length, reinterpret_cast<const uint8_t*>(huajiao_config.defaultKey.data()), huajiao_config.defaultKey.length());

    qihoo::protocol::messages::Message message;
    parse_address_book_message(out_result.data(), out_result.size(), &message);

    if (message.msgid() == InitLoginResp) {
        sstream << "==================HandShakePack -> InitLoginResp==================" << std::endl;;
        auto response = message.resp().init_login_resp();

        sstream << "server_ram       = " << response.server_ram() << std::endl;
        sstream << "client_ram       = " << response.client_ram() << std::endl;

        ci.server_ram = response.server_ram();
        ci.client_ram = response.client_ram();
        ci.handshake = true;
    } else {
        sstream << "[Unpacket Error] response msgid exception,msgid = " << message.msgid() << std::endl;
    }
    return sstream.str();
}

//_processLoginPack
std::string parse_longin_response_pack(const void* data, uint32_t size, huajiao_conn_info_t &ci)
{
    uint32_t length = size - 4;
    std::stringstream sstream;

    std::vector<uint8_t> out_result = rc4_vector((const uint8_t*)data + 4, length, reinterpret_cast<const uint8_t*>(ci.password.data()), ci.password.length());
    qihoo::protocol::messages::Message message;
    parse_address_book_message(out_result.data(), out_result.size(), &message);

    if (message.msgid() == LoginResp) {
        sstream << "==================LoginPack -> LoginResp==================" << std::endl;
        auto login = message.resp().login();

        sstream << "session_id           =   " << login.session_id() << std::endl;
        sstream << "session_key          =   " << login.session_key() << std::endl;
        sstream << "client_login_ip      =   " << login.client_login_ip() << std::endl;
        sstream << "serverip             =   " << login.serverip() << std::endl;

        ci.session = login.session_key();
        ci.bLogin = true;
    } else {
        sstream << "[Unpacket Error] response msgid exception,msgid = " << message.msgid() << std::endl;
    }
    return sstream.str();
}

//_processJsonMessagePack
std::string parse_json_message(const std::string& message)
{
    std::stringstream sstream;
    sstream << "==================JsonMsgContent==================" << std::endl;;
    std::string err;
    auto json = json11::Json::parse(message, err);
    if (err.empty()) {
        std::string roomid = json["roomid"].string_value();
        int type = json["type"].int_value();
        std::string text = json["text"].string_value();

        sstream << "roomid           = " << roomid << std::endl;
        sstream << "type             = " << type << std::endl;

        sstream << "text             = " << text << std::endl;

        if (type == 16) {
            int liveid = json["extends"]["liveid"].int_value();
            double userid = json["extends"]["userid"].number_value();
            sstream << "liveid           = " << liveid << std::endl;
            sstream << "userid           = "
                      << std::setiosflags(std::ios::fixed)
                      << std::setprecision(0)
                      << userid
                      << std::endl;
        } else {
            std::string liveid = json["extends"]["liveid"].string_value();
            std::string userid = json["extends"]["userid"].string_value();
            sstream << "liveid           = " << liveid << std::endl;
            sstream << "userid           = " << userid << std::endl;
        }

        std::string nickname = json["extends"]["nickname"].string_value();
        int level = json["extends"]["level"].int_value();
        int watches = json["extends"]["watches"].int_value();
        std::string distance = json["extends"]["distance"].string_value();
        int exp = json["extends"]["exp"].int_value();
        std::string credentials = json["verifiedinfo"]["credentials"].string_value();
        std::string realname = json["verifiedinfo"]["realname"].string_value();
        std::string avatar = json["extends"]["avatar"].string_value();

        sstream << "nickname         = " << nickname << std::endl;
        sstream << "level            = " << level << std::endl;
        sstream << "watches          = " << watches << std::endl;
        sstream << "distance         = " << distance << std::endl;
        sstream << "exp              = " << exp << std::endl;
        sstream << "credentials      = " << credentials << std::endl;
        sstream << "realname         = " << realname << std::endl;
        sstream << "avatar           = " << avatar << std::endl;
    }
    return sstream.str();
}
