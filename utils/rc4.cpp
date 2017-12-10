#include "rc4.hpp"
#include <openssl/rc4.h>

void rc4_ptr(const uint8_t *data, uint32_t data_len, const uint8_t *key_data, uint32_t key_len, uint8_t *outbuf)
{
    RC4_KEY key;
    RC4_set_key(&key, key_len, key_data);
    RC4(&key, data_len, data, outbuf);
}

std::vector<uint8_t> rc4_vector(const uint8_t *data, uint32_t data_len, const uint8_t *key_data, uint32_t key_len)
{
    std::vector<uint8_t> output(data_len);
    rc4_ptr(data, data_len, key_data, key_len, output.data());
    return output;
}
std::string rc4_str(const uint8_t *data, uint32_t data_len, const uint8_t *key_data, uint32_t key_len)
{
    std::string output;
    output.resize(data_len);
    rc4_ptr(data, data_len, key_data, key_len, (uint8_t *)(output.data()));
    return output;
}
