#include "Constants.h"
#include "Debug.h"

#include <iostream>

void SearchDebug::Increment(const std::string& theVariable) {
    debugVariables[theVariable]++;
};

void SearchDebug::Print() {
    for(auto variable : debugVariables) {
        P("\t " << variable.first << " " << variable.second);
    }
}
