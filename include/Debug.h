#ifndef DEBUG_H
#define DEBUG_H

#include <map>
#include <string>

const bool DEBUG_PRINT_STATISTICS = true;

// Macros
#ifdef DEBUG
    #define D(x) x
#else
    #define D(x) ((void)0)
#endif

// Object to collect and print search statistics
class SearchDebug {
public:
    void Increment(std::string theVariable) { debugVariables[theVariable]++; };
    void Print();
private:
    void Transform();
    
    std::map<std::string, int> debugVariables;
};

#endif //DEBUG_H
