#include <vector>
#include <boost/algorithm/string/split.hpp>
#include <boost/tokenizer.hpp>
#include <boost/utility/string_view.hpp>
#include <boost/utility.hpp>
#include "stt.hpp"
#include "douyu_config.hpp"
#include <iostream>

STT_t parse_stt(const char *msg)
{
    boost::string_view msg_view(msg);
    boost::char_separator<char> sep_array("/");
    boost::tokenizer<boost::char_separator<char>> msg_tok(msg_view, sep_array);

    STT_t stt;
    for (auto i : msg_tok) {
        auto pos = i.find_first_of("@=");
        if (pos == std::string::npos) {
            continue;
        }
        stt.insert(std::make_pair(std::string(i, 0, pos), std::string(i, pos + 2)));
    }
    return stt;
}
std::vector<uint8_t>* make_stt(const std::initializer_list<std::pair<std::string, std::string>>& il)
{
    std::string result;
    for (auto i : il) {
        result.append(i.first).append("@=").append(i.second).append("/");
    };
    auto packet = new std::vector<uint8_t>(4 + 4 + 4 + result.length() + 1);
    uint8_t *packet_buf = packet->data();
    *(uint32_t *)(packet_buf) = packet->size() - 4;
    packet_buf += 4;
    *(uint32_t *)(packet_buf) = packet->size() - 4;
    packet_buf += 4;
//    *(uint32_t *)(packet_buf) = MSG_TYPE::C_TO_S;
//    print_hex((const char *)packet->data(), packet->size());
    *(uint8_t *)(packet_buf) = 177;
    *(uint8_t *)(packet_buf + 1) = 2;
    *(uint8_t *)(packet_buf + 2) = 0;
    *(uint8_t *)(packet_buf + 3) = 0;
    packet_buf += 4;
    std::strcpy((char *)packet_buf, result.data());
    return packet;
}
