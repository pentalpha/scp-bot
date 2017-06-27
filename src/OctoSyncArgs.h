#ifndef _OCTO_SYNC_ARGS_
#define _OCTO_SYNC_ARGS_

#include <ncurses.h>
#include <fstream>
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
    string hostAddress = "";
    string localPasswd = "";
    int hostPort = DEFAULT_HOST_PORT;
    string hostPasswd = "";
    int scpPort = -1;

    static const string syncCmdArgName;
    static const string hostCmdArgName;
    static const string syncDirArgName;
    static const string hostAddressArgName;
    static const string hostPortArgName;
    static const string localPasswdArgName;
    static const string hostPasswdArgName;
    static const string scpPortArgName;

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
    */
    static const string configFileName;
    void readValuesFromConfigFile();
    void writeValuesToConfigFile();

    static string getSecretString();
};

#endif