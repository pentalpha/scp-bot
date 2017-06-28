#include "SyncBot.h"

SyncBot::SyncBot(OctoSyncArgs args)
: localDirName(getAbsolutePath(args.syncDir)), localDir(getAbsolutePath(args.syncDir), true), remoteDir()
{
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

    remoteUpdating = localUpdating = false;
    syncAllowdByRemote = authByRemote = false;

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
    //log("SYNC-BOT", "Starting to update");
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
        if(isServer){
            remoteAddress = ((Server*)socket)->clientAddress;
            log("SYNC-BOT", string("Remote address is ") + remoteAddress);
        }
        //log("SYNC-BOT", "SyncBot has NOT_STARTED AUTH");
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
    msg += localUserName;
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
    if(remoteUpdating){
        if(localDir.hasChanges()){
            vector<string> changes = localDir.popChanges();
            for(string change : changes){
                while(remoteUpdating){
                    //wait remote to end his update
                }
                sendStartSync();
                while(!syncAllowdByRemote){

                }
                sendChangeMsg(change);
                syncAllowdByRemote = false;
                sendEndSync();
            }
        }else{
            while(remoteUpdating){
                //wait remote to end his update
            }
            localUpdating = true;
            updateLocalDirIfNotBusy();
            localUpdating = false;
        }
    }
}

void SyncBot::updateLocalDirIfNotBusy(){
    if(localDir.isUpdating() == false){
        localDir.updateFilesAndDirs(true);
    }
}

void SyncBot::sendChangeMsg(string change){
    list<string> words = splitStrInList(change);
    string op = words.front();
    bool up, file;
    if(op == "up-file"){
        up = true;
        file = true;
    }else if(op == "rm-file"){
        up = false;
        file = true;
    }else if(op == "up-dir"){
        up = true;
        file = false;
    }else if(op == "rm-dir"){
        up = false;
        file = false;
    }
    string obj = words.back();
    string noSyncDirObj = localDir.getFilePathWithoutSyncDir(obj);
    string remotePathObj = remoteDirName;
    if(remoteDirName[remoteDirName.length()-1] != '/'){
        remotePathObj += "/";
    }
    remotePathObj += noSyncDirObj;
    if(!file){
        if(up){
            sendDirAdd(obj);
            if(!remoteDir.hasDir(remotePathObj)){
                sendMkdir(remotePathObj);
            }
        }else{
            sendDirRemove(obj);
            if(remoteDir.hasDir(remotePathObj)){
                sendDeleteDir(remotePathObj);
            }
        }
    }else{
        if(up){
            time_t lastMod = localDir.getModTimeOfFile(obj);
            sendFileAdd(obj, lastMod);
            bool send = false;
            if(!remoteDir.hasFile(remotePathObj)){
                send = true;
            }else{
                time_t remoteLastMod = remoteDir.getModTimeOfFile(remotePathObj);
                if(lastMod > remoteLastMod + 2){
                    send = true;
                }
            }
            if(send){
                sendFile(obj, remotePathObj);
            }
        }else{
            sendFileRemove(obj);
            if(remoteDir.hasFile(remotePathObj)){
                sendDeleteFile(remotePathObj);
            }
        }
    }
}

void SyncBot::sendDirRemove(string dir){
    socket->sendMsg(string("dir rm ") + dir);
}

void SyncBot::sendDirAdd(string dir){
    socket->sendMsg(string("dir add ") + dir);
}

void SyncBot::sendFileRemove(string file){
    socket->sendMsg(string("file rm ") + file);
}

void SyncBot::sendFileAdd(string file, time_t lastMod){
    socket->sendMsg(string("file up ") + file + string(" ") + to_string(lastMod));
}

void SyncBot::sendStartSync(){
    localUpdating = true;
    socket->sendMsg("start-sync");
    
}

void SyncBot::sendAllowSync(){
    remoteUpdating = true;
    socket->sendMsg("allow-sync");
}

void SyncBot::sendEndSync(){
    localUpdating = false;
    socket->sendMsg("end-sync");
}

void SyncBot::sendMkdir(string dir){
    socket->sendMsg(string("mkdir ") + dir);
}

void SyncBot::sendDeleteDir(string dir){
    socket->sendMsg(string("rm -Rf ") + dir);
}

