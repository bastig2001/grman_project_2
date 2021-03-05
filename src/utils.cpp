#include "utils.h"

#include <sstream>

using namespace std;

unsigned int get_value_of_first_three_chars(istream&);
string get_base64_chars(unsigned int);

const char base64_chars[]{"ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/"};
const char pad_char{'='};


stringstream encode_base64(istream& to_encode) {
    stringstream encoded{};
    to_encode.unsetf(ios_base::skipws);

    while (to_encode.rdbuf()->in_avail() >= 3) {
        encoded << get_base64_chars(get_value_of_first_three_chars(to_encode));
    }

    if (to_encode.rdbuf()->in_avail() > 0) {
        streamsize remaining{to_encode.rdbuf()->in_avail()};
        stringstream last_to_encode{};
        last_to_encode.unsetf(ios_base::skipws);

        for (short i{0}; i < remaining; i++) {
            unsigned char c{};
            to_encode >> c;
            last_to_encode << c;
        }
        for (short i{3}; i > remaining; i--) {
            last_to_encode << char(0);
        }

        string last_encoded{
            get_base64_chars(get_value_of_first_three_chars(last_to_encode))
        };

        for (short i{0}; i <= remaining; i++) {
            encoded << last_encoded[i];
        }
        for (short i{3}; i > remaining; i--) {
            encoded << pad_char;
        }
    }

    return encoded;
}

unsigned int get_value_of_first_three_chars(istream& chars) {
    unsigned int chars_value{};

    for (short i{2}; i >= 0; i--) {
        unsigned char c{};
        chars >> c;
        chars_value += int(c) << (8 * i);
    }

    return chars_value;
}

string get_base64_chars(unsigned int chars_value) {
    ostringstream chars{};

    for (short i{3}; i >= 0; i--) {
        unsigned int c{chars_value >> (6 * i)};
        chars_value -= c << (6 * i);
        chars << base64_chars[c];
    }

    return chars.str();
}

stringstream decode_base64(istream&) {
    return stringstream{};
}
