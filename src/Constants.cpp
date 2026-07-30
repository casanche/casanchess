#include "Constants.h"

#include <cmath>

namespace {
    std::array<u16, LOG_TABLE_SIZE> generate_log_table() {
        std::array<u16, LOG_TABLE_SIZE> table{};
        table[0] = 0;
        for(size_t i = 1; i < LOG_TABLE_SIZE; ++i) {
            double result = LOG_TABLE_SCALE * std::log(static_cast<double>( i ));
            table[i] = static_cast<u16>(result);
        }
        return table;
    }
}

const std::array<u16, LOG_TABLE_SIZE> LogTable = generate_log_table();
