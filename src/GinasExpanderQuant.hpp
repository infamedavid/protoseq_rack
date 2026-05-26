#pragma once

#include <algorithm>
#include <cmath>

namespace protoseq {

inline int quantizeGinasExpanderSeedCv(float cv) {
    const float clamped = std::clamp(cv, 0.0f, 10.0f);
    return static_cast<int>(std::lround(clamped * 100.0f));
}

inline int quantizeGinasExpanderAlenCv(float cv) {
    const float clamped = std::clamp(cv, 0.0f, 10.0f);
    const float normalized = clamped / 10.0f;
    const float mapped = 2.0f + normalized * 14.0f;
    return std::clamp(static_cast<int>(std::lround(mapped)), 2, 16);
}

} // namespace protoseq
