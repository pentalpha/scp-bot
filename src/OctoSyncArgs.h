#ifndef _OCTO_SYNC_ARGS_
#define _OCTO_SYNC_ARGS_

#include <fstream>
#include <stdio.h>
#include <unistd.h>   //_getch*/
#include <termios.h>  //_getch*/
#include "ArgParser.h"
#include "FileUtils.h"

using namespace std;

#define DEFAULT_HOST_PORT 50002

class OctoSyncArgs : public ArgParser{
public:
    OctoSyncArgs(int argc, char * argv[]);

    bool findArgs();
    bool validateArgs();

    void printGivenArgs();
    void printHelp();

    bool success = false;
    bool sync = false;
    bool host = false;
    string syncDir = "./";
    string hostAddress = "none";
    string localPasswd = "none";
    int hostPort = DEFAULT_HOST_PORT;
    string hostPasswd = "none";
    int scpPort = -1;
    int loggingLevel = 7;

    static const string syncCmdArgName;
    static const string hostCmdArgName;
    static const string syncDirArgName;
    static const string hostAddressArgName;
    static const string hostPortArgName;
    static const string localPasswdArgName;
    static const string hostPasswdArgName;
    static const string scpPortArgName;
    static const string loggingLevelArgName;

    /*
    Sequence of values:
    sync
    host
    syncDir
    hostAddress
    hostPort
    localPasswd
    hostPasswd
    hostPort
    scpPort
    loggingLevel
    */
    static const string configFileName;
    void readValuesFromConfigFile();
    void writeValuesToConfigFile();

    static string getSecretString();
};

#endif