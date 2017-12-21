#include "YY.hpp"
#include "utils/others.hpp"

bool platform_YY::is_room_valid()
{
    return str_is_num(m_roomid);
}

void platform_YY::start()
{
}
