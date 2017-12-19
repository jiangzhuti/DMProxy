#ifndef STT_H
#define STT_H

#include <string>
#include <map>
#include <vector>
#include <initializer_list>

typedef std::map<std::string, std::string> STT_t;
STT_t parse_stt(const char *msg);
std::vector<uint8_t> *make_stt(const std::initializer_list<std::pair<std::string, std::string>>& il);


#endif
