#ifndef PLATFORM_BASE_HPP
#define PLATFORM_BASE_HPP

#include <utility>
#include <tuple>
#include <string>
#include <vector>
#include <cstdint>
#include <functional>
#include <memory>

enum class platform_packet_encap
{
    TEXT,
    BINARY,
    NONE
};
enum class platform_packet_direct
{
    SERVER,
    CLIENT,
    NONE
};

typedef std::tuple<
                   std::string, //text message
                   std::vector<uint8_t>, //binary message
                   platform_packet_encap, //type
                   platform_packet_direct //direct
                  > platform_packet_t;

class platform_base
{
    //注意!!
    //自己保证成员函数的线程安全性
public:
    virtual platform_packet_t handle_binary_message(const void *data, size_t size) = 0;
    virtual platform_packet_t handle_text_message(const std::string &msg) = 0;
    virtual platform_packet_t handle_open(std::string roomid) = 0;
    virtual bool is_roomid_valid(std::string roomid) = 0;
    virtual std::string get_dm_url() = 0;
    virtual ~platform_base();
protected:
    platform_base() {}

};

typedef std::shared_ptr<platform_base> platform_base_ptr_t;

typedef std::function<platform_base_ptr_t(void)> platform_creator_t;

#endif // PLATFORM_BASE_HPP
