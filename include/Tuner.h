#pragma once

#include <string>

// SPSA tuning can be activated during compilation: `cmake -DSPSA_TUNING=ON`
#ifdef SPSA_TUNING

// Tuning mode activated: variables modified via UCI
namespace Tuner {
    #define TUNABLE_PARAM(type, name, ...) \
        extern type name;

    #include "TunerParams.def"

    #undef TUNABLE_PARAM

    // API for tuning variables
    void Initialize();
    void RegisterUciOptions();
    void SetOption(const std::string&, const std::string&);
    void PrintSPSAInputs();
}

#else

namespace Tuner {

    #define TUNABLE_PARAM(type, name, value, ...) \
        constexpr type name = value;

    #include "TunerParams.def"

    #undef TUNABLE_PARAM

    // Empty functions for compatibility
    inline void Initialize() {}
    inline void RegisterUciOptions() {}
    inline void SetOption(const std::string&, const std::string&) {}
    inline void PrintSPSAInputs() {}
}

#endif // SPSA_TUNING
