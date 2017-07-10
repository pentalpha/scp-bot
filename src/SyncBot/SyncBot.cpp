#include "../../include/SyncBot.h"

SyncBot::SyncBot(SyncArgs args)
: localDirName(getAbsolutePath(args.syncDir)), localDir(getAbsolutePath(args.syncDir), true), remoteDir()
{
    setLogLevel(args.loggingLevel);

    isServer = args.host;
    localDirName = getAbsolutePath(args.syncDir);
    remoteDirName = "";
    hostPort = args.hostPort;
    hostAddress = args.hostAddress;
    hostPasswd = args.hostPasswd;
    hostPasswdTry = "";

    localPasswd = args.localPasswd;
    remotePasswd = "";
    localUserName = getUserName();
    remoteUserName = "";
    if(!isServer){
        remoteAddress = hostAddress;
    }else{
        remoteAddress = "0.0.0.0";
    }
    //cout << "user name " << localUserName << endl;
    if(args.scpPort != -1){
        scpPort = args.scpPort;
    }

    //remoteUpdating = localUpdating = false;
    authByRemote = false;
    remoteSharedAllInfoFlag = sharedAllInfoFlag = false;
    remoteSharedAllChangesFlag = sharedAllChangesFlag = false;
    syncAllowState = WAIT;

    socket = makeSocket(hostAddress, hostPort, isServer);
    if(socket == NULL){
        error("SYNC-BOT", "Error during socket creation");
        exit(1);
    }
    state = SLEEP;
    authState = NOT_STARTED;

    updateThread = new thread(&SyncBot::updateCycle, this);
    updateThread->detach();
    treatMsgThread = new thread(&SyncBot::treatMessagesCycle, this);
    treatMsgThread->detach();
}

string SyncBot::getUserName(){
    return string(getlogin());
}

list<string> SyncBot::splitStrInList(string s){
    istringstream sstream(s);
    list<string> words;
    string newWord;
    while(sstream >> newWord){
        words.push_back(newWord);
    }
    return words;
}

Socket* SyncBot::makeSocket(string ip, int port, bool server){
    Socket* socket;
    if(server){
        socket = new Server(ip.c_str(), port);
    }else{
        socket = new Client(ip.c_str(), port);
    }
    return socket;
}

bool SyncBot::run(){
    log(6,"SYNC-BOT", "Running SyncBot and opening for new connections");
    bool started = socket->startTransaction();
    if(started){
        socket->waitToFinish();
        finishFlag = true;
        updateThread->join();
        treatMsgThread->join();
        return true;
    }else{
        return false;
    }
}

void SyncBot::updateCycle(){
    log(2,"SYNC-BOT", "Starting to update");
    while(!finishFlag){
        update();
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
}

void SyncBot::update(){
    //cout << "update\n";
    if(state == SLEEP){
        sleeping();
    }else if(state == WAITING){
        waiting();
    }else if(state == AUTH){
        authentication();
    }else if(state == INFO_SHARE){
        infoShare();
    }else if(state == CHANGE_SHARE){
        changeShare();
    }else if(state == REALTIME_SYNC){
        sync();
    }
}

void SyncBot::treatMessagesCycle(){
    while(!finishFlag){
        while(true){
            string newMsg = socket->getMessage();
            if(newMsg != ""){
                treatMessage(newMsg);
            }else{
                break;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
    }
}

