#ifndef PLATFORM_YY_HPP
#define PLATFORM_YY_HPP

#include "platforms/platform_base.hpp"
#include "network/dmp_cs.hpp"

class platform_YY final : public platform_base
{
public:
    platform_YY(boost::asio::io_service& ios, std::string roomid) : platform_base(ios, "YY", roomid)
    {}
    void start();
    void close(){};
    bool is_room_valid();
};

#endif // PLATFORM_YY_HPP
