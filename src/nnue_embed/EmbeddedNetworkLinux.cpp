#include "EmbeddedNetwork.h"

extern "C" const std::byte nnue_begin[];
extern "C" const std::byte nnue_end[];

std::span<const std::byte> EmbeddedNetwork::Bytes() {
    return {nnue_begin, nnue_end};
}
