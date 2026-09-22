#include "EmbeddedNetwork.h"

#include <windows.h>

std::span<const std::byte> EmbeddedNetwork::Bytes() {
    const auto resource = FindResourceA(nullptr, "NNUE", RT_RCDATA);
    const auto handle = LoadResource(nullptr, resource);
    const auto* bytes = static_cast<const std::byte*>( LockResource(handle) );

    return {bytes, SizeofResource(nullptr, resource)};
}
