#include "NNUE.h"

#include <cstdint>
#include <iostream>
#include <filesystem>
#include <fstream>
#include <string>

float GetNumber(std::ifstream& ifile) {
    std::string line;
    std::getline(ifile, line, '\n');
    float decimal = std::stof(line);
    return decimal;
}

void Convert(std::string ifilename, std::string ofilename) {
    //Read model parameters from plain .txt
    std::ifstream ifile;
    ifile.open(ifilename);

    if(!ifile.is_open())
        return;

    std::cout << "Converting network: " << ifilename << std::endl;

    NetworkStorage* nnue_storage = new NetworkStorage;

    //L1
    for(uint col = 0; col < ARCH[L1][COL]; col++) {
        for(uint row = 0; row < ARCH[L1][ROW]; row++) {
            float decimal = GetNumber(ifile);
            int converted_value = decimal * CONVERSION_FACTOR;
            assert(std::abs(converted_value) < __INT16_MAX__);
            if(std::abs(converted_value) >= __INT16_MAX__)
                std::cout << "Watch out! Int16 overflow (" << decimal << ")" << std::endl;
            nnue_storage->w1[row * ARCH[L1][COL] + col] = (int16_t)(converted_value);
        }
    }
    for(uint col = 0; col < ARCH[L1][COL]; col++) {
        float decimal = GetNumber(ifile);
        nnue_storage->b1[col] = decimal;
    }

    //L2
    for(uint col = 0; col < ARCH[L2][COL]; col++) {
        for(uint row = 0; row < ARCH[L2][ROW]; row++) {
            float decimal = GetNumber(ifile);
            //nnue_storage->w0[row * ARCH[L2][COL] + col] = decimal;
            nnue_storage->w2[col * ARCH[L2][ROW] + row] = decimal; //transposition
        }
    }
    for(uint col = 0; col < ARCH[L2][COL]; col++) {
        float decimal = GetNumber(ifile);
        nnue_storage->b2[col] = decimal;
    }

    //L3
    for(uint col = 0; col < ARCH[L3][COL]; col++) {
        for(uint row = 0; row < ARCH[L3][ROW]; row++) {
            float decimal = GetNumber(ifile);
            // nnue_storage->w0[row * ARCH[L3][COL] + col] = decimal;
            nnue_storage->w3[col * ARCH[L3][ROW] + row] = decimal; //transposition
        }
    }
    for(uint col = 0; col < ARCH[L3][COL]; col++) {
        float decimal = GetNumber(ifile);
        nnue_storage->b3[col] = decimal;
    }

    //L4
    for(uint col = 0; col < ARCH[L4][COL]; col++) {
        for(uint row = 0; row < ARCH[L4][ROW]; row++) {
            float decimal = GetNumber(ifile);
            nnue_storage->w4[row * ARCH[L4][COL] + col] = decimal;
        }
    }
    for(uint col = 0; col < ARCH[L4][COL]; col++) {
        float decimal = GetNumber(ifile);
        nnue_storage->b4[col] = decimal;
    }

    ifile.close();

    //Write binary file
    std::ofstream ofile;
    ofile.open(ofilename, std::ofstream::binary);

    if(!ofile.is_open())
        return;

    std::cout << "Writting binary to: " << ofilename << std::endl;

    ofile.write((char*)nnue_storage, sizeof(NetworkStorage));
    ofile.close();
}

int main(int argc, char** argv) {
    std::filesystem::path filepath = argv[1];
    std::string inputFile = filepath.string();
    std::string outputFile = filepath.replace_extension(".nn").string();
    Convert(inputFile, outputFile);
}
