#pragma once

#include <string>

// SPSA tuning can be activated during compilation: `cmake -DSPSA_TUNING=ON`
#ifndef SPSA_TUNING

namespace Tuner {
    // LMR Parameters
    constexpr int LMR_COMMON = 50;
    constexpr int LMR_ISPV = -100;
    constexpr int LMR_LOGTERM_DEPTH = 160;
    constexpr int LMR_LOGTERM_MOVENUMBER = 30;
    constexpr int LMR_LOGTERM_HISTORY_SCORE = -40;
    constexpr int LMR_BADCAPTURES = 20;
    constexpr int LMR_BADCAPTURE_TIER = -35;
    constexpr int LMR_KILLERS = -120;
    constexpr int LMR_KILLER_TIER = -50;

    // Empty functions for compatibility
    inline void RegisterTuningOptions() {}
    inline void SetTuningOption(const std::string&, const std::string&) {}
}

#else

// Global variables modified via UCI
namespace Tuner {
    extern int LMR_COMMON;
    extern int LMR_ISPV;
    extern int LMR_LOGTERM_DEPTH;
    extern int LMR_LOGTERM_MOVENUMBER;
    extern int LMR_LOGTERM_HISTORY_SCORE;
    extern int LMR_BADCAPTURES;
    extern int LMR_BADCAPTURE_TIER;
    extern int LMR_KILLERS;
    extern int LMR_KILLER_TIER;

    // Registrar opciones UCI para tuning
    void RegisterTuningOptions();
    
    // Procesar setoption para parámetros de tuning
    void SetTuningOption(const std::string& name, const std::string& value);
}

#endif
