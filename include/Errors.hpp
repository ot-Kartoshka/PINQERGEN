#pragma once

#include <string_view>


enum class Error {
    ParseError,
    UnknownGenerator,
    InvalidLength,
    InvalidAlpha,
    InvalidSegments,
    FileReadError,
    FileWriteError,
    EmptySequence,
    NotEnoughData
};

std::string_view error_to_string(Error err);
