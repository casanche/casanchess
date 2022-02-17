#ifndef UTILS_H
#define UTILS_H

#include <chrono>
#include <random>

namespace Utils {

    //Pseudo-Random Number Generator
    class PRNG {
    public:
        PRNG(int seed = 0);
        uint32_t Random(int min, int max);
    private:
        std::random_device m_device;
        std::mt19937 m_mersenne;
    };

    //Pseudo-Random Number Generator for 64-bitwords
    class PRNG_64 {
    public:
        PRNG_64(int seed = 0);
        uint64_t Random();
    private:
        std::random_device m_device;
        std::mt19937_64 m_mersenne;
        std::uniform_int_distribution<uint64_t> m_distribution;
    };

    //General purpose clock
    //Returns elapsed time in milliseconds
    class Clock {
    public:
        void Start();
        int64_t Elapsed();
        int64_t ElapsedNanoseconds();
    private:
        std::chrono::system_clock::time_point m_start;
    };

} //namespace Utils

#endif //UTILS_H