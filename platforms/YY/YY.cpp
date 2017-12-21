#include "YY.hpp"
#include "utils/others.hpp"
#include "json11/json11.hpp"
#include <ctime>

bool platform_YY::is_room_valid()
{
    return str_is_num(m_roomid);
}

void platform_YY::start()
{
    std::error_code ec;
    auto conn = client.get_connection(m_ws_url, ec);
    if (CLIENT_REPORT_WHEN_ERROR(ec)) {
        return;
    }
    conn->set_open_handler(std::bind(&platform_YY::on_client_open,
                                     std::dynamic_pointer_cast<platform_YY>(shared_from_this()),
                                     std::placeholders::_1));
    conn->set_fail_handler(std::bind(&platform_YY::on_client_fail,
                                     std::dynamic_pointer_cast<platform_YY>(shared_from_this()),
                                     std::placeholders::_1));
    conn->set_message_handler(std::bind(&platform_YY::on_client_message,
                                        std::dynamic_pointer_cast<platform_YY>(shared_from_this()),
                                        std::placeholders::_1,
                                        std::placeholders::_2));
    conn->set_close_handler(std::bind(&platform_YY::on_client_close,
                                      std::dynamic_pointer_cast<platform_YY>(shared_from_this()),
                                      std::placeholders::_1));
    m_hdl = client.connect(conn);
    m_hb_info["uri"] = "6";
    m_hb_info["svc_link"] = "66b2c67d-f7ab-4355-a6f0-b52ffccc857e";
}

void platform_YY::close()
{
    std::error_code ec;
    m_hb_timer->cancel();
    client.close(m_hdl, websocketpp::close::status::normal, "close", ec);
    CLIENT_REPORT_WHEN_ERROR(ec);
}

void platform_YY::on_client_open(connection_hdl hdl)
{
    (void) hdl;
    auto x = client.get_con_from_hdl(hdl);
}

void platform_YY::on_client_fail(connection_hdl hdl)
{
    auto ec = client.get_con_from_hdl(hdl)->get_ec();
    CLIENT_REPORT_WHEN_ERROR(ec);
}


void platform_YY::on_client_message(connection_hdl hdl, message_ptr msg)
{
    std::error_code ec;
    if (msg->get_opcode() != opcode::TEXT) {
        ec = std::make_error_code(std::errc::bad_message);
        CLIENT_CLOSE_AND_REPORT_WHEN_ERROR_WS(ec, hdl);
    }
    std::string payload = msg->get_payload();
    std::string err;
    auto msg_json = json11::Json::parse(payload, err);
    if (!err.empty()) {
        ec = std::make_error_code(std::errc::bad_message);
        CLIENT_CLOSE_AND_REPORT_WHEN_ERROR_WS(ec, hdl);
    }
    std::string resp = msg_json["response"].string_value();
    if (resp == "login") {
        auto packet = new_login_packet();
        client.send(hdl, packet, opcode::TEXT, ec);
        CLIENT_CLOSE_AND_REPORT_WHEN_ERROR_WS(ec, hdl);
        m_hb_timer = client.set_timer(45000, std::bind(&platform_YY::handle_heartbeat,
                                                       std::dynamic_pointer_cast<platform_YY>(shared_from_this()),
                                                       std::placeholders::_1));
    } else if (resp == "init") {
        auto packet = new_init_packet();
        client.send(hdl, packet, opcode::TEXT, ec);
        if (CLIENT_CLOSE_AND_REPORT_WHEN_ERROR_WS(ec, hdl)) {
            m_hb_timer->cancel();
        }
    } else if (resp == "join") {
        //nothing to do
    } else if (__builtin_expect(resp == "chat", true)) {
        std::string uid = msg_json["uid"].string_value(); //uid or yyid???
        std::string nickname = msg_json["nick"].string_value();
        std::string chat_msg = msg_json["chat_msg"].string_value();
        publish_json(uid, nickname, chat_msg);
    } else {
        //nothing to do
    }
}

void platform_YY::on_client_close(connection_hdl hdl)
{
    (void)hdl;
    publish("YY danmu connection closed!");
    m_hb_timer->cancel();
    m_close_callback(std::string().append(m_platform_name).append("_").append(m_roomid));
}

void platform_YY::handle_heartbeat(std::error_code ec)
{
    if (CLIENT_CLOSE_AND_REPORT_WHEN_ERROR_WS(ec, m_hdl)) {
        return;
    }
    m_hb_info["ts"] = std::to_string(time(NULL));
    std::string hb = json11::Json(m_hb_info).dump();
    client.send(m_hdl, hb, opcode::TEXT, ec);
    if (CLIENT_CLOSE_AND_REPORT_WHEN_ERROR_WS(ec, m_hdl)) {
        return;
    }
    m_hb_timer = client.set_timer(45000, std::bind(&platform_YY::handle_heartbeat,
                                                   std::dynamic_pointer_cast<platform_YY>(shared_from_this()),
                                                   std::placeholders::_1));
}


std::string platform_YY::new_login_packet()
{
    m_login_info.clear();
    m_login_info["uri"] = "0";
    m_login_info["type"] = "0";
    m_login_info["svc_link"] = "66b2c67d-f7ab-4355-a6f0-b52ffccc857e";
    return json11::Json(m_login_info).dump();
}

std::string platform_YY::new_init_packet()
{
    m_init_info.clear();
    m_init_info["uri"] = "1";
    m_init_info["top_sid"] = m_roomid;
    m_init_info["sub_sid"] = m_roomid;
    m_init_info["svc_link"] = "66b2c67d-f7ab-4355-a6f0-b52ffccc857e";
    return json11::Json(m_init_info).dump();
}
