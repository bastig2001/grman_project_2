#include "utils.h"

#include <algorithm>
#include <iterator>

using namespace std;

const char base64_chars[]{"ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/"};
const char pad_char{'='};


std::string encode_msg_base64(const Message& msg) {
    return encode_base64(msg.SerializeAsString());
}

Message decode_base64_msg_stream(std::istream& msg_stream) {
    string msg_str{};
    getline(msg_stream, msg_str);

    Message msg{};
    msg.ParseFromString(decode_base64(msg_str));
    return msg;
}

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

string decode_base64(const string& to_decode) {
    string decoded{""};

    unsigned int cumulated{0};
    unsigned short bits_in_cumulated{0};

    for (unsigned char c : to_decode) {
        // cumulate base 64 chars to number
        cumulated <<= 6;
        cumulated += distance(base64_chars, find(base64_chars, base64_chars + 64, c));
        bits_in_cumulated += 6;

        if (bits_in_cumulated >= 8) {
            unsigned short shift(bits_in_cumulated - 8);

            // convert cumulated base 64 chars to 8 bit chars
            unsigned int char_val{cumulated >> shift};
            cumulated -= (char_val << shift);
            bits_in_cumulated -= 8;
            decoded += char(char_val);
        }
    }

    // check for padding chars and remove decoded chars accordingly
    if (to_decode[to_decode.length() - 1] == pad_char) {
        decoded.pop_back();

        if (to_decode[to_decode.length() - 2] == pad_char) {
            decoded.pop_back();
        }
    }

    return decoded;
}
