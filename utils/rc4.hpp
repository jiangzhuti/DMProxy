#ifndef RC4_HPP
#define RC4_HPP

#include <vector>
#include <string>
#include <cstdint>

std::vector<uint8_t> rc4_vector(const uint8_t *data, uint32_t data_len, const uint8_t *key_data, uint32_t key_len);
void rc4_ptr(const uint8_t *data, uint32_t data_len, const uint8_t *key_data, uint32_t key_len, uint8_t *outbuf);
std::string rc4_str(const uint8_t *data, uint32_t data_len, const uint8_t *key_data, uint32_t key_len);//maybe unsafe

#endif // RC4_HPP
