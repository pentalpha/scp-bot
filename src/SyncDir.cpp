#include "SyncDir.h"

/*struct SyncDirDiff{
    vector<string> missingDirs, missingFiles;
    vector<string> newDirs, newFiles;
}

//Directory to sync. Can represent the data on the remote too
class SyncDir{
public:
    SyncDir(string dir, bool local = true);
    static SyncDirDiff diff(SyncDir* other);

private:
    void updateFilesAndDirs();
    void searchForNewFiles();
    void updateModTimes();
    void updateDirs();

    string directory;
    bool remote, updateFlag, finishFlag;
    map<string, FileInfo> files;
    map<string, FileInfo> subDirs;

    mutex updateMutex;
    thread* updatingThread;
};*/

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
void SyncDir::addDir(FileInfo dir){
    updateMutex.lock();
    subDirs[dir.path] = dir;
    updateMutex.unlock();
}
void SyncDir::rmDir(string dirPath){
    updateMutex.lock();
    subDirs.erase(dirPath);
    updateMutex.unlock();
}

void SyncDir::updateCycle(){
    while(updateFlag && !finishFlag){
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
        updateFilesAndDirs();
    }
}

void SyncDir::updateFilesAndDirs(){
    updateMutex.lock();
    updateDirs();
    searchForNewFiles();
    updateModTimes();
    updateMutex.unlock();
}

set<string> SyncDir::getFilesSet(){
    set<string> keys;
    for(pair<string, FileInfo> fileEntry : files){
        keys.insert(fileEntry.first);
    }
    return keys;
}

void SyncDir::searchForNewFiles(){
    vector<tinydir_file> scanned = getSubFiles(false,
        directory);
    set<string> deletedFiles = getFilesSet();
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
        fileEntry.second.lastModification = lastMod;
    }
}

void SyncDir::updateDirs(){
    vector<FileInfo> dirInfos = getFileInfoFromDir(true,
        directory);
    for(FileInfo info : dirInfos){
        subDirs[info.path] = info;
    }
}

SyncDirDiff SyncDir::diff(SyncDir* other){
    //TODO
}

void SyncDir::finish(){
    finishFlag = true;
}