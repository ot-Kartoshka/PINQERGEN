#pragma once

#include "Generators.hpp"


std::unique_ptr<ByteGenerator> MakeBlumMicali(uint64_t seed, bool byteMode);
std::unique_ptr<ByteGenerator> MakeBbs(uint64_t seed, bool byteMode);
