#include "platforms.hpp"
#include <cstring>
#include "huajiao/huajiao.hpp"

struct str_case_less
{
    bool operator() (const std::string &lhs, const std::string &rhs) const
    {
        return strcasecmp(lhs.data(), rhs.data()) < 0;
    }
};

static std::map<std::string, platform_creator_t, str_case_less> platform_creator_table;

void platforms_init()
{
    platform_creator_table["huajiao"] = []() -> platform_base_ptr_t
                                        {
                                            return std::static_pointer_cast<platform_base>(std::make_shared<platform_huajiao>());
                                        };
}

platform_base_ptr_t platform_get_instance(std::string tag)
{
    if (platform_creator_table.count(tag) == 0) {
        return nullptr;
    } else {
        return platform_creator_table[tag]();
    }
}
