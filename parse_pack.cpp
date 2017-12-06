#include "parse_pack.hpp"
#include "global.hpp"
#include "json11/json11.hpp"
#include "utils/gzip.hpp"

#include <iomanip>
#include <iostream>

void HandleBinaryMessage(const void* data, int size)
{
    if (g_user_info.handshake) {
        if (g_user_info.bLogin) {
            parse_message_pack(data, size);
        } else {
            parse_longin_response_pack(data, size);
        }
    } else {
        parse_hand_shake_pack(data, size);
    }
}

//_processMessagePack
void parse_message_pack(const void* data, int size)
{
    if (size == 4 && (*(int*)data) == 0) {
        std::cout << "\n==================HeartbeatPack==================\n\n"
                  << std::endl;
    } else {
        char* pData = (char*)data + 4;

        qihoo::protocol::messages::Message message;
        parse_address_book_message(pData, size - 4, &message);

        switch (message.msgid()) {
        case LoginResp:
            parse_longin_response_pack(data, size);
            break;
        case Service_Resp:
            parse_service_resp(message);
            break;
        case NewMessageNotify:
            parse_new_message_notify(message);
            break;
        default:
            std::cerr << "[Unknown unpack] : msgid = " << message.msgid() << std::endl;
            message.PrintDebugString();
            break;
        }
    }
}

//_processNewMessageNotifyMessage
void parse_new_message_notify(const qihoo::protocol::messages::Message& message)
{
    auto notify = message.notify();

    qihoo::protocol::chatroom::ChatRoomPacket newmsgnotify;
    parse_chat_room_message(notify.newinfo_ntf().info_content(), &newmsgnotify);

    qihoo::protocol::chatroom::ChatRoomDownToUser user_data;
    user_data = newmsgnotify.to_user_data();

    int payloadtype = user_data.payloadtype();

    std::cout << "\n==================NewMessageNotify==================\n"
              << std::endl;
    std::cout << "msgid            = " << message.msgid() << std::endl;
    std::cout << "payloadtype      = " << payloadtype << std::endl;

    if (user_data.result() == SuccessFul) {
        if (payloadtype == NewmsgNotify) {
            std::cout << "\n[ChatRoomNewMsg Response] ->\n"
                      << std::endl;

            qihoo::protocol::chatroom::ChatRoomNewMsg chatroom_newmsg;
            chatroom_newmsg = user_data.newmsgnotify();

            int msgtype = chatroom_newmsg.msgtype();
            std::cout << "msgtype          = " << msgtype << std::endl;

            if (msgtype == 0 && chatroom_newmsg.memcount()) {
                parse_json_message(chatroom_newmsg.msgcontent());
            } else {
                std::cerr << "\n[Unknow Msgtype] into = " << chatroom_newmsg.DebugString() << std::endl;
            }
        } else if (payloadtype == MemberJoinNotify) {
            std::cout << "\n[MemberJoinChatRoomNotify Response] -> \n"
                      << std::endl;

            qihoo::protocol::chatroom::MemberJoinChatRoomNotify memberjoinnotify;
            memberjoinnotify = user_data.memberjoinnotify();
            qihoo::protocol::chatroom::ChatRoom room;
            room = memberjoinnotify.room();

            std::string value = room.properties(1).value();
            std::string userdata = room.members(0).userdata();

            std::cout << "userdata     = " << userdata << std::endl;
            std::cout << "value        = " << value << std::endl;
        } else if (payloadtype == MemberQuitNotify) {
            std::cout << "\n[MemberQuitChatRoomNotify Response] -> \n"
                      << std::endl;

            qihoo::protocol::chatroom::MemberQuitChatRoomNotify memberquitnotify;
            memberquitnotify = user_data.memberquitnotify();
            qihoo::protocol::chatroom::ChatRoom room;
            room = memberquitnotify.room();

            std::string userId = room.members(0).userid();
            std::string value = room.properties(0).value();

            std::cout << "userId       = " << userId << std::endl;
            std::cout << "value        = " << value << std::endl;
        } else if (payloadtype == MemberGzipNotify && user_data.multinotify().size() > 0) {
            std::cout << "\n[MemberGzipNotify Response] -> \n"
                      << std::endl;

            std::cout << "MultiNotify Count = " << user_data.multinotify().size() << std::endl
                      << std::endl;

            for (int i = 0; i < user_data.multinotify().size(); i++) {
                int type = user_data.multinotify(i).type();
                int regmemcount = user_data.multinotify(i).regmemcount();
                int memcount = user_data.multinotify(i).memcount();
                std::string data = user_data.multinotify(i).data();

                std::cout << "type             = " << type << std::endl;
                std::cout << "regmemcount      = " << regmemcount << std::endl;
                std::cout << "memcount         = " << memcount << std::endl
                          << std::endl;

                std::vector<unsigned char> unpack;
                std::vector<unsigned char> src_data;
                src_data.assign(data.begin(), data.end());
                if (ungzip(src_data, unpack)) {
                    qihoo::protocol::chatroom::ChatRoomNewMsg newMsg;
                    qihoo::protocol::chatroom::CRUser crUser;

                    newMsg.ParseFromArray(&unpack[0], unpack.size());
                    crUser = newMsg.sender();

                    std::string roomid = newMsg.roomid();
                    int msgType = newMsg.msgtype();
                    int msgId = newMsg.msgid();
                    std::string msgcontent = newMsg.msgcontent();
                    std::string userid = crUser.userid();
                    std::string name = crUser.name();
                    std::string userData = crUser.userdata();

                    std::cout << "roomid           = " << roomid << std::endl;
                    std::cout << "userid           = " << userid << std::endl;
                    std::cout << "userName         = " << name << std::endl;
                    std::cout << "msgType          = " << msgType << std::endl;
                    std::cout << "msgId            = " << msgId << std::endl;
                    //std::cout << WHITE << "	msgContent      = " << msgcontent << std::endl;
                    std::cout << "userData         = " << userData << std::endl;

                    parse_json_message(msgcontent);
                }
            }
        } else {
            std::cerr << "\n[Unknow PayloadType] into = " << user_data.DebugString() << std::endl;
        }
    } else {
        std::cerr << "\n[Unpacket Error] debug = " << notify.DebugString() << std::endl;
    }
}

