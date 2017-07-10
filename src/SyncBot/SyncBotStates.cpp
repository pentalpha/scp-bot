#include "../../include/SyncBot.h"

void SyncBot::sleeping(){
    //cout << "asleep\n";
    if(!(socket->isAsleep())){
        state = WAITING;
        log(7,"SYNC-BOT", "SyncBot is now WAITING for a new connection");
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
        state = AUTH;
        
        authState = NOT_STARTED;
        if(isServer){
            remoteAddress = ((Server*)socket)->clientAddress;
            
        }
        log(1, "SYNC-BOT", string("Remote address is ") + remoteAddress);
        log(6, "SYNC-BOT", string("SyncBot is now authenticating with ") + remoteAddress);
    }else{
        updateLocalDirIfNotBusy();
    }
}

//////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////

void SyncBot::authentication(){
    if (authState == NOT_STARTED){
        if(isServer && someoneTryedToLogin()){
            //log(3,"SYNC-BOT", "Someone tryed to login on server");
            if(correctServerPassword()){
                log(4, "SYNC-BOT-HOST", "Sync client logged in to server");
                bool sent = sendAuthMessage();
                if(sent){
                    authState = WAITING_REMOTE_AUTH;
                    log(4, "SYNC-BOT-HOST", "SyncBot is now WAITING_REMOTE_AUTH");
                }
            }else{
                log(4, "SYNC-BOT-HOST", "Sync client tryed to login with incorrect password");
            }
        }else if(!isServer){
            bool sent = sendLoginMessage();
            if(sent){
                authState = WAITING_REMOTE_AUTH;
                log(4, "SYNC-BOT-CLIENT", "SyncBot is now WAITING_REMOTE_AUTH");
            }
        }
    }else if (authState == WAITING_REMOTE_AUTH){
        if(hasRemoteAuthorization()){
            if(!isServer){
                bool sent = sendAuthMessage();
                if(sent){
                    authState = AUTHORIZED;
                    log(4, "SYNC-BOT-CLIENT", "SyncBot is now AUTHORIZED");
                }
            }else{
                authState = AUTHORIZED;
                log(4, "SYNC-BOT-HOST", "SyncBot is now AUTHORIZED");
            }
        }
    }else if(authState == AUTHORIZED){
        state = INFO_SHARE;
        log(7, "SYNC-BOT", "SyncBot is now in Information Sharing Mode");
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
    log(2, "SYNC-BOT", string("Trying to send login message: ") + msg);
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
    log(2, "SYNC-BOT", string("Trying to send auth message: ") + msg);
    return socket->sendMsg(msg);
}

bool SyncBot::hasRemoteAuthorization(){
    return authByRemote;
}

//////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////

void SyncBot::infoShare(){
    log(1,"SYNC-BOT", string("Trying to lock on infoShare()"));
    syncLock.lock();
    log(1,"SYNC-BOT", string("Locking on infoShare()"));
    updateLocalDirIfNotBusy();

    if(sharedAllInfoFlag && remoteSharedAllInfoFlag){
        sharedAllInfoFlag = remoteSharedAllInfoFlag = false;
        onLocalButNotInRemote = localDir.diff(remoteDir);
        state = CHANGE_SHARE;
        log(7, "SYNC-BOT", "SyncBot is now in Change Sharing Mode");
        printChangesToSend();
    }else if(localDir.hasChanges()){
        SyncChange change = localDir.nextChange();
        if(change.path != ""){
            log(3,"SYNC-BOT", string("Going try infoShare: ") 
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
    }else if(!sharedAllInfoFlag){
        sharedAllInfoFlag = true;
        sendSharedAllInfo();
    }
    
    log(1,"SYNC-BOT", string("Unlocking on infoShare()"));
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

void SyncBot::sendSharedAllInfo(){
    socket->sendMsg("shared-all-info");
}

void SyncBot::printChangesToSend(){
    log(3, "SYNC-BOT", "Changes present locally but not on the remote: ");
    for(int i = 0; i < onLocalButNotInRemote.size(); i++){
        log(3, "SYNC-BOT", onLocalButNotInRemote[i].toString());
    }
}

//////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////

void SyncBot::changeShare(){
    log(1,"SYNC-BOT", string("Trying to lock on changeShare()"));
    syncLock.lock();
    log(1,"SYNC-BOT", string("Locking on changeShare()"));
        
    if(sharedAllChangesFlag && remoteSharedAllChangesFlag){
        sharedAllChangesFlag = remoteSharedAllChangesFlag = false;
        state = REALTIME_SYNC;
        log(7, "SYNC-BOT", "SyncBot is now in Realtime Synchronization Mode");

    }else if(onLocalButNotInRemote.size() > 0){
        SyncChange change = onLocalButNotInRemote.front();
        if(change.path != ""){
            log(3, "SYNC-BOT", string("Going try fileShare: ") 
                + change.path);
            sendStartSync();
            while(syncAllowState == WAIT){

            }

            if(syncAllowState == ALLOWED){
                sendChange(change);
                onLocalButNotInRemote.pop_front();
            }
            syncAllowState = WAIT;
            sendEndSync();
        }else{
            onLocalButNotInRemote.pop_front();
        }

    }else if(sharedAllChangesFlag == false){
        sharedAllChangesFlag = true;
        sendSharedAllChanges();
    }

    log(1,"SYNC-BOT", string("Unlocking on changeShare()"));
    syncLock.unlock();
}

void SyncBot::sendSharedAllChanges(){
    socket->sendMsg("shared-all-changes");
}

void SyncBot::sendChange(SyncChange change){
    string obj = change.path;
    string noSyncDirObj = localDir.absolutePathToRelative(obj);
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
                if(lastMod > remoteLastMod + SyncDir::changeTimeTolerance){
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
    //log(1,"SYNC-BOT", string("Transfering with: ") + cmd);
    log(8, "SYNC-BOT", string("Starting file trasfer of ") + localFile);
    system(cmd.c_str());
    log(8, "SYNC-BOT", string("Finished file trasfer of ") + localFile);
}

void SyncBot::sendDeleteFile(string file){
    socket->sendMsg(string("rm -f ") + file);
}

//////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////

void SyncBot::sync(){
    log(1,"SYNC-BOT", string("Trying to lock on sync()"));
    syncLock.lock();
    log(1,"SYNC-BOT", string("Locking on sync()"));
    updateLocalDirIfNotBusy();
    if(localDir.hasChanges()){
        SyncChange change = localDir.nextChange();
        if(change.path != ""){
            log(3, "SYNC-BOT", string("Going try sync: ") 
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
    log(1,"SYNC-BOT", string("Unlocking on sync()"));
    syncLock.unlock();
}