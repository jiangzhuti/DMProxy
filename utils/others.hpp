#ifndef OTHERS_HPP
#define OTHERS_HPP
#include <string>
#include <cstddef> // for size_t

inline bool str_is_num(std::string str)
{
    for (size_t i = 0; i < str.size(); i++) {
        int tmp = (int)str[i];
        if (tmp >= 48 && tmp <= 57)
            continue;
        else
            return false;
    }
    return true;
}

std::string random_string(const size_t length);

int random_number(const size_t length);

#endif // OTHERS_HPP
