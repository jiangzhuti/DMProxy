#ifndef GZIP_HPP
#define GZIP_HPP

#include <vector>
#include <cstddef> //for size_t

bool ungzip(std::vector<unsigned char>& in, size_t count_in, std::vector<unsigned char>& out, size_t& count_out);
bool ungzip(std::vector<unsigned char>& in, std::vector<unsigned char>& out);

#endif // GZIP_HPP
