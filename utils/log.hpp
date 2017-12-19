#ifndef LOG_HPP
#define LOG_HPP
#include <iostream>

#define PRINT_ERROR(ec) \
    std::cerr << __func__ << ":" << __LINE__ << " error occured because: " << ec.message() << std::endl;
#endif // LOG_HPP
