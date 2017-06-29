#include "logging.h"

std::mutex loggingMutex;

int loggingLevelRequired = 0;

void setLogLevel(int newLogLevel){
    loggingMutex.lock();
    loggingLevelRequired = newLogLevel;
    loggingMutex.unlock();
}

void log(int logLevel, std::string origin, std::string message){
    if(logLevel >= loggingLevelRequired){
        loggingMutex.lock();
        std::clog << "[" << origin << "] " << message << std::endl;
        loggingMutex.unlock();
    }
}

void error(std::string origin, std::string message){
    loggingMutex.lock();
    std::cerr << "[" << origin << "-ERROR] " << message << std::endl;
    loggingMutex.unlock();
}

std::string vectorToStr(std::vector<std::string> words){
    std::string str = "";
    for(auto x : words){
        str += std::string(" ") + x;
    }
    return str;
}

std::string vectorToStr(std::vector<int> v){
    std::string str = "";
    bool first = true;
    for(int x : v){
        if(first){
            first = false;
        }else{
            str += std::string("_");
        }
        str += std::to_string(x);
    }
    return str;
}

