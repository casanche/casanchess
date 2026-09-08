#pragma once

#include <memory>
#include <string>

struct Engine;

class Interface {
public:
    Interface();
    ~Interface();

    void Print();
    void Start(std::string fenString = "");
private:
    void PrintWelcome();

    std::unique_ptr<Engine> m_engine;
};
