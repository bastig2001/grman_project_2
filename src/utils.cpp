#include "utils.h"

#include <iostream>
#include <sstream>

using namespace std;

const char base64_chars[]{"ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/"};
// const char pad_char{'='};


stringstream encode_base64(istream& to_encode) {
    stringstream encoded{};
    unsigned int chars_val{};

    while (to_encode.rdbuf()->in_avail() >= 3) {
        chars_val = 0;

        for (short i{2}; i >= 0; i--) {
            char c{};
            to_encode >> c;
            chars_val += int(c) << (8 * i);
        }

        for (short i{3}; i >= 0; i--) {
            unsigned int c{chars_val >> (6 * i)};
            chars_val -= c << (6 * i);
            encoded << base64_chars[c];
        }
    }

    return encoded;
}

stringstream decode_base64(istream&) {
    return stringstream{};
}
