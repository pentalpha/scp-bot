#include "OctoSyncArgs.h"

const string OctoSyncArgs::syncCmdArgName = "sync";
const string OctoSyncArgs::hostCmdArgName = "host";
const string OctoSyncArgs::syncDirArgName = "syncDir";
const string OctoSyncArgs::hostAddressArgName = "hostAddress";
const string OctoSyncArgs::hostPortArgName = "hostPort";
const string OctoSyncArgs::localPasswdArgName = "localPasswd";
const string OctoSyncArgs::hostPasswdArgName = "hostPasswd";
const string OctoSyncArgs::scpPortArgName = "scpPort";

const string OctoSyncArgs::configFileName = ".octoConfig";

OctoSyncArgs::OctoSyncArgs(int argc, char * argv[])
: ArgParser(argc, argv)
{
    sync = operation == syncCmdArgName;
    host = operation == hostCmdArgName;
    if(!sync && !host){
        if(helpOperation()){
            printHelp();
        }
        log("OCTO-SYNC-ARGS", string("Invalid operation: ") + operation);
        success = false;
    }else{
        if(args.find(syncCmdArgName) != args.end()){
            syncDir = args[syncCmdArgName];
        }
        readValuesFromConfigFile();
        success = findArgs();
    }
    printGivenArgs();
    writeValuesToConfigFile();
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
            inputStream >> hostPort;
            inputStream >> localPasswd;
            inputStream >> hostPasswd;
            inputStream >> hostPort;
            inputStream >> scpPort;
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
        outputStream << hostPort << endl;
        outputStream << localPasswd << endl;
        outputStream << hostPasswd << endl;
        outputStream << hostPort << endl;
        outputStream << scpPort << endl;
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

    return validateArgs();
}

bool OctoSyncArgs::validateArgs(){
    if(syncDir == ""){
        log("OCTO-SYNC-ARGS", string("No synchronization directory defined"));
        return false;
    }
    if(hostAddress == ""){
        log("OCTO-SYNC-ARGS", string("No host address defined"));
        return false;
    }
    if(localPasswd == ""){
        cout << "Enter user password: ";
        string passwd = getSecretString();
        if(passwd != ""){
            localPasswd = passwd;
        }else{
            log("OCTO-SYNC-ARGS", string("No local password defined"));
            return false;
        }
    }
    if(hostPasswd == ""){
        if(sync){
            cout << "Enter host password: ";
            string passwd = getSecretString();
            if(passwd != ""){
                hostPasswd = passwd;
            }else{
                log("OCTO-SYNC-ARGS", string("No host password defined"));
                return false;
            }
        }
    }
    if(hostPort == -1){
        log("OCTO-SYNC-ARGS", string("No host port defined"));
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
    if(syncDir != ""){
        cout << "Synchronization directory: \n\t" << syncDir << endl; 
    }
    if(hostAddress != ""){
        cout << "Host address: \n\t" << hostAddress << endl; 
    }
    if(localPasswd != ""){
        cout << "Local password: \n\t" << localPasswd << endl; 
    }
    if(hostPasswd != ""){
        cout << "Host password: \n\t" << hostPasswd << endl; 
    }
    if(hostPort != -1){
        cout << "Host port: \n\t" << hostPort << endl; 
    }
    if(scpPort != -1){
        cout << "SCP port: \n\t" << scpPort << endl; 
    }
}

void OctoSyncArgs::printHelp(){
    cout << syncCmdArgName << " or " << hostCmdArgName << endl
    << "\t" << "The operation to perform: either host or sync" << endl
    << "Use [arg-name]=[value] to pass arguments:" << endl
    << "* = obrigatory" << endl
    << "# = obrigatory to host" << endl
    << "$ = obrigatory to sync" << endl
    << syncDirArgName << endl
    << "\t" << "* The directory to synchronize" << endl
    << hostAddressArgName << endl
    << "\t" << "* Hosting machine ip" << endl
    << hostPortArgName << endl
    << "\t" << "* Hosting machine port" << endl
    << localPasswdArgName << endl
    << "\t" << "* Local machine (sync or host) user password" << endl
    << hostPasswdArgName << endl
    << "\t" << "$ Password of the host" << endl
    << scpPortArgName << endl
    << "\t" << "Specify a port for the scp command" << endl;
}

string OctoSyncArgs::getSecretString(){
    string passwd = "";
    char tempchar[2] = "";  // Temporary character array 
    char key;  // Holds the key number 
    key = getch();
    while(key != 13 /*Enter*/)  
    {
        if (key == 8  && passwd.length() > 0) {/*Backspace*/
            passwd = passwd.substr(0, passwd.length()-1);
        }else{
            tempchar[0] = key; // Copy the char to an array 
            tempchar[1] = 0; // with a null at the end
            passwd = passwd + string(tempchar);
        }
        key = getch();  // Grab a character  
    }
    return passwd;
}