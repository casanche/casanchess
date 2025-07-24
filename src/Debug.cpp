#include "Constants.h"
#include "Debug.h"

#include <iostream>

void SearchDebug::Transform() {
    std::string colorGreen = "\033[1;92m";
    std::string colorBlack = "\033[0m";
    // debugVariables[colorGreen+"NegaMax Calls to Evaluation (permil)"+colorBlack] = CastInt(1000 * (double)debugVariables["NegaMax Calls to Evaluation"] / debugVariables["NegaMax Hits"]);
    // debugVariables[colorGreen+"NegaMax Cutoffs (permil)"+colorBlack] = CastInt(1000 * (double)debugVariables["NegaMax Cutoffs (score >= beta)"] / debugVariables["NegaMax Hits"]);
    // debugVariables[colorGreen+"NegaMax GenerateMoves (permil)"+colorBlack] = CastInt(1000 * (double)debugVariables["NegaMax GenerateMoves Hits"] / debugVariables["NegaMax Hits"]);
    // debugVariables[colorGreen+"TT Hits (in NegaMax) (permil)"+colorBlack] = CastInt(1000 * (double)debugVariables["TT Hits (in NegaMax)"] / debugVariables["NegaMax Hits"]);
    // debugVariables[colorGreen+"TT Hits (in Quiescence) (permil)"+colorBlack] = CastInt(1000 * (double)debugVariables["TT Hits (in Quiescence)"] / debugVariables["Quiescence Hits"]);
}

void SearchDebug::Print() {
    Transform();
    for(auto variable : debugVariables) {
        P("\t " << variable.first << " " << variable.second);
    }
}
