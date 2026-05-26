#pragma once

#include <cmath>

namespace protoseq {
namespace detail {

inline float clampFloat(float value, float low, float high) {
    return value < low ? low : (value > high ? high : value);
}

inline int clampInt(int value, int low, int high) {
    return value < low ? low : (value > high ? high : value);
}

} // namespace detail

inline int quantizeGinasExpanderSeedCv(float cv) {
    const float clamped = detail::clampFloat(cv, 0.0f, 10.0f);
    return static_cast<int>(std::lround(clamped * 100.0f));
}

inline int quantizeGinasExpanderAlenCv(float cv) {
    const float clamped = detail::clampFloat(cv, 0.0f, 10.0f);
    const float normalized = clamped / 10.0f;
    const float mapped = 2.0f + normalized * 14.0f;
    return detail::clampInt(static_cast<int>(std::lround(mapped)), 2, 16);
}

} // namespace protoseq
