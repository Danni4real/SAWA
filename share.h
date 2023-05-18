#ifndef SHARE_H
#define SHARE_H

#include <ctime>
#include <string>
#include <chrono>

#define Debug

typedef unsigned int uint;

inline std::string time_now()
{
    std::time_t now = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
    return std::string(std::ctime(&now), 24);
}

#endif
