#include "utils.h"

using namespace std;

const char base64_chars[]{"ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/"};
const char pad_char{'='};


string encode_base64(const string& to_encode) {
    string encoded{""};

    unsigned int cumulated{0};
    unsigned short bits_in_cumulated{0};

    for (unsigned char c : to_encode) {
        // cumulate chars to number
        cumulated <<= 8;
        cumulated += int(c);
        bits_in_cumulated += 8;

        while (bits_in_cumulated >= 6) {
            unsigned short shift(bits_in_cumulated - 6);

            // convert cumulated chars to base 64 chars
            unsigned int base64_val{cumulated >> shift};
            cumulated -= (base64_val << shift);
            bits_in_cumulated -= 6;
            encoded += base64_chars[base64_val];
        }
    }

    if (bits_in_cumulated > 0) {
        // encode remaining bits
        encoded += base64_chars[cumulated << (6 - bits_in_cumulated)];
    }

    if (encoded.length() % 4 != 0) {
        // fill encoded to multiple of 4
        encoded += string(4 - (encoded.length() % 4), pad_char);
    }

    return encoded;
}

string decode_base64(const string&) {
    return "";
}
