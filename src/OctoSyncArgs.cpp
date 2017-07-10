#include "OctoSyncArgs.h"

const string OctoSyncArgs::syncCmdArgName = "sync";
const string OctoSyncArgs::hostCmdArgName = "host";
const string OctoSyncArgs::syncDirArgName = "syncDir";
const string OctoSyncArgs::hostAddressArgName = "hostAddress";
const string OctoSyncArgs::hostPortArgName = "hostPort";
const string OctoSyncArgs::localPasswdArgName = "localPasswd";
const string OctoSyncArgs::hostPasswdArgName = "hostPasswd";
const string OctoSyncArgs::scpPortArgName = "scpPort";
const string OctoSyncArgs::loggingLevelArgName = "logLevel";

const string OctoSyncArgs::configFileName = ".syncConfig";

OctoSyncArgs::OctoSyncArgs(int argc, char * argv[])
: ArgParser(argc, argv)
{
    if(!helpOperation()){
        sync = operation == syncCmdArgName;
        host = operation == hostCmdArgName;
        if(!sync && !host){
            
            error("OCTO-SYNC-ARGS", string("Invalid operation: ") + operation);
            success = false;
        }else{
            if(args.find(syncCmdArgName) != args.end()){
                syncDir = args[syncCmdArgName];
            }
            readValuesFromConfigFile();
            success = findArgs();
            //printGivenArgs();
            writeValuesToConfigFile();
        }
        
    }else{
        printHelp();
    }
}

void OctoSyncArgs::readValuesFromConfigFile(){
    if(syncDir.length()>0){
        bool lastCharIsSlash = syncDir[syncDir.length()-1] == '/';
        string file = syncDir;
        if(!lastCharIsSlash){
            file = file + string("/");
        }
        file = file + string(configFileName);
        if(fileExists(file.c_str())){
            ifstream inputStream;
            inputStream.open(file);
            int isSync;
            inputStream >> isSync;
            sync = (bool)isSync;
            int isHost;
            inputStream >> isHost;
            host = (bool)isHost;
            inputStream >> syncDir;
            inputStream >> hostAddress;
            inputStream >> localPasswd;
            inputStream >> hostPasswd;
            inputStream >> hostPort;
            inputStream >> scpPort;
            inputStream >> loggingLevel;
            inputStream.close();
        }
    }

}

void OctoSyncArgs::writeValuesToConfigFile(){
    if(syncDir.length()>0){
        bool lastCharIsSlash = syncDir[syncDir.length()-1] == '/';
        string file = syncDir;
        if(!lastCharIsSlash){
            file = file + string("/");
        }
        file = file + string(configFileName);
        ofstream outputStream;
        outputStream.open(file, ios::out | ios::trunc);
        outputStream << (int)sync << endl;
        outputStream << (int)host << endl;
        outputStream << syncDir << endl;
        outputStream << hostAddress << endl;
        outputStream << localPasswd << endl;
        outputStream << hostPasswd << endl;
        outputStream << hostPort << endl;
        outputStream << scpPort << endl;
        outputStream << loggingLevel << endl;
        outputStream.close();
    }

}

bool OctoSyncArgs::findArgs(){
    if(args.find(hostAddressArgName) != args.end()){
        hostAddress = args[hostAddressArgName];
    }

    if(args.find(hostPortArgName) != args.end()){
        hostPort = stoi(args[hostPortArgName]);
    }

    if(args.find(localPasswdArgName) != args.end()){
        localPasswd = args[localPasswdArgName];
    }

    if(args.find(hostPasswdArgName) != args.end()){
        hostPasswd = args[hostPasswdArgName];
    }

    if(args.find(scpPortArgName) != args.end()){
        scpPort = stoi(args[scpPortArgName]);
    }

    if(args.find(loggingLevelArgName) != args.end()){
        loggingLevel = stoi(args[loggingLevelArgName]);
    }

    return validateArgs();
}

