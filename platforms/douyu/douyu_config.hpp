#ifndef DOUYU_CONFIG_HPP
#define DOUYU_CONFIG_HPP
#include <cstdint>
enum
{
    HEADER_SIZE = 12
};
enum MSG_TYPE : uint16_t
{
    C_TO_S = 689,
    S_TO_C = 690
};
#endif // DOUYU_CONFIG_HPP
