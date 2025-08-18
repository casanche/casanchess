#ifdef SPSA_TUNING

#include "Tuner.h"

#include <iostream>
#include <map>
#include <string>

namespace Tuner {

    #define TUNABLE_PARAM(type, name, value, ...) \
        type name = value;

    #include "TunerParams.tune"

    #undef TUNABLE_PARAM

    struct Tunable {
        std::string type;
        std::string name;
        int value;
        int min;
        int max;
        int step;
        double learningRate = 0.01;

        int Get() const { return value; }
        void Set(int newValue) { value = newValue; }
    };

    std::map<std::string, Tunable> tunableParams;

    void Initialize() {
        #define TUNABLE_PARAM(type, name, value, min, max, step) \
            tunableParams[#name] = { #type, #name, value, min, max, step };

        #include "TunerParams.tune"

        #undef TUNABLE_PARAM
    }

    void RegisterUciOptions() {
        for(const auto& [name, param] : tunableParams) {
            std::cout << "option name " << name << " type spin default " << param.value
                      << " min " << param.min << " max " << param.max << std::endl;
        }
    }

    void SetOption(const std::string& name, const std::string& value) {
        if(auto it = tunableParams.find(name); it != tunableParams.end()) {
            it->second.Set( std::stoi(value) );
        }
    }

    void PrintSPSAInputs() {
        for(const auto& [name, param] : tunableParams) {
            std::cout << name << ", " << param.type << ", " << param.value << ", "
                << param.min << ", " << param.max << ", "
                << param.step << ", " << param.learningRate << std::endl;
        }
    }

}

#endif // SPSA_TUNING
