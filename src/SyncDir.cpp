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
    updateMutex.lock();
    files[file.path] = file;
    updateMutex.unlock();
}

void SyncDir::rmFile(string filePath){
    updateMutex.lock();
    files.erase(filePath);
    updateMutex.unlock();
}
void SyncDir::addDir(string dir){
    updateMutex.lock();
    subDirs.insert(dir);
    updateMutex.unlock();
}
void SyncDir::rmDir(string dirPath){
    updateMutex.lock();
    subDirs.erase(dirPath);
    updateMutex.unlock();
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
    updateMutex.lock();
    updateDirs();
    searchForNewFiles();
    updateModTimes();
    updateMutex.unlock();
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
        }else{
            deletedFiles.erase(file.path);
        }
    }
    for(string deleted : deletedFiles){
        log("SYNC-DIR", string("Removed ") + deleted);
        files.erase(deleted);
    }

}

void SyncDir::updateModTimes(){
    for(pair<string, FileInfo> fileEntry : files){
        time_t lastMod = getLastModTime(fileEntry.first.c_str());
        if(lastMod > fileEntry.second.lastModification){
            log("SYNC-DIR", fileEntry.second.path + string(" has been altered: ")
            + timeToChar(lastMod) + string(" > ") + timeToChar(fileEntry.second.lastModification));
            files[fileEntry.first].lastModification = lastMod;
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
        }
    }
    vector<string> deletedDirs;
    for(string dir : subDirs){
        if(newDirs.count(dir) == 0){
            deletedDirs.push_back(dir);
            log("SYNC-DIR",string("Removed folder: ") + dir);
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