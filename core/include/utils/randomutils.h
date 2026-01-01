#pragma once

#include <cstdint>
#include <random>

namespace SpaceRogueLite::Utils {

inline std::mt19937& getRandomGenerator() {
    static std::random_device dev;
    static std::mt19937 rng(dev());
    return rng;
}

inline void setRandomGeneratorSeed(uint32_t seed) { getRandomGenerator() = std::mt19937(seed); }

inline uint32_t randomRange(uint32_t lower, uint32_t upper) {
    std::uniform_int_distribution<std::mt19937::result_type> dist(lower, upper);

    return dist(getRandomGenerator());
}

inline double randomRangeDouble(double lower, double upper) {
    std::uniform_real_distribution<> dist(lower, upper);

    return dist(getRandomGenerator());
}

inline uint32_t randomDN(uint32_t n) {
    std::uniform_int_distribution<std::mt19937::result_type> distn(1, n);

    return distn(getRandomGenerator());
}

inline uint32_t randomD6(void) { return randomDN(6); }

template <typename T>
inline T randomChoice(const std::vector<T>& vec) {
    return vec[randomRange(0, vec.size() - 1)];
}

template <typename T>
inline T randomChoice(const std::vector<T>& vec, const std::vector<int>& weights) {
    game_assert(vec.size() == weights.size());

    static std::default_random_engine generator;
    std::discrete_distribution<int> dist(weights.begin(), weights.end());

    return vec[dist(generator)];
}

}  // namespace SpaceRogueLite::Utils
