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
    const float clamped = detail::clampFloat(cv, 0.0f, 20.0f);
    return detail::clampInt(static_cast<int>(std::lround(clamped * 100.0f)), 0, 2000);
}

inline int quantizeGinasExpanderAlenCv(float cv) {
    if (cv <= 0.0f) {
        return 2;
    }
    if (cv >= 20.0f) {
        return 128;
    }

    float mapped = 2.0f;
    if (cv <= 10.0f) {
        mapped = 2.0f + (cv / 10.0f) * 62.0f;
    } else {
        mapped = 64.0f + ((cv - 10.0f) / 10.0f) * 64.0f;
    }

    return detail::clampInt(static_cast<int>(std::lround(mapped)), 2, 128);
}

} // namespace protoseq
