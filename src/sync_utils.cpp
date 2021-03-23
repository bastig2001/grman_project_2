#include "sync_utils.h"

#include <algorithm>
#include <cmath>
#include <fmt/core.h>
#include <openssl/md5.h>
#include <iterator>
#include <tuple>

using namespace std;

string unsigned_char_to_hexadecimal_string(unsigned char*, unsigned int);
tuple<unsigned int, unsigned int, unsigned int> calc_weak_signature(
    const string&, 
    unsigned long, 
    unsigned long
);
tuple<unsigned int, unsigned int, unsigned int> increment_weak_signature(
    const string&, 
    unsigned long,
    unsigned long, 
    unsigned int,
    unsigned int
);

const unsigned int signature_modulus = pow(2, 16);


string get_strong_signature(const string& bytes) {
    unsigned char digest[MD5_DIGEST_LENGTH];
    MD5((unsigned char*)bytes.c_str(), bytes.length(), digest);
    return unsigned_char_to_hexadecimal_string(digest, MD5_DIGEST_LENGTH);
}

string unsigned_char_to_hexadecimal_string(
    unsigned char* arr, 
    unsigned int n
) {
    string result{};
    for (unsigned int i{0}; i < n; i++) {
        result += fmt::format("{:02x}", (unsigned int)arr[i]);
    }
    return result;
}

unsigned int get_weak_signature(
    const std::string& data, 
    unsigned long offset, 
    unsigned long block_size
) {
    return get<2>(calc_weak_signature(data, offset, block_size));
}

vector<unsigned int> get_weak_signatures(
    const string& data, 
    unsigned long initial_offset, 
    unsigned long block_size
) {
    unsigned long number_of_signatures{data.length() - block_size + 1};
    vector<unsigned int> signatures{};
    signatures.reserve(number_of_signatures);

    auto [r1, r2, signature]{
        calc_weak_signature(
            data, 
            initial_offset, 
            block_size
    )};
    signatures.push_back(signature);
    
    for (unsigned int i{0}; i < number_of_signatures - 1; i++) {
        auto [new_r1, new_r2, signature]{
            increment_weak_signature(
                data, 
                initial_offset + i, 
                block_size,
                r1,
                r2
        )};
        signatures.push_back(signature);
        r1 = new_r1;
        r2 = new_r2;
    }

    return signatures;
}

tuple<unsigned int, unsigned int, unsigned int> calc_weak_signature(
    const string& data, 
    unsigned long offset, 
    unsigned long block_size
) {
    unsigned int r1{0};
    for_each(data.begin() + offset, data.begin() + offset + block_size, 
        [&](unsigned char c){
            r1 += c;
            r1 %= signature_modulus;
    });

    unsigned int r2{0};
    unsigned int i{0};
    for_each(data.begin() + offset, data.begin() + offset + block_size, 
        [&](unsigned char c){
            r2 += (block_size - i) * c;
            r2 %= signature_modulus;
            i++;
    });

    return {r1, r2 , r1 + signature_modulus * r2};
}

tuple<unsigned int, unsigned int, unsigned int> increment_weak_signature(
    const string& data, 
    unsigned long preceding_offset, 
    unsigned long block_size, 
    unsigned int preceding_r1,
    unsigned int preceding_r2
) {
    unsigned int r1 = (
            preceding_r1 
            - (unsigned char)data[preceding_offset] 
            + (unsigned char)data[preceding_offset + block_size]
        ) % signature_modulus
    ;
    unsigned int r2 = (
            preceding_r2
            - block_size * (unsigned char)data[preceding_offset]
            + r1
        ) % signature_modulus
    ;

    return {r1, r2, r1 + signature_modulus * r2};
}