void SyncBot::sendFile(string localFile, string remoteFile){
    //socket->sendMsg(string("mkdir ") + dir);
    string cmd = "sshpass -p '";
    cmd += remotePasswd + string("' scp -p ");
    if(remoteScpPort > 0){
        cmd += string("-P ") + to_string(remoteScpPort) + string(" ");
    }
    cmd += localFile + string(" ");
    cmd += remoteUserName + string("@") + remoteAddress;
    cmd += string(":") + remoteFile;
    log("SYNC-BOT", string("Transfering with: ") + cmd);
    system(cmd.c_str());
    log("SYNC-BOT", "Finished trasfer");
}

void SyncBot::sendDeleteFile(string file){
    socket->sendMsg(string("rm -f ") + file);
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
    list<string> words = splitStrInList(message);
    if(words.size() > 0){
        string op = words.front();
        words.erase(words.begin());
        if(op == "start-sync" && words.size() == 0){
            remoteStartSync();
        }else if(op == "end-sync" && words.size() == 0){
            remoteEndSync();
        }else if(op == "login" && words.size() == 1){
            login(words.front());
        }else if(op == "mkdir" && words.size() == 1){
            mkdir(message, words.back());
        }else if(op == "rm" && words.size() == 2){
            erase(message, words.back());
        }else if(op == "dir" && words.size() == 2){
            dir(words.front(), words.back());
        }else if(op == "file" && words.size() >= 2){
            auto it = words.begin();
            string action = *it;
            it++;
            string fileName = *it;
            if(action == "up"){
                it++;
                string lastModStr = *it;
                int lastModInt = stoi(lastModStr);
                fileUp(fileName, lastModInt);
            }else if (action == "rm"){
                fileRemove(fileName);
            }
        }else if(op == "auth" && words.size() >= 3){
            auto it = words.begin();
            string userPassword = *it;
            it++;
            string userName = *it;
            it++;
            string remoteDir = *it;
            int transferPort = -1;
            if(words.size() > 3){
                it++;
                string transferPortStr = *it;
                transferPort = stoi(transferPortStr);
            }
            auth(userPassword, userName, remoteDir, transferPort);
        }else{
            error("SYNC-BOT", string("Invalid message to treat: ") + message);
        }
    }else{
        error("SYNC-BOT", "Cannot treat empty message");
    }
}

void SyncBot::login(string password){
    std::lock_guard<std::mutex> guard(loginMutex);
    //log("SYNC-BOT", string("Treating: login ") + password);
    hostPasswdTry = password;
}

void SyncBot::auth(string userPassword, string userName, string remoteSyncDir, int transferPort){
    //log("SYNC-BOT", string("Treating: auth ") + userPassword + string(" ") + remoteSyncDir);
    if(transferPort > 0){
        remoteScpPort = transferPort;
    }
    remotePasswd = userPassword;
    remoteDir.setDir(remoteSyncDir);
    remoteDirName = remoteSyncDir;
    remoteUserName = userName;
    authByRemote = true;
    //log("SYNC-BOT", "Treated auth and authorized");
}

void SyncBot::dir(string op, string dir){
    //log("SYNC-BOT", string("Adding directory ") + op + string(" ") + dir);
    if(op == "add"){
        remoteDir.addDir(dir);
    }else if(op == "rm"){
        remoteDir.rmDir(dir);
    }
}

void SyncBot::fileUp(string file, time_t lastMod){
    log("SYNC-BOT", string("Updating file to remote SyncDir: ") 
        + file + string(" ") + timeToChar(lastMod));
    FileInfo info;
    info.path = file;
    info.lastModification = lastMod;
    remoteDir.addFile(info);
}

void SyncBot::fileRemove(string file){
    log("SYNC-BOT", string("Removing file from remote SyncDir: ") + file);
    remoteDir.rmFile(file);
}

void SyncBot::remoteStartSync(){
    log("SYNC-BOT", string("Remote requires to start sync"));
    //remoteUpdating = true;
    while(localUpdating){
        //wait to finish update local
    }
    sendAllowSync();
}

void SyncBot::allowedToSync(){
    log("SYNC-BOT", string("Allowed to do sync"));
    syncAllowdByRemote = true;
}

void SyncBot::remoteEndSync(){
    log("SYNC-BOT", string("Remote ending sync"));
    remoteUpdating = false;
}

void SyncBot::mkdir(string message, string dir){
    log("SYNC-BOT", string("Making directory ") + dir);
    system(message.c_str());
}

void SyncBot::erase(string message, string obj){
    log("SYNC-BOT", string("Erasing ") + obj);
    system(message.c_str());
}