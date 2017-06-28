#include "SyncBot.h"

SyncBot::SyncBot(OctoSyncArgs args)
: localDirName(getAbsolutePath(args.syncDir)), localDir(getAbsolutePath(args.syncDir), true), remoteDir()
{
    isServer = args.host;
    localDirName = getAbsolutePath(args.syncDir);
    hostPort = args.hostPort;
    hostAddress = args.hostAddress;
    hostPasswd = args.hostPasswd;
    hostPasswdTry = "";

    localPasswd = args.localPasswd;
    remotePasswd = "";

    if(args.scpPort != -1){
        scpPort = args.scpPort;
    }

    remoteUpdating = localUpdating = authByRemote = false;

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
    log("SYNC-BOT", "Running SyncBot and opening for new connections");
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

//////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////

void SyncBot::updateCycle(){
    log("SYNC-BOT", "Starting to update");
    while(!finishFlag){
        update();
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
    }else if(state == SYNC){
        sync();
    }
}

//////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////

void SyncBot::sleeping(){
    //cout << "asleep\n";
    if(!(socket->isAsleep())){
        state = WAITING;
        log("SYNC-BOT", "SyncBot is now WAITING");
    }else{
        updateLocalDirIfNotBusy();
    }
}

//////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////

void SyncBot::waiting(){
    if(socket->isConnected()){
        //cout << "not connected\n";
        state = AUTH;
        log("SYNC-BOT", "SyncBot is now in AUTH");
        authState = NOT_STARTED;
        log("SYNC-BOT", "SyncBot has NOT_STARTED AUTH");
    }else{
        //cout << "not connected\n";
        updateLocalDirIfNotBusy();
    }
}

//////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////

void SyncBot::authentication(){
    if (authState == NOT_STARTED){
        if(isServer && someoneTryedToLogin()){
            //log("SYNC-BOT", "Someone tryed to login on server");
            if(correctServerPassword()){
                log("SYNC-BOT-HOST", "Sync client logged in to server");
                bool sent = sendAuthMessage();
                if(sent){
                    authState = WAITING_REMOTE_AUTH;
                    log("SYNC-BOT-HOST", "SyncBot is now WAITING_REMOTE_AUTH");
                }
            }else{
                log("SYNC-BOT-HOST", "Sync client tryed to login with incorrect password");
            }
        }else if(!isServer){
            bool sent = sendLoginMessage();
            if(sent){
                authState = WAITING_REMOTE_AUTH;
                log("SYNC-BOT-CLIENT", "SyncBot is now WAITING_REMOTE_AUTH");
            }
        }
    }else if (authState == WAITING_REMOTE_AUTH){
        if(hasRemoteAuthorization()){
            if(!isServer){
                bool sent = sendAuthMessage();
                if(sent){
                    authState = AUTHORIZED;
                    log("SYNC-BOT-CLIENT", "SyncBot is now AUTHORIZED");
                }
            }else{
                authState = AUTHORIZED;
                log("SYNC-BOT-HOST", "SyncBot is now AUTHORIZED");
            }
        }
    }else if(authState == AUTHORIZED){
        state = SYNC;
        log("SYNC-BOT", "SyncBot is now in SYNC");
    }
}

bool SyncBot::someoneTryedToLogin(){
    std::lock_guard<std::mutex> guard(loginMutex);
    return (hostPasswdTry != "");
}

bool SyncBot::correctServerPassword(){
    std::lock_guard<std::mutex> guard(loginMutex);
    return (hostPasswdTry == hostPasswd);
}

void SyncBot::setHostPasswdTry(string newTry){
    std::lock_guard<std::mutex> guard(loginMutex);
    hostPasswdTry = newTry;
}

bool SyncBot::sendLoginMessage(){
    string msg = "";
    msg += "login ";
    msg += hostPasswd;
    msg += "\n";
    //log("SYNC-BOT", string("Trying to send login message: ") + msg);
    return socket->sendMsg(msg);
}

bool SyncBot::sendAuthMessage(){
    string msg = "";
    msg += "auth ";
    msg += localPasswd;
    msg += " ";
    msg += localDirName;
    if(scpPort != -1){
        msg += " ";
        msg += to_string(scpPort);
    }
    msg += "\n";
    //log("SYNC-BOT", string("Trying to send auth message: ") + msg);
    return socket->sendMsg(msg);
}

bool SyncBot::hasRemoteAuthorization(){
    return authByRemote;
}

//////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////

void SyncBot::sync(){
    updateLocalDirIfNotBusy();
}

void SyncBot::updateLocalDirIfNotBusy(){
    if(localDir.isUpdating() == false){
        localDir.updateFilesAndDirs(true);
    }
}

//////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////

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

void SyncBot::treatMessage(string message){
    istringstream sstream(message);
    list<string> words;
    string newWord;
    while(sstream >> newWord){
        words.push_back(newWord);
    }
    if(words.size() > 0){
        string op = words.front();
        words.erase(words.begin());
        if(op == "login" && words.size() == 1){
            login(words.front());
        }else if(op == "auth" && words.size() >= 2){
            auto it = words.begin();
            string userPassword = *it;
            it++;
            string remoteDir = *it;
            int transferPort = -1;
            if(words.size() > 2){
                it++;
                string transferPortStr = *it;
                transferPort = stoi(transferPortStr);
            }
            auth(userPassword, remoteDir, transferPort);
        }else{
            error("SYNC-BOT", string("Invalid message to treat: ") + message);
        }
    }else{
        error("SYNC-BOT", "Cannot treat empty message");
    }
}

void SyncBot::login(string password){
    std::lock_guard<std::mutex> guard(loginMutex);
    log("SYNC-BOT", string("Treating login ") + password);
    hostPasswdTry = password;
}

void SyncBot::auth(string userPassword, string remoteSyncDir, int transferPort){
    log("SYNC-BOT", string("Treating auth ") + userPassword + string(" ") + remoteSyncDir);
    if(transferPort > 0){
        remoteScpPort = transferPort;
    }
    remotePasswd = userPassword;
    remoteDir.setDir(remoteSyncDir);
    authByRemote = true;
    //log("SYNC-BOT", "Treated auth and authorized");
}