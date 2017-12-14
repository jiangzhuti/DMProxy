#ifndef PLATFORMS_HPP
#define PLATFORMS_HPP

#include <string>
#include <map>

#include "platform_base.hpp"

platform_base_ptr_t platform_get_instance(std::string tag, boost::asio::io_service* ios_ptr);
void platforms_init();

#endif // PLATFORMS_HPP
