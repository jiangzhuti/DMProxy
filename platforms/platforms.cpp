#include "platforms.hpp"
#include <cstring>
#include "huajiao/huajiao.hpp"

static std::map<std::string, platform_creator_t> platform_creator_table;

void platforms_init()
{
    platform_creator_table["huajiao"] = [](boost::asio::io_service* ios_ptr) -> platform_base_ptr_t
                                        {
                                            return std::static_pointer_cast<platform_base>(std::make_shared<platform_huajiao>(ios_ptr));
                                        };
}

platform_base_ptr_t platform_get_instance(std::string tag, boost::asio::io_service *ios_ptr)
{
    if (platform_creator_table.count(tag) == 0) {
        return nullptr;
    } else {
        return platform_creator_table[tag](ios_ptr);
    }
}
