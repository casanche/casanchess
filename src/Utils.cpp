#include "Utils.h"

namespace Utils {

    //PRNG
    PRNG::PRNG(int seed) :
        m_mersenne(seed ? seed : m_device()) {}

    uint32_t PRNG::Random(int min, int max) {
        return std::uniform_int_distribution<uint32_t>(min, max)(m_mersenne);
    }

    //PRNG_64
    PRNG_64::PRNG_64(int seed) :
        m_mersenne(seed ? seed : m_device()) {}

    uint64_t PRNG_64::Random() {
        return m_distribution(m_mersenne);
    }

    //Clock
    typedef std::chrono::high_resolution_clock HighResClock;
    
    void Clock::Start() {
        m_start = HighResClock::now();
    }
    int64_t Clock::Elapsed() {
        return std::chrono::duration_cast<std::chrono::milliseconds>(HighResClock::now() - m_start).count();
    }

} //namespace Utils