//_processService_RespMessage
void parse_service_resp(const qihoo::protocol::messages::Message& message)
{
    auto resp = message.resp();
    auto service_resp = resp.service_resp();
    auto response = service_resp.response();

    qihoo::protocol::chatroom::ChatRoomPacket packet;
    parse_chat_room_message(response, &packet);

    auto user_data = packet.to_user_data();
    int service_id = service_resp.service_id();
    int payloadtype = user_data.payloadtype();
    std::string reason = user_data.reason();

    std::cout << "\n==================Service_Resp==================\n"
              << std::endl;
    std::cout << "msgid            = " << message.msgid() << std::endl;
    std::cout << "service_id       = " << service_id << std::endl;
    std::cout << "payloadtype      = " << payloadtype << std::endl;
    std::cout << "reason           = " << reason << std::endl;

    if (payloadtype == ApplyJoinChatRoomResp) {
        if (user_data.result() == SuccessFul) {
            qihoo::protocol::chatroom::ApplyJoinChatRoomResponse applyjoinchatroomresp;
            applyjoinchatroomresp = user_data.applyjoinchatroomresp();
            qihoo::protocol::chatroom::ChatRoom room;
            room = applyjoinchatroomresp.room();

            std::string roomid = room.roomid();
            std::string userid = room.members(0).userid();
            std::string roomtype = room.roomtype();

            std::cout << "\n[JoinChatRoom Response] -> \n"
                      << std::endl;

            std::cout << "roomid           = " << roomid << std::endl;
            std::cout << "userid           = " << userid << std::endl;
            std::cout << "roomtype         = " << roomtype << std::endl;

            if (!room.partnerdata().empty()) {
                std::string partnerdata = room.partnerdata();
                std::cout << "partnerdata      = " << partnerdata << std::endl;
            }
        }
    } else if (payloadtype == QuitChatRoomResp) {
        qihoo::protocol::chatroom::QuitChatRoomResponse quitchatroomresp;
        quitchatroomresp = user_data.quitchatroomresp();
        qihoo::protocol::chatroom::ChatRoom room;
        room = quitchatroomresp.room();

        std::cout << "\n[QuitChatRoom Response] \n"
                  << std::endl;
    } else {
        std::cerr << "\n[Unknow PayloadType] into = " << user_data.DebugString() << std::endl;
    }
}

//_parseAddressBookMessage
void parse_address_book_message(const void* data, int size, qihoo::protocol::messages::Message* message)
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
void parse_chat_room_new_message(const void* data, int size, qihoo::protocol::chatroom::ChatRoomNewMsg* message)
{
    message->ParseFromArray(data, size);
    //message.PrintDebugString();
}

