#include "Utils.h"

namespace Utils {

    //PRNG
    PRNG::PRNG(uint64_t seed)
    {
        if(seed != 0) {
            m_mersenne.seed(seed);
        } else {
            std::random_device rd;
            uint64_t upper = static_cast<uint64_t>(rd());
            uint32_t lower = rd();

            uint64_t seedCombined = (upper << 32) | lower;
            m_mersenne.seed(seedCombined);
        }
    }

    uint32_t PRNG::Random32(uint32_t min, uint32_t max) {
        return std::uniform_int_distribution<uint32_t>(min, max)(m_mersenne);
    }

    uint64_t PRNG::Random64(uint64_t min, uint64_t max) {
        return std::uniform_int_distribution<uint64_t>(min, max)(m_mersenne);
    }

    double PRNG::RandomDouble(double min, double max) {
        return std::uniform_real_distribution<double>(min, max)(m_mersenne);
    }

    //Clock
    using SteadyClock = std::chrono::steady_clock;
    
    void Clock::Start() {
        m_start = SteadyClock::now();
    }
    int64_t Clock::Elapsed() const {
        return std::chrono::duration_cast<std::chrono::milliseconds>(SteadyClock::now() - m_start).count();
    }
    int64_t Clock::ElapsedNanoseconds() const {
        return std::chrono::duration_cast<std::chrono::nanoseconds>(SteadyClock::now() - m_start).count();
    }

} //namespace Utils
