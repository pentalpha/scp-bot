#include "ArgParser.h"

ArgParser::ArgParser(int argc, char * argv[]){
    argCount = argc;
    exec = string(argv[0]);
    if(argc >= 2){
        for(int i = 1; i < argCount; i++){
            rawArgs.push_back(string(argv[i]));
        }
        operation = rawArgs[0];
        for(int i = 1; i < rawArgs.size(); i++){
            string arg = rawArgs[i];
            pair<string, string> x = separateRawArg(arg);
            if(x.first == "ERROR"){
                log("ARG-PARSER", string("'") + arg + string("' can not be parsed."));
            }else{
                args[x.first] = x.second;
            }
        }
    }
}

pair<string, string> ArgParser::separateRawArg(string arg){
    pair<string, string> res;
    int equalChar = -1;
    for(int i = 1; i < arg.length()-1; i++){
        if(arg[i] == '='){
            equalChar = i;
            break;
        }
    }
    if(equalChar == -1){
        res.first = "ERROR";
        res.second = "ERROR";
    }else{
        res.first = arg.substr(0,equalChar);
        res.second = arg.substr(equalChar+1, arg.length());
    }
    return res;
}

bool ArgParser::helpOperation(){
    return (operation == "-h" || operation == "--help");
}