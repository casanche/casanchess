#pragma once

#include <chrono>
#include <cstdint>
#include <limits>
#include <random>

namespace Utils {

    //Pseudo-Random Number Generator
    class PRNG {
    public:
        explicit PRNG(uint64_t seed = 0);
    
        uint32_t Random32(uint32_t min = 0, uint32_t max = std::numeric_limits<uint32_t>::max() );
        uint64_t Random64(uint64_t min = 0, uint64_t max = std::numeric_limits<uint64_t>::max() );
        double   RandomDouble(double min = 0.0, double max = 1.0);

    private:
        std::mt19937_64 m_mersenne;
    };

    //General purpose clock
    //Returns elapsed time in milliseconds
    class Clock {
    public:
        Clock() { Start(); }

        void Start();
        int64_t Elapsed() const;
        int64_t ElapsedNanoseconds() const;

        static auto Now() {
            return std::chrono::system_clock::now();
        }

    private:
        std::chrono::steady_clock::time_point m_start;
    };

} //namespace Utils
