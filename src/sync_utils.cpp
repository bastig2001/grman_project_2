#include "sync_utils.h"

#include <algorithm>
#include <cmath>
#include <fmt/core.h>
#include <openssl/md5.h>
#include <iterator>
#include <tuple>

using namespace std;

string unsigned_char_to_hexadecimal_string(unsigned char*, unsigned int);
tuple<unsigned int, unsigned int, unsigned int> calc_checksum(
    const string&, 
    unsigned long, 
    unsigned long
);
tuple<unsigned int, unsigned int, unsigned int> increment_checksum(
    const string&, 
    unsigned long,
    unsigned long, 
    unsigned int,
    unsigned int
);

const unsigned int checksum_modulus = pow(2, 16);


string get_md5_hash(const string& bytes) {
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

unsigned int get_rolling_checksum(
    const std::string& data, 
    unsigned long offset, 
    unsigned long block_size
) {
    return get<2>(calc_checksum(data, offset, block_size));
}

vector<unsigned int> get_rolling_checksums(
    const string& data, 
    unsigned long initial_offset, 
    unsigned long block_size
) {
    unsigned long number_of_checksums{data.length() - block_size + 1};
    vector<unsigned int> checksums{};
    checksums.reserve(number_of_checksums);

    auto [r1, r2, checksum]{
        calc_checksum(
            data, 
            initial_offset, 
            block_size
    )};
    checksums.push_back(checksum);
    
    for (unsigned int i{0}; i < number_of_checksums - 1; i++) {
        auto [new_r1, new_r2, checksum]{
            increment_checksum(
                data, 
                initial_offset + i, 
                block_size,
                r1,
                r2
        )};
        checksums.push_back(checksum);
        r1 = new_r1;
        r2 = new_r2;
    }

    return checksums;
}

tuple<unsigned int, unsigned int, unsigned int> calc_checksum(
    const string& data, 
    unsigned long offset, 
    unsigned long block_size
) {
    unsigned int r1{0};
    for_each(data.begin() + offset, data.begin() + offset + block_size, 
        [&](unsigned char c){
            r1 += c;
            r1 %= checksum_modulus;
    });

    unsigned int r2{0};
    unsigned int i{0};
    for_each(data.begin() + offset, data.begin() + offset + block_size, 
        [&](unsigned char c){
            r2 += (block_size - i) * c;
            r2 %= checksum_modulus;
            i++;
    });

    return {r1, r2 , r1 + checksum_modulus * r2};
}

tuple<unsigned int, unsigned int, unsigned int> increment_checksum(
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
        ) % checksum_modulus
    ;
    unsigned int r2 = (
            preceding_r2
            - block_size * (unsigned char)data[preceding_offset]
            + r1
        ) % checksum_modulus
    ;

    return {r1, r2, r1 + checksum_modulus * r2};
}
