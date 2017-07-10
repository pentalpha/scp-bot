#ifndef _ARG_PARSER_
#define _ARG_PARSER_

#include <iostream>
#include <string>
#include <map>
#include <vector>
#include "logging.h"

using namespace std;

class ArgParser{
public:
    ArgParser(int argc, char * argv[]);
    bool helpOperation();
    string exec;
    string operation;
    map<string, string> args;
    int argCount;
protected:
    vector<string> rawArgs;
private:
    pair<string, string> separateRawArg(string arg);
};

#endif