#ifndef DEBUG_H
#define DEBUG_H

#include <map>
#include <string>

const bool DEBUG_PRINT_STATISTICS = true;

const bool DEBUG_SEARCH_TREE = false;

// Macros
#ifdef DEBUG
    #define D(x) x
#else
    #define D(x) ((void)0)
#endif

// Object to collect and print search statistics
class SearchDebug {
public:
    void Clear() { debugVariables.clear(); };
    void Increment(const std::string& theVariable);
    void Print();
private:
    std::map<std::string, int> debugVariables;
};

#endif //DEBUG_H