//_processHandShakePack
void parse_hand_shake_pack(const void* data, int size)
{
    int length = size - 6;

    char* szBuffer = (char*)malloc(length);
    memset(szBuffer, 0, length);
    memcpy(szBuffer, (char*)data + 6, length);

    std::string out_result;
    rc4_xx(szBuffer, g_config.defaultKey, &out_result);

    free(szBuffer);

    qihoo::protocol::messages::Message message;
    parse_address_book_message(out_result.c_str(), (int)out_result.length(), &message);

    if (message.msgid() == InitLoginResp) {
        std::cout << "\n==================HandShakePack -> InitLoginResp==================\n"
                  << std::endl;

        auto response = message.resp().init_login_resp();

        std::cout << "server_ram       = " << response.server_ram() << std::endl;
        std::cout << "client_ram       = " << response.client_ram() << std::endl;

        g_user_info.server_ram = response.server_ram();
        g_user_info.client_ram = response.client_ram();
        g_user_info.handshake = true;
    } else {
        std::cerr << "\n[Unpacket Error] response msgid exception,msgid = " << message.msgid() << std::endl;
    }
}

//_processLoginPack
void parse_longin_response_pack(const void* data, int size)
{
    int length = size - 4;

    char* szBuffer = (char*)malloc(length);
    memset(szBuffer, 0, length);
    memcpy(szBuffer, (char*)data + 4, length);

    std::string out_result;
    rc4_xx(szBuffer, g_user_info.password, &out_result);

    free(szBuffer);

    qihoo::protocol::messages::Message message;
    parse_address_book_message(out_result.c_str(), (int)out_result.length(), &message);

    if (message.msgid() == LoginResp) {
        std::cout << "\n==================LoginPack -> LoginResp==================\n"
                  << std::endl;

        auto login = message.resp().login();

        std::cout << "session_id           =   " << login.session_id() << std::endl;
        std::cout << "session_key          =   " << login.session_key() << std::endl;
        std::cout << "client_login_ip      =   " << login.client_login_ip() << std::endl;
        std::cout << "serverip             =   " << login.serverip() << std::endl;

        g_user_info.session = login.session_key();
        g_user_info.bLogin = true;
    } else {
        std::cerr << "\n[Unpacket Error] response msgid exception,msgid = " << message.msgid() << std::endl;
    }
}

//_processJsonMessagePack
void parse_json_message(const std::string& message)
{
    std::cout << "\n==================JsonMsgContent==================\n"
              << std::endl;

    std::string err;
    auto json = json11::Json::parse(message, err);
    if (err.empty()) {
        std::string roomid = json["roomid"].string_value();
        int type = json["type"].int_value();
        std::string text = json["text"].string_value();

        std::cout << "roomid           = " << roomid << std::endl;
        std::cout << "type             = " << type << std::endl;

        std::cout << "text             = " << text << std::endl;

        if (type == 16) {
            int liveid = json["extends"]["liveid"].int_value();
            double userid = json["extends"]["userid"].number_value();
            std::cout << "liveid           = " << liveid << std::endl;
            std::cout << "userid           = "
                      << std::setiosflags(std::ios::fixed)
                      << std::setprecision(0)
                      << userid
                      << std::endl;
        } else {
            std::string liveid = json["extends"]["liveid"].string_value();
            std::string userid = json["extends"]["userid"].string_value();
            std::cout << "liveid           = " << liveid << std::endl;
            std::cout << "userid           = " << userid << std::endl;
        }

        std::string nickname = json["extends"]["nickname"].string_value();
        int level = json["extends"]["level"].int_value();
        int watches = json["extends"]["watches"].int_value();
        std::string distance = json["extends"]["distance"].string_value();
        int exp = json["extends"]["exp"].int_value();
        std::string credentials = json["verifiedinfo"]["credentials"].string_value();
        std::string realname = json["verifiedinfo"]["realname"].string_value();
        std::string avatar = json["extends"]["avatar"].string_value();

        std::cout << "nickname         = " << nickname << std::endl;
        std::cout << "level            = " << level << std::endl;
        std::cout << "watches          = " << watches << std::endl;
        std::cout << "distance         = " << distance << std::endl;
        std::cout << "exp              = " << exp << std::endl;
        std::cout << "credentials      = " << credentials << std::endl;
        std::cout << "realname         = " << realname << std::endl;
        std::cout << "avatar           = " << avatar << std::endl;
    }
}
