#include "config.h"

#include <iostream>

using namespace std;


int main(int argc, char* argv[]) {
    auto config_result = configure(argc, argv);
    
    if (auto config = get_if<Config>(&config_result)) {
        cout << "This program was called correctly.\n"
             << "The config is:\n" << config << endl;
    }
    else {
        return get<int>(config_result);
    }
}

