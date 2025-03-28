#include "IPReader.hpp"
#include <fstream>
#include <iostream>
#include <string>

std::string getSavedServerIP(const std::string& filename) {
    std::ifstream infile(filename);
    std::string ip;
    if (infile.is_open()) {
        std::getline(infile, ip);
        infile.close();
    }
    else {
        std::cerr << "Failed to open file: " << filename << std::endl;
    }
    return ip;
}
