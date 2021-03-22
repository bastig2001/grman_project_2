#include "utils.h"

#include <fmt/core.h>
#include <openssl/md4.h>
#include <iterator>
#include <unordered_map>

using namespace std;

const char base64_chars[]{"ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/"};
const char pad_char{'='};
const unordered_map<char, unsigned short> base64_vals{
    {'A', 0}, {'B', 1}, {'C', 2}, {'D', 3}, {'E', 4}, {'F', 5}, {'G', 6}, {'H', 7}, 
    {'I', 8}, {'J', 9}, {'K', 10}, {'L', 11}, {'M', 12}, {'N', 13}, {'O', 14}, 
    {'P', 15}, {'Q', 16}, {'R', 17}, {'S', 18}, {'T', 19}, {'U', 20}, {'V', 21}, 
    {'W', 22}, {'X', 23}, {'Y', 24}, {'Z', 25}, {'a', 26}, {'b', 27}, {'c', 28}, 
    {'d', 29}, {'e', 30}, {'f', 31}, {'g', 32}, {'h', 33}, {'i', 34}, {'j', 35}, 
    {'k', 36}, {'l', 37}, {'m', 38}, {'n', 39}, {'o', 40}, {'p', 41}, {'q', 42}, 
    {'r', 43}, {'s', 44}, {'t', 45}, {'u', 46}, {'v', 47}, {'w', 48}, {'x', 49}, 
    {'y', 50}, {'z', 51}, {'0', 52}, {'1', 53}, {'2', 54}, {'3', 55}, {'4', 56}, 
    {'5', 57}, {'6', 58}, {'7', 59}, {'8', 60}, {'9', 61}, {'+', 62}, {'/', 63},
    {pad_char, 0}
};

string unsigned_char_to_string(unsigned char*, unsigned int);


std::string msg_to_base64(const Message& msg) {
    return to_base64(msg.SerializeAsString());
}

Message msg_from_base64(std::istream& msg_stream) {
    string msg_str{};
    getline(msg_stream, msg_str);

    Message msg{};
    msg.ParseFromString(from_base64(msg_str));
    return msg;
}

string to_base64(const string& to_encode) {
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

string from_base64(const string& to_decode) {
    string decoded{""};

    unsigned int cumulated{0};
    unsigned short bits_in_cumulated{0};

    for (unsigned char c : to_decode) {
        // cumulate base 64 chars to number
        cumulated <<= 6;
        cumulated += base64_vals.at(c);
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

string get_md4_hash(const string& bytes) {
    unsigned char digest[MD4_DIGEST_LENGTH];
    MD4((unsigned char*)bytes.c_str(), bytes.length(), digest);
    return unsigned_char_to_string(digest, MD4_DIGEST_LENGTH);
}

string unsigned_char_to_string(unsigned char* arr, unsigned int n) {
    string result{};
    for (unsigned int i{0}; i < n; i++) {
        result += fmt::format("{:02x}", (unsigned int)arr[i]);
    }
    return result;
}
