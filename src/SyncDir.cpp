#include "SyncDir.h"

SyncDir::SyncDir(){
    bool local = false;
    updateFlag = local;
    finishFlag = false;
    updatingFlag = false;
    remote = !local;
    directory = "";
}

void SyncDir::setDir(string dir){
    if(directory == ""){
        directory = dir;
    }
}

SyncDir::SyncDir(string dir, bool local){
    updateFlag = local;
    finishFlag = false;
    updatingFlag = false;
    remote = !local;
    directory = dir;
    /*if(!remote){
        //updateFilesAndDirs();
        updatingThread = new thread(&SyncDir::updateCycle, this);
        updatingThread->detach();
    }*/
}

SyncDir::~SyncDir(){
    if(updateFlag || updatingFlag || remote){
        finishFlag = true;
        updatingThread->join();
        delete updatingThread;
    }
}

//////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////

void SyncDir::addFile(FileInfo file){
    std::lock_guard<std::mutex> guard(updateMutex);
    files[file.path] = file;
    log("SYNC-DIR", string("Added ") + file.path);
    if(!remote){
        upFileMsg(file.path);
    }
}
void SyncDir::modFile(string filePath, time_t newModTime){
    std::lock_guard<std::mutex> guard(updateMutex);
    time_t lastMod = files[filePath].lastModification;
    files[filePath].lastModification = newModTime;
    log("SYNC-DIR", filePath + string(" has been altered: ")
    + timeToChar(newModTime) + string(" > ") + timeToChar(lastMod));
    if(!remote){
        upFileMsg(filePath);
    }
}
void SyncDir::rmFile(string filePath){
    std::lock_guard<std::mutex> guard(updateMutex);
    files.erase(filePath);
    log("SYNC-DIR", string("Removed ") + filePath);
    if(!remote){
        rmFileMsg(filePath);
    }
}

void SyncDir::addDir(string dir){
    std::lock_guard<std::mutex> guard(updateMutex);
    subDirs.insert(dir);
    log("SYNC-DIR", string("New folder: ") + dir);
    if(!remote){
        upDirMsg(dir);
    }
}
void SyncDir::rmDir(string dirPath){
    std::lock_guard<std::mutex> guard(updateMutex);
    subDirs.erase(dirPath);
    log("SYNC-DIR",string("Removed folder: ") + dirPath);
    if(!remote){
        rmDirMsg(dirPath);
    }
}

//////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////

void SyncDir::updateFilesAndDirs(bool delay){
    updatingFlag = true;
    if(delay){
        std::this_thread::sleep_for(std::chrono::milliseconds(autoUpdateDelayMS));
    }
    //std::lock_guard<std::mutex> guard(updateMutex);
    updateDirs();
    searchForNewFiles();
    updateModTimes();
    updatingFlag = false;
}

unordered_set<string> SyncDir::getFilesSet(){
    unordered_set<string> keys;
    for(pair<string, FileInfo> fileEntry : files){
        keys.insert(fileEntry.first);
    }
    return keys;
}

void SyncDir::searchForNewFiles(){
    vector<tinydir_file> scanned = getSubFiles(directory);
    unordered_set<string> deletedFiles = getFilesSet();
    for(tinydir_file file : scanned){
        if(files.find(file.path) == files.end()){
            FileInfo info = getFileInfo(file);
            addFile(info);
        }else{
            deletedFiles.erase(file.path);
        }
    }
    for(string deleted : deletedFiles){
        rmFile(deleted);
    }

}

void SyncDir::updateModTimes(){
    for(pair<string, FileInfo> fileEntry : files){
        time_t lastMod = getLastModTime(fileEntry.first.c_str());
        if(lastMod > fileEntry.second.lastModification){
            modFile(fileEntry.first, lastMod);
        }else{
            //log("SYNC-DIR", fileEntry.second.path + string(" remains the same."));
        }
    }
}

void SyncDir::updateDirs(){
    unordered_set<string> newDirs = getDirs(directory);
    for(string dir : newDirs){
        if(subDirs.count(dir) == 0){
            addDir(dir);
        }
    }
    vector<string> deletedDirs;
    for(string dir : subDirs){
        if(newDirs.count(dir) == 0){
            deletedDirs.push_back(dir);
        }
    }
    for(string deletedDir : deletedDirs){
        rmDir(deletedDir);
    }
}

SyncDirDiff SyncDir::diff(SyncDir* other){
    //TODO
}

//////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////

void SyncDir::finish(){
    finishFlag = true;
}

//////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////

bool SyncDir::searchAndEraseElementInListContaining(list<string> &items, string s){
    bool erased = false;
    auto i = items.begin();
    while (i != items.end())
    {
        string value = *i;
        bool isSubStr = (value.find(s) != string::npos);
        if (isSubStr){
            i = items.erase(i);
            break;
        }else{
            ++i;
        }
    }
    return erased;
}

void SyncDir::putChangeMessage(string type, string file, bool fileMsg){
    std::lock_guard<std::mutex> guard(addMsgMutex);
    string msg = type + string(" ") + file;
    if(fileMsg){
        searchAndEraseElementInListContaining(fileChanges, string("-file ") + file);
        fileChanges.push_back(msg);
    }else{
        searchAndEraseElementInListContaining(dirChanges, string("-dir ") + file);
        dirChanges.push_back(msg);
    }
}

vector<string> SyncDir::popChanges(){
    std::lock_guard<std::mutex> guard(addMsgMutex);
    vector<string> changes;
    auto i = dirChanges.begin();
    while(i != dirChanges.end()){
        string value = *i;
        changes.push_back(value);
        i++;
    }
    auto j = fileChanges.begin();
    while(j != fileChanges.end()){
        string value = *j;
        changes.push_back(value);
        j++;
    }

    dirChanges.clear();
    fileChanges.clear();

    return changes;
}

//////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////

bool SyncDir::isUpdating(){
    return updatingFlag;
}

void SyncDir::stopUpdating(){
    updateFlag = false;
    while(updatingFlag){

    }
}

void SyncDir::resumeUpdating(){
    updateFlag = true;
}

void SyncDir::updateCycle(){
    while(!finishFlag){
        if(updateFlag){
            //updatingFlag = true;
            updateFilesAndDirs();
            //updatingFlag = false;
            std::this_thread::sleep_for(std::chrono::milliseconds(autoUpdateDelayMS));
        }
    }
}