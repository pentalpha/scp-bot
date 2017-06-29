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

    //remoteUpdating = localUpdating = false;
    authByRemote = false;
    remoteSharedInitialInfo = sharedInitialInfo = false;
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
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
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
    }else if(state == SHARE){
        share();
    }else if(state == SYNC){
        sync();
    }
}

//////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////
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

void SyncBot::updateLocalDirIfNotBusy(){
    //localUpdating.lock();
    if(localDir.isUpdating() == false){
        localDir.updateFilesAndDirs(false);
    }
    //localUpdating.unlock();
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
        state = SHARE;
        log("SYNC-BOT", "SyncBot is now in SHARE");
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

void SyncBot::share(){
    log("SYNC-BOT", string("Trying to lock on share()"));
    syncLock.lock();
    log("SYNC-BOT", string("Locking on share()"));
    updateLocalDirIfNotBusy();
    if(localDir.hasChanges()){
        SyncChange change = localDir.nextChange();
        if(change.path != ""){
            log("SYNC-BOT", string("Going try share: ") 
                + change.path);
            sendStartSync();
            while(syncAllowState == WAIT){

            }
            if(syncAllowState == ALLOWED){
                sendChangeInfo(change);
                localDir.popNextChange();
            }
            syncAllowState = WAIT;
            sendEndSync();
        }
    }else if(!sharedInitialInfo){
        sendSharedAll();
        sharedInitialInfo = true;
    }
    if(sharedInitialInfo && remoteSharedInitialInfo){
        state = SYNC;
        log("SYNC-BOT", "SyncBot is now in SYNC");
    }
    log("SYNC-BOT", string("Unlocking on share()"));
    syncLock.unlock();
}

void SyncBot::sendChangeInfo(SyncChange change){
    if(change.isFile){
        if(change.isUp){
            time_t lastChange = localDir.getModTimeOfFile(change.path);
            sendFileAdd(change.path, lastChange);
        }else{
            sendFileRemove(change.path);
        }
    }else{
        if(change.isUp){
            sendDirAdd(change.path);
        }else{
            sendDirRemove(change.path);
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
    //localUpdating = true;
    syncAllowState = WAIT;
    socket->sendMsg("start-sync");
}

void SyncBot::sendAllowSync(){
    //remoteUpdating = true;
    socket->sendMsg("allow-sync");
}

void SyncBot::sendDenySync(){
    socket->sendMsg("deny-sync");
}

void SyncBot::sendEndSync(){
    //localUpdating = false;
    syncAllowState = WAIT;
    socket->sendMsg("end-sync");
}

void SyncBot::sendSharedAll(){
    socket->sendMsg("shared-all");
}

//////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////

void SyncBot::sync(){
    log("SYNC-BOT", string("Trying to lock on sync()"));
    syncLock.lock();
    log("SYNC-BOT", string("Locking on sync()"));
    updateLocalDirIfNotBusy();
    if(localDir.hasChanges()){
        SyncChange change = localDir.nextChange();
        if(change.path != ""){
            log("SYNC-BOT", string("Going try sync: ") 
                + change.path);
            sendStartSync();
            while(syncAllowState == WAIT){

            }
            if(syncAllowState == ALLOWED){
                sendChangeInfo(change);
                sendChange(change);
                //sendChangeMsg(change);
                localDir.popNextChange();
            }
            syncAllowState = WAIT;
            sendEndSync();
        }
    }
    log("SYNC-BOT", string("Unlocking on sync()"));
    syncLock.unlock();
}

void SyncBot::sendChange(SyncChange change){
    string obj = change.path;
    string noSyncDirObj = localDir.getFilePathWithoutSyncDir(obj);
    string remotePathObj = remoteDirName;
    if(remoteDirName[remoteDirName.length()-1] != '/'){
        remotePathObj += "/";
    }
    remotePathObj += noSyncDirObj;

    if(change.isFile){
        if(change.isUp){
            time_t lastMod = localDir.getModTimeOfFile(obj);
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
            if(remoteDir.hasFile(remotePathObj)){
                sendDeleteFile(remotePathObj);
            }
        }
    }else{
        if(change.isUp){
            if(!remoteDir.hasDir(remotePathObj)){
                sendMkdir(remotePathObj);
            }
        }else{
            if(remoteDir.hasDir(remotePathObj)){
                sendDeleteDir(remotePathObj);
            }
        }
    }
}

/*void SyncBot::sendChangeMsg(string change){
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
}*/

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
//////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////
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
        }else if(op == "allow-sync" && words.size() == 0){
            allowedToSync();
        }else if(op == "deny-sync" && words.size() == 0){
            denyedToSync();
        }else if(op == "shared-all" && words.size() == 0){
            remoteSharedAll();
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

//////////////////////////////////////////////////////////////////////////////////

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

void SyncBot::remoteSharedAll(){
    log("SYNC-BOT", string("Remote shared all initial information."));
    remoteSharedInitialInfo = true;
}

void SyncBot::remoteStartSync(){
    log("SYNC-BOT", string("Remote requires to start sync"));
    //if(localUpdating){
    //    sendDenySync();
    //}else{
    log("SYNC-BOT", string("Trying to lock on remoteStartSync()"));
    syncLock.lock();
    log("SYNC-BOT", string("Locking on remoteStartSync()"));
    sendAllowSync();
    //}
}

void SyncBot::allowedToSync(){
    log("SYNC-BOT", string("Allowed to do sync"));
    syncAllowState = ALLOWED;
}

void SyncBot::denyedToSync(){
    log("SYNC-BOT", string("Denyed to do sync"));
    syncAllowState = DENYED;
}

void SyncBot::remoteEndSync(){
    log("SYNC-BOT", string("Remote ending sync"));
    log("SYNC-BOT", string("Unlocking on remoteEndSync()"));
    syncLock.unlock();
    //remoteUpdating = false;
}

void SyncBot::mkdir(string message, string dir){
    log("SYNC-BOT", string("Making directory ") + dir);
    system(message.c_str());
}

void SyncBot::erase(string message, string obj){
    log("SYNC-BOT", string("Erasing ") + obj);
    system(message.c_str());
}