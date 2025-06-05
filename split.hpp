#ifndef SPLIT_HPP
#define SPLIT_HPP

#include <sstream>
#include <string>
#include <vector>

inline std::vector<std::string> split(const std::string &s, char delimiter) {
    std::vector<std::string> tokens;
    std::string token;
    std::istringstream stream(s);
    while (std::getline(stream, token, delimiter)) {
        if (!token.empty()) {
            tokens.push_back(token);
        }
    }
    return tokens;
}

#endif // SPLIT_HPP
