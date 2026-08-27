#include "NNUE_Architecture.h"

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
                  << " with factor " << factor << " --> Clamping to limits!" << std::endl;
        integer = std::clamp(integer, min, max);
    }
    return static_cast<T>(integer);
}

void Convert(std::string ifilename, std::string ofilename) {
    std::ifstream ifile;
    ifile.open(ifilename);

    if(!ifile.is_open()) return;

    std::cout << "Converting V1.3 network: " << ifilename << std::endl;

    auto nnue_storage = std::make_unique<Network>();

    // L1
    for(uint col = 0; col < ARCH[L1][COL]; col++) {
        for(uint row = 0; row < ARCH[L1][ROW]; row++) {
            nnue_storage->w1[row * ARCH[L1][COL] + col] = Quantize<i16>(GetNumber(ifile), NNUEConstants::QUANT_FACTOR_L1);
        }
    }
    for(uint row = 0; row < ARCH[L1][ROW]; row++) {
        nnue_storage->linearW[row] = Quantize<i16>(GetNumber(ifile), NNUEConstants::QUANT_FACTOR_L1);
    }
    for(uint col = 0; col < ARCH[L1][COL]; col++) {
        nnue_storage->b1[col] = Quantize<i16>(GetNumber(ifile), NNUEConstants::QUANT_FACTOR_L1);
    }
    GetNumber(ifile); // Linear bias cancels in the us-them subtraction

    // L2
    for(uint col = 0; col < ARCH[L2][COL]; col++) {
        for(uint row = 0; row < ARCH[L2][ROW]; row++) {
            nnue_storage->w2[col * ARCH[L2][ROW] + row] = Quantize<i16>(GetNumber(ifile), NNUEConstants::QUANT_FACTOR_W);
        }
    }
    for(uint col = 0; col < ARCH[L2][COL]; col++) {
        nnue_storage->b2[col] = Quantize<i32>(GetNumber(ifile), NNUEConstants::QUANT_FACTOR_B);
    }

    // L3
    for(uint col = 0; col < ARCH[L3][COL]; col++) {
        for(uint row = 0; row < ARCH[L3][ROW]; row++) {
            nnue_storage->w3[col * ARCH[L3][ROW] + row] = Quantize<i16>(GetNumber(ifile), NNUEConstants::QUANT_FACTOR_W);
        }
    }
    for(uint col = 0; col < ARCH[L3][COL]; col++) {
        nnue_storage->b3[col] = Quantize<i32>(GetNumber(ifile), NNUEConstants::QUANT_FACTOR_B);
    }

    // Drawishness residual head from x_clamp256
    for(uint row = 0; row < ARCH[L2][ROW]; row++) {
        nnue_storage->drawW[row] = Quantize<i16>(GetNumber(ifile), NNUEConstants::QUANT_FACTOR_W);
    }
    nnue_storage->drawB = Quantize<i32>(GetNumber(ifile), NNUEConstants::QUANT_FACTOR_B);

    ifile.close();

    // Write binary file
    std::ofstream ofile;
    ofile.open(ofilename, std::ios::binary);
    if(!ofile.is_open()) {
        std::cerr << "ERROR: Could not open output file for writing: " << ofilename << std::endl;
        return;
    }

    std::cout << "Writing V1.3 binary to: " << ofilename << std::endl;
    ofile.write((char*)nnue_storage.get(), sizeof(Network));
    ofile.close();
}
