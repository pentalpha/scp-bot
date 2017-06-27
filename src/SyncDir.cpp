#include "SyncDir.h"

SyncDir::SyncDir(string dir, bool local){
    updateFlag = local;
    finishFlag = false;
    remote = !local;
    directory = dir;
    if(updateFlag){
        updateFilesAndDirs();
        updatingThread = new thread(&SyncDir::updateCycle, this);
        updatingThread->detach();
    }
}

SyncDir::~SyncDir(){
    if(updateFlag || remote){
        finishFlag = true;
        updatingThread->join();
        delete updatingThread;
    }
}

void SyncDir::addFile(FileInfo file){
    std::lock_guard<std::mutex> guard(updateMutex);
    files[file.path] = file;
}

void SyncDir::rmFile(string filePath){
    std::lock_guard<std::mutex> guard(updateMutex);
    files.erase(filePath);
}
void SyncDir::addDir(string dir){
    std::lock_guard<std::mutex> guard(updateMutex);
    subDirs.insert(dir);
}
void SyncDir::rmDir(string dirPath){
    std::lock_guard<std::mutex> guard(updateMutex);
    subDirs.erase(dirPath);
}

void SyncDir::updateCycle(){
    while(!finishFlag){
        if(updateFlag){
            std::this_thread::sleep_for(std::chrono::milliseconds(1000));
            updateFilesAndDirs();
        }
    }
}

void SyncDir::updateFilesAndDirs(){
    std::lock_guard<std::mutex> guard(updateMutex);
    updateDirs();
    searchForNewFiles();
    updateModTimes();
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
            files[file.path] = info;
            log("SYNC-DIR", string("Added ") + file.path);
            upFileMsg(file.path);
        }else{
            deletedFiles.erase(file.path);
        }
    }
    for(string deleted : deletedFiles){
        log("SYNC-DIR", string("Removed ") + deleted);
        files.erase(deleted);
        rmFileMsg(deleted);
    }

}

void SyncDir::updateModTimes(){
    for(pair<string, FileInfo> fileEntry : files){
        time_t lastMod = getLastModTime(fileEntry.first.c_str());
        if(lastMod > fileEntry.second.lastModification){
            log("SYNC-DIR", fileEntry.second.path + string(" has been altered: ")
            + timeToChar(lastMod) + string(" > ") + timeToChar(fileEntry.second.lastModification));
            files[fileEntry.first].lastModification = lastMod;
            upFileMsg(fileEntry.first);
        }else{
            //log("SYNC-DIR", fileEntry.second.path + string(" remains the same."));
        }
    }
}

void SyncDir::updateDirs(){
    unordered_set<string> newDirs = getDirs(directory);
    for(string dir : newDirs){
        if(subDirs.count(dir) == 0){
            subDirs.insert(dir);
            log("SYNC-DIR",string("New folder: ") + dir);
            upDirMsg(dir);
        }
    }
    vector<string> deletedDirs;
    for(string dir : subDirs){
        if(newDirs.count(dir) == 0){
            deletedDirs.push_back(dir);
            log("SYNC-DIR",string("Removed folder: ") + dir);
            rmDirMsg(dir);
        }
    }
    for(string deletedFile : deletedDirs){
        subDirs.erase(deletedFile);
    }
}

SyncDirDiff SyncDir::diff(SyncDir* other){
    //TODO
}

void SyncDir::finish(){
    finishFlag = true;
}

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