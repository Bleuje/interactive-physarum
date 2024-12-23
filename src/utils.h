#pragma once

inline uint16_t floatAsUint16(float x)
{
    return uint16_t(std::round(std::max(std::min(x, 1.0f), 0.0f) * 65535.0f));
}