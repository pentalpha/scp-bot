#include "../../include/SyncBot.h"

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
        }else if(op == "shared-all-info" && words.size() == 0){
            remoteSharedAllInfo();
        }else if(op == "shared-all-changes" && words.size() == 0){
            remoteSharedAllChanges();
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
    log(3,"SYNC-BOT", string("Treating: login ") + password);
    hostPasswdTry = password;
}

void SyncBot::auth(string userPassword, string userName, string remoteSyncDir, int transferPort){
    log(3,"SYNC-BOT", string("Treating: auth ") + userPassword + string(" ") + remoteSyncDir);
    if(transferPort > 0){
        remoteScpPort = transferPort;
    }
    remotePasswd = userPassword;
    remoteDir.setDir(remoteSyncDir);
    remoteDirName = remoteSyncDir;
    remoteUserName = userName;
    authByRemote = true;
    log(3,"SYNC-BOT", "Treated auth and authorized");
}

void SyncBot::dir(string op, string dir){
    log(3,"SYNC-BOT", string("Treating: Adding directory ") + op + string(" ") + dir);
    if(op == "add"){
        remoteDir.addDir(dir);
    }else if(op == "rm"){
        remoteDir.rmDir(dir);
    }
}

void SyncBot::fileUp(string file, time_t lastMod){
    log(4,"SYNC-BOT", string("Added file from remote: ") 
        + file + string(" ") + timeToChar(lastMod));
    FileInfo info;
    info.path = file;
    info.lastModification = lastMod;
    remoteDir.addFile(info);
}

void SyncBot::fileRemove(string file){
    log(4,"SYNC-BOT", string("Removing file from remote SyncDir: ") + file);
    remoteDir.rmFile(file);
}

void SyncBot::remoteSharedAllInfo(){
    log(4,"SYNC-BOT", string("Remote shared all information."));
    remoteSharedAllInfoFlag = true;
}

void SyncBot::remoteSharedAllChanges(){
    log(4,"SYNC-BOT", string("Remote shared all files."));
    remoteSharedAllChangesFlag = true;
}

void SyncBot::remoteStartSync(){
    log(4,"SYNC-BOT", string("Remote requires to start sync"));
    //if(localUpdating){
    //    sendDenySync();
    //}else{
    log(1,"SYNC-BOT", string("Trying to lock on remoteStartSync()"));
    syncLock.lock();
    log(1,"SYNC-BOT", string("Locking on remoteStartSync()"));
    sendAllowSync();
    //}
}

void SyncBot::allowedToSync(){
    log(4,"SYNC-BOT", string("Allowed to do sync"));
    syncAllowState = ALLOWED;
}

void SyncBot::denyedToSync(){
    log(4,"SYNC-BOT", string("Denyed to do synchronization"));
    syncAllowState = DENYED;
}

void SyncBot::remoteEndSync(){
    log(4,"SYNC-BOT", string("Remote ending sync"));
    log(1,"SYNC-BOT", string("Unlocking on remoteEndSync()"));
    syncLock.unlock();
    //remoteUpdating = false;
}

void SyncBot::mkdir(string message, string dir){
    log(8,"SYNC-BOT", string("Making directory ") + dir);
    system(message.c_str());
}

void SyncBot::erase(string message, string obj){
    log(8,"SYNC-BOT", string("Erasing ") + obj);
    system(message.c_str());
}