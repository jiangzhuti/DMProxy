
enum PACKET_TYPE
{
    RestoreSessionReq = 100000,
    LoginReq = 100001,
    ChatReq = 100002,
    QueryInfoReq = 100003,
    GetInfoReq = 100004,
    LogoutReq = 100005,
    QueryUserStatusReq = 100007,
    QueryUserRegReq = 100008,
    InitLoginReq = 100009,
    ExQueryUserStatusReq = 100010,
    Service_Req = 100011,
    Ex1QueryUserStatusReq = 100012,
    QueryPeerMsgMaxIdReq = 100013,
    QueryConvSummaryReq = 100014,
    UpdateSessionReq = 100015,
    RestoreSessionResp = 200000,
    LoginResp = 200001,
    ChatResp = 200002,
    QueryInfoResp = 200003,
    GetInfoResp = 200004,
    LogoutResp = 200005,
    QueryUserStatusResp = 200007,
    QueryUserRegResp = 200008,
    InitLoginResp = 200009,
    ExQueryUserStatusResp = 200010,
    Service_Resp = 200011,
    Ex1QueryUserStatusResp = 200012,
    QueryPeerMsgMaxIdResp = 200013,
    QueryConvSummaryResp = 200014,
    UpdateSessionResp = 200015,
    NewMessageNotify = 300000,
    ReLoginNotify = 300001,
    ReConnectNotify = 300002
};

enum MSG_NOTIFY_TYPE
{
    SuccessFul = 0,
    ApplyJoinChatRoomResp = 102,
    QuitChatRoomResp = 103,
    NewmsgNotify = 1000,
    MemberJoinNotify = 1001,
    MemberQuitNotify = 1002,
    MemberGzipNotify = 1003
};

enum ERROR_TYPE
{
    ERR_CLIENT_VER_LOWER = 1000,
    ERR_REQUEST_PACKET_SEQ = 1001,
    ERR_LOGIN_FAILED = 1002,
    ERROR_INVALID_SENDER = 1003,
    ERROR_HIGHER_FREQUENCY = 1004,
    ERROR_UNKNOW_CHAR_TYPE = 1005,
    ERROR_DB_EXCEPTION = 1006,
    ERROR_SES_EXCEPTION = 1007,
    ERROR_PASSWD_INVALID = 1008,
    ERROR_DB_INNER = 2000,
    ERROR_SES_INNER = 2001,
    ERROR_NO_FOUND_USER = 3000,
    ERROR_GROUP_USER_KICKED = 4000,
    Socket_Error = 4294967040,
    Unable_To_Connect_Server = 4294967041
};

#define MOBILE_ANDROID	"android"
#define MOBILE_IOS		"ios"
#define MOBILE_PC		"pc"