bool OctoSyncArgs::validateArgs(){
    /*if(syncDir == ""){
        error("OCTO-SYNC-ARGS", string("No synchronization directory defined"));
        return false;
    }*/
    if(hostAddress == "none"){
        error("OCTO-SYNC-ARGS", string("No host address defined"));
        return false;
    }
    if(localPasswd == "none"){
        cout << "Enter user password: ";
        string passwd = getSecretString();
        cout << endl;
        if(passwd != ""){
            localPasswd = passwd;
        }else{
            error("OCTO-SYNC-ARGS", string("No local password defined"));
            return false;
        }
    }
    if(hostPasswd == "none"){
        cout << "Enter host password: ";
        string passwd = getSecretString();
        cout << endl;
        if(passwd != ""){
            hostPasswd = passwd;
        }else{
            error("OCTO-SYNC-ARGS", string("No host password defined"));
            return false;
        }
    }
    if(hostPort == -1){
        error("OCTO-SYNC-ARGS", string("No host port defined"));
        return false;
    }
    //if(scpPort != -1){
    //    cout << "SCP port: \n\t" << scpPort << endl; 
    //}
    return true;
}

void OctoSyncArgs::printGivenArgs(){
    if(sync){
        cout << "SYNC";
    }else{
        cout << "HOST";
    }
    cout << " operation;" << endl;
    if(syncDir != "./"){
        cout << "Synchronization directory: \n\t" << syncDir << endl; 
    }
    if(hostAddress != "none"){
        cout << "Host address: \n\t" << hostAddress << endl; 
    }
    if(localPasswd != "none"){
        cout << "Local password: \n\t ***"  << endl; 
    }
    if(hostPasswd != "none"){
        cout << "Host password: \n\t" << hostPasswd << endl; 
    }
    if(hostPort != -1){
        cout << "Host port: \n\t" << hostPort << endl; 
    }
    if(scpPort != -1){
        cout << "SCP port: \n\t" << scpPort << endl; 
    }
    if(loggingLevel != 7){
        cout << "Logging level: \n\t" << loggingLevel << endl; 
    }
}

void OctoSyncArgs::printHelp(){
    cout << syncCmdArgName << " or " << hostCmdArgName << endl
    << "\t" << "The operation to perform: either host or sync;" << endl
    << "\t" << "The host must be the first instance to be executed, it will wait for a connection;" << endl
    << "\t" << "The sync connects to a host that is waiting for a new connection;" << endl
    << "Use [arg-name]=[value] to pass arguments:" << endl
    << "* = obrigatory" << endl
    << syncDirArgName << endl
    << "\t" << " The directory to synchronize. Default = ./;" << endl
    << hostAddressArgName << endl
    << "\t" << "* Hosting machine ip;" << endl
    << hostPortArgName << endl
    << "\t" << " Hosting machine port. Default = " << DEFAULT_HOST_PORT << ";" << endl
    << localPasswdArgName << endl
    << "\t" << " Local machine (sync or host) user password."
            << " The remote will use this password on the scp command;" << endl
    << hostPasswdArgName << endl
    << "\t" << "* Server password of the host. Used to login;" << endl
    << scpPortArgName << endl
    << "\t" << "Specify a port for the scp command. If not specified, the default will be used;" << endl
    << loggingLevelArgName << endl
    << "\t" << "Minimum level of log messages. A lower value means more output. Default = 7;" << endl;
}

char getch(){
    /*#include <unistd.h>   //_getch*/
    /*#include <termios.h>  //_getch*/
    char buf=0;
    struct termios old={0};
    fflush(stdout);
    if(tcgetattr(0, &old)<0)
    	perror("tcsetattr()");
    old.c_lflag&=~ICANON;
    old.c_lflag&=~ECHO;
    old.c_cc[VMIN]=1;
    old.c_cc[VTIME]=0;
    if(tcsetattr(0, TCSANOW, &old)<0)
    	perror("tcsetattr ICANON");
    if(read(0,&buf,1)<0)
    	perror("read()");
    old.c_lflag|=ICANON;
    old.c_lflag|=ECHO;
    if(tcsetattr(0, TCSADRAIN, &old)<0)
    	perror ("tcsetattr ~ICANON");
    //printf("%c\n",buf);
    return buf;
}

string OctoSyncArgs::getSecretString(){
    //initscr();
    string passwd = "";
    char tempchar[2] = "";  // Temporary character array 
    int key;  // Holds the key number 
    key = (int)getch();
    while(key != 10 /*Enter*/)  
    {
        if (key == 127  && passwd.length() > 0) {/*Backspace*/
            passwd = passwd.substr(0, passwd.length()-1);
        }else{
            tempchar[0] = (char)key; // Copy the char to an array 
            tempchar[1] = (char)0; // with a null at the end
            passwd = passwd + string(tempchar);
        }
        key = getch();  // Grab a character  
    }
    return passwd;
}