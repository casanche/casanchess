#ifndef _TEST_COMMON
#define _TEST_COMMON

#include <iostream>
#include <sstream>
#include <string>

namespace Test {

    class CoutHelper {
    public:
        CoutHelper() {
            m_backup = std::cout.rdbuf();
        }
        void Mute() {
            std::cout.rdbuf(nullptr);
        }
        void Speak() {
            std::cout.rdbuf(m_backup);
        }
    private:
        std::streambuf* m_backup;
    };

    struct EPDPosition {
        std::string fen;
        std::string bestMove;
        std::string id;
    };

    EPDPosition ReadEPDLine(std::string line);

}


#endif //_TEST_COMMON
