#include "Tuner.h"

#ifdef SPSA_TUNING

#include <iostream>
#include <string>

namespace Tuner {
    // Variables globales para tuning SPSA - valores iniciales por defecto
    int LMR_COMMON = 50;
    int LMR_ISPV = -100;
    int LMR_LOGTERM_DEPTH = 160;
    int LMR_LOGTERM_MOVENUMBER = 30;
    int LMR_LOGTERM_HISTORY_SCORE = -40;
    int LMR_BADCAPTURES = 20;
    int LMR_BADCAPTURE_TIER = -35;
    int LMR_KILLERS = -120;
    int LMR_KILLER_TIER = -50;

    void RegisterTuningOptions() {
        // Registrar opciones UCI para tuning SPSA
        // Formato: option name <name> type spin default <value> min <min> max <max>
        
        std::cout << "option name LMR_COMMON type spin default 50 min -50 max 200" << std::endl;
        std::cout << "option name LMR_ISPV type spin default -100 min -200 max 0" << std::endl;
        std::cout << "option name LMR_LOGTERM_DEPTH type spin default 160 min 0 max 300" << std::endl;
        std::cout << "option name LMR_LOGTERM_MOVENUMBER type spin default 30 min 0 max 100" << std::endl;
        std::cout << "option name LMR_LOGTERM_HISTORY_SCORE type spin default -40 min -150 max 0" << std::endl;
        std::cout << "option name LMR_BADCAPTURES type spin default 20 min -50 max 150" << std::endl;
        std::cout << "option name LMR_BADCAPTURE_TIER type spin default -35 min -60 max -5" << std::endl;
        std::cout << "option name LMR_KILLERS type spin default -120 min -300 max 0" << std::endl;
        std::cout << "option name LMR_KILLER_TIER type spin default -50 min -100 max 0" << std::endl;
    }

    void SetTuningOption(const std::string& name, const std::string& value) {
        int val = std::stoi(value);
        
        if (name == "LMR_COMMON") {
            LMR_COMMON = val;
        } else if (name == "LMR_ISPV") {
            LMR_ISPV = val;
        } else if (name == "LMR_LOGTERM_DEPTH") {
            LMR_LOGTERM_DEPTH = val;
        } else if (name == "LMR_LOGTERM_MOVENUMBER") {
            LMR_LOGTERM_MOVENUMBER = val;
        } else if (name == "LMR_LOGTERM_HISTORY_SCORE") {
            LMR_LOGTERM_HISTORY_SCORE = val;
        } else if (name == "LMR_BADCAPTURES") {
            LMR_BADCAPTURES = val;
        } else if (name == "LMR_BADCAPTURE_TIER") {
            LMR_BADCAPTURE_TIER = val;
        } else if (name == "LMR_KILLERS") {
            LMR_KILLERS = val;
        } else if (name == "LMR_KILLER_TIER") {
            LMR_KILLER_TIER = val;
        }
    }
}

#endif
