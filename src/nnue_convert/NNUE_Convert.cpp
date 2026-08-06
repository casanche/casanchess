#include "NNUE.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <iostream>
#include <filesystem>
#include <fstream>
#include <string>

float GetNumber(std::ifstream& ifile) {
    std::string line;
    std::getline(ifile, line, '\n');
    return std::stof(line);
}

template <typename T>
T Quantize(float decimal, float factor) {
    i64 integer = std::llround(decimal * factor);

    i64 min = std::numeric_limits<T>::min();
    i64 max = std::numeric_limits<T>::max();

    if(integer < min || integer > max) {
        std::cerr << "Warning: Quantization overflow for value " << decimal
                  << " with factor " << factor
                  << " --> Clamping to limits!"
                  << std::endl;
        integer = std::clamp(integer, min, max);
    }

    return static_cast<T>(integer);
}

void Convert(std::string ifilename, std::string ofilename) {
    //Read model parameters from plain .txt
    std::ifstream ifile;
    ifile.open(ifilename);

    if(!ifile.is_open())
        return;

    std::cout << "Converting network: " << ifilename << std::endl;

    auto nnue_storage = std::make_unique<Network>();

    //L1
    for(uint col = 0; col < ARCH[L1][COL]; col++) {
        for(uint row = 0; row < ARCH[L1][ROW]; row++) {
            float decimal = GetNumber(ifile);
            i16 quantized = Quantize<i16>(decimal, NNUEConstants::QUANT_FACTOR_L1);
            nnue_storage->w1[row * ARCH[L1][COL] + col] = quantized;
        }
    }
    for(uint col = 0; col < ARCH[L1][COL]; col++) {
        float decimal = GetNumber(ifile);
        i16 quantized = Quantize<i16>(decimal, NNUEConstants::QUANT_FACTOR_L1);
        nnue_storage->b1[col] = quantized;
    }

    //L2
    for(uint col = 0; col < ARCH[L2][COL]; col++) {
        for(uint row = 0; row < ARCH[L2][ROW]; row++) {
            float decimal = GetNumber(ifile);
            i16 quantized = Quantize<i16>(decimal, NNUEConstants::QUANT_FACTOR_W);
            nnue_storage->w2[col * ARCH[L2][ROW] + row] = quantized; //transposition
        }
    }
    for(uint col = 0; col < ARCH[L2][COL]; col++) {
        float decimal = GetNumber(ifile);
        i32 quantized = Quantize<i32>(decimal, NNUEConstants::QUANT_FACTOR_B);
        nnue_storage->b2[col] = quantized;
    }

    //L3
    for(uint col = 0; col < ARCH[L3][COL]; col++) {
        for(uint row = 0; row < ARCH[L3][ROW]; row++) {
            float decimal = GetNumber(ifile);
            i16 quantized = Quantize<i16>(decimal, NNUEConstants::QUANT_FACTOR_W);
            nnue_storage->w3[col * ARCH[L3][ROW] + row] = quantized; //transposition
        }
    }
    for(uint col = 0; col < ARCH[L3][COL]; col++) {
        float decimal = GetNumber(ifile);
        i32 quantized = Quantize<i32>(decimal, NNUEConstants::QUANT_FACTOR_B);
        nnue_storage->b3[col] = quantized;
    }

    //L4
    for(uint col = 0; col < ARCH[L4][COL]; col++) {
        for(uint row = 0; row < ARCH[L4][ROW]; row++) {
            float decimal = GetNumber(ifile);
            i16 quantized = Quantize<i16>(decimal, NNUEConstants::QUANT_FACTOR_W);
            nnue_storage->w4[row * ARCH[L4][COL] + col] = quantized;
        }
    }
    for(uint col = 0; col < ARCH[L4][COL]; col++) {
        float decimal = GetNumber(ifile);
        i32 quantized = Quantize<i32>(decimal, NNUEConstants::QUANT_FACTOR_B);
        nnue_storage->b4[col] = quantized;
    }

    ifile.close();

    //Write binary file
    std::ofstream ofile;
    ofile.open(ofilename, std::ios::binary);

    if(!ofile.is_open()) {
        std::cerr << "ERROR: Could not open output file for writing: " << ofilename << std::endl;
        return;
    }

    std::cout << "Writing binary to: " << ofilename << std::endl;

    ofile.write((char*)nnue_storage.get(), sizeof(Network));
    ofile.close();
}

int main([[maybe_unused]] int argc, char** argv) {
    std::filesystem::path filepath = argv[1];
    std::string inputFile = filepath.string();
    std::string outputFile = filepath.replace_extension(".nn").string();
    Convert(inputFile, outputFile);
}
