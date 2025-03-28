#ifndef IPREADER_HPP
#define IPREADER_HPP

#pragma once
#include <string>

// Returns the saved server IP address from the specified file.
std::string getSavedServerIP(const std::string& filename = "server_ip.txt");


#endif