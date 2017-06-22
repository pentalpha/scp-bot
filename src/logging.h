#ifndef LOGGING_H
#define LOGGING_H

#include <iostream>
#include <string>
#include <mutex>
#include <vector>

void log(std::string origin, std::string message);
void error(std::string origin, std::string message);
std::string vectorToStr(std::vector<std::string> words);
std::string vectorToStr(std::vector<int> v);

#endif // LOGGING_H
