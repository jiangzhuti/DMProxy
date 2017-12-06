#include "md5.hpp"
#include <openssl/md5.h>

const char HEX_NUMBERS[16] = {
  '0', '1', '2', '3',
  '4', '5', '6', '7',
  '8', '9', 'a', 'b',
  'c', 'd', 'e', 'f'
};

std::string md5_str(const std::string& text)
{
    uint8_t digest[16];
    MD5(reinterpret_cast<const unsigned char *>(text.data()), text.length(), digest);
    std::string output;
    output.reserve(16 << 1);
    for (size_t i = 0; i < 16; ++i) {
        int t = digest[i];
        int a = t / 16;
        int b = t % 16;
        output.append(1, HEX_NUMBERS[a]);
        output.append(1, HEX_NUMBERS[b]);
    }
    return output;
}
