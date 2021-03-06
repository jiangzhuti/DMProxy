#include "platforms.hpp"
#include <cstring>
#include "huajiao/huajiao.hpp"
#include "douyu/douyu.hpp"
#include "bilibili/bilibili.hpp"
#include "YY/YY.hpp"

static std::map<std::string, platform_creator_t> platform_creator_table;

void platforms_init()
{
    platform_creator_table["huajiao"] = [](boost::asio::io_service& ios, std::string roomid) -> platform_base_ptr_t
                                        {
                                            return std::dynamic_pointer_cast<platform_base>(std::make_shared<platform_huajiao>(ios, roomid));
                                        };
    platform_creator_table["douyu"] = [](boost::asio::io_service& ios, std::string roomid) -> platform_base_ptr_t
                                        {
                                            return std::dynamic_pointer_cast<platform_base>(std::make_shared<platform_douyu>(ios, roomid));
                                        };
    platform_creator_table["bilibili"] = [](boost::asio::io_service& ios, std::string roomid) -> platform_base_ptr_t
                                        {
                                            return std::dynamic_pointer_cast<platform_base>(std::make_shared<platform_bilibili>(ios, roomid));
                                        };
    platform_creator_table["YY"] = [](boost::asio::io_service& ios, std::string roomid) -> platform_base_ptr_t
                                        {
                                            return std::dynamic_pointer_cast<platform_base>(std::make_shared<platform_YY>(ios, roomid));
                                        };
}

platform_base_ptr_t platform_get_instance(std::string tag, boost::asio::io_service &ios, std::string roomid)
{
    if (platform_creator_table.count(tag) == 0) {
        return nullptr;
    } else {
        return platform_creator_table[tag](ios, roomid);
    }
}
