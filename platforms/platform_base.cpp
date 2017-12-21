#include "platform_base.hpp"
#include <json11/json11.hpp>
#include "utils/base64.hpp"

platform_base::~platform_base()
{}

void platform_base::add_listener(connection_hdl conn_hdl)
{
    wlock_t wlock(m_lmtx);
    m_listeners.insert(conn_hdl);
}

void platform_base::erase_listener(connection_hdl conn_hdl)
{
    wlock_t wlock(m_lmtx);
    m_listeners.erase(conn_hdl);
}

bool platform_base::have_listener(connection_hdl conn_hdl)
{
    rlock_t rlock(m_lmtx);
    return m_listeners.count(conn_hdl) == 1;
}

size_t platform_base::listeners_count()
{
    rlock_t rlock(m_lmtx);
    return m_listeners.size();
}

void platform_base::set_close_callback(close_callback_t cb)
{
    m_close_callback = cb;
}

void platform_base::publish(std::string dm_msg)
{
    error_code ec;
    rlock_t rlock(m_lmtx);
    for (auto i : m_listeners) {
        server.send(i, dm_msg, opcode::TEXT, ec);
        if (SERVER_CLOSE_AND_REPORT_WHEN_ERROR(ec, i)) {
            std::cerr << "\n" << dm_msg << std::endl;
        }
    }
}

void platform_base::publish_json(std::string uid, std::string nickname, std::string text)
{
    //use base64 to avoid 'payload contained invalid data'
    auto msg_json = json11::Json({
                                     {"uid", uid},
                                     {"nickname", base64_encode((unsigned char *)nickname.data(), nickname.length())},
                                     {"text", base64_encode((unsigned char *)text.data(), text.length())}
                                 });
    publish(msg_json.dump());
}
