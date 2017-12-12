#ifndef HUAJIAO_HPP
#define HUAJIAO_HPP

#include <string>
#include <utils/others.hpp>
#include "make_pack.hpp"
#include "parse_pack.hpp"
#include "platforms/platform_base.hpp"
#include "huajiao_config.hpp"
#include "utils/rw_lock.hpp"

class platform_huajiao final : public platform_base
{
public:
    platform_huajiao() : conn_info() {}
    platform_packet_t handle_binary_message(const void *data, size_t size);
    platform_packet_t handle_text_message(const std::string &msg);
    platform_packet_t handle_open(std::string roomid);
    bool is_roomid_valid(std::string roomid);
    std::string get_dm_url();
private:
    huajiao_conn_info_t conn_info;
    //正常并不会出现数据争用的情况
    //因此这个锁并没有用到
    rw_mutex_t conn_info_rw_mtx;
};

#endif
