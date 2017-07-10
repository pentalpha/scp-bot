#include "../../include/SyncDir.h"

const int SyncDir::autoUpdateDelayMS = 500;
const int SyncDir::changeTimeTolerance = 2;

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

string SyncDir::absolutePathToRelative(string filePath)
{
    if(filePath.find(directory) != string::npos){
        string subPath = filePath.substr(directory.length(),
                                        filePath.length());
        if(subPath[0] == '/'){
            subPath = subPath.substr(1, subPath.length());
            return subPath;
        }
    }
    return "";
}

string SyncDir::relativePathToAbsolute(string filePath){
    string rPath = directory;
    if(directory[directory.length()-1] == '/'){
        rPath = rPath.substr(0, rPath.length()-1);
    }
    if(filePath[0] == '/'){
        filePath = filePath.substr(1, filePath.length());
    }
    rPath += '/';
    rPath += filePath;
    return rPath;
}

bool SyncDir::hasDir(string dir){
    if(dir.find(directory) != string::npos){
        return (subDirs.find(dir) != subDirs.end());
    }else{
        string rDir = relativePathToAbsolute(dir);
        return (subDirs.find(rDir) != subDirs.end());
    }
}

bool SyncDir::hasFile(string file){
    if((file.find(directory) != string::npos)){
        return (files.find(file) != files.end());
    }else{
        string rFile = relativePathToAbsolute(file);
        return (files.find(rFile) != files.end());
    }
}

time_t SyncDir::getModTimeOfFile(string filePath){
    if(files.find(filePath) != files.end()){
        return files[filePath].lastModification;
    }else{
        filePath = relativePathToAbsolute(filePath);
        if(files.find(filePath) != files.end()){
            return files[filePath].lastModification;
        }else{
            return 0;
        }
    }
}


//////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////

string SyncDir::getNoticeToLogIfRemote(){
    string remoteNotice = " ";
    if(remote){
        remoteNotice = " (remote) ";
    }
    return remoteNotice;
}

void SyncDir::addFile(FileInfo file){
    std::lock_guard<std::mutex> guard(updateMutex);
    files[file.path] = file;
    log(5,"SYNC-DIR", string("Added") + getNoticeToLogIfRemote() + file.path
        + string(" ") + timeToChar(file.lastModification));
    if(!remote){
        upFileMsg(file.path);
    }
}
void SyncDir::modFile(string filePath, time_t newModTime){
    std::lock_guard<std::mutex> guard(updateMutex);
    time_t lastMod = files[filePath].lastModification;
    files[filePath].lastModification = newModTime;
    log(5,"SYNC-DIR", filePath + getNoticeToLogIfRemote() + string("has been modified: ")
    + timeToChar(newModTime) + string(" > ") + timeToChar(lastMod));
    if(!remote){
        upFileMsg(filePath);
    }
}
void SyncDir::rmFile(string filePath){
    std::lock_guard<std::mutex> guard(updateMutex);
    files.erase(filePath);
    log(5,"SYNC-DIR", string("Removed") + getNoticeToLogIfRemote() + filePath);
    if(!remote){
        rmFileMsg(filePath);
    }
}

void SyncDir::addDir(string dir){
    std::lock_guard<std::mutex> guard(updateMutex);
    subDirs.insert(dir);
    log(5,"SYNC-DIR", string("New folder:") + getNoticeToLogIfRemote() + dir);
    if(!remote){
        upDirMsg(dir);
    }
}
void SyncDir::rmDir(string dirPath){
    std::lock_guard<std::mutex> guard(updateMutex);
    subDirs.erase(dirPath);
    log(5,"SYNC-DIR",string("Removed folder:") + getNoticeToLogIfRemote() + dirPath);
    if(!remote){
        rmDirMsg(dirPath);
    }
}

//////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////

void SyncDir::updateFilesAndDirs(bool delay){
    updatingFlag = true;
    if(delay){
        std::this_thread::sleep_for(std::chrono::milliseconds(SyncDir::autoUpdateDelayMS));
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
            if(info.path.find(SyncArgs::configFileName) == string::npos){
                addFile(info);
            }
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
        if(lastMod > fileEntry.second.lastModification + changeTimeTolerance){
            modFile(fileEntry.first, lastMod);
        }else{
            log(0, "SYNC-DIR", fileEntry.second.path + string(" remains the same."));
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

//////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////

void SyncDir::finish(){
    finishFlag = true;
}

//////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////

void SyncDir::eraseChangesWhichInvolve(list<SyncChange> items, string path){
    auto i = items.begin();
    while (i != items.end())
    {
        SyncChange value = *i;
        if (path == value.path){
            i = items.erase(i);
        }else{
            ++i;
        }
    }
}

/*bool SyncDir::searchAndEraseElementInListContaining(list<string> &items, string s){
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
}*/

void SyncDir::putChangeMessage(SyncChange change){
    std::lock_guard<std::mutex> guard(addMsgMutex);
    if(change.isFile){
        eraseChangesWhichInvolve(fileChanges, change.path);
        //searchAndEraseElementInListContaining(fileChanges, string("-file ") + file);
        fileChanges.push_back(change);
    }else{
        eraseChangesWhichInvolve(dirChanges, change.path);
        //searchAndEraseElementInListContaining(dirChanges, string("-dir ") + file);
        dirChanges.push_back(change);
    }
}

vector<SyncChange> SyncDir::popChanges(){
    std::lock_guard<std::mutex> guard(addMsgMutex);
    vector<SyncChange> changes;
    auto i = dirChanges.begin();
    while(i != dirChanges.end()){
        SyncChange value = *i;
        changes.push_back(value);
        i++;
    }
    auto j = fileChanges.begin();
    while(j != fileChanges.end()){
        SyncChange value = *j;
        changes.push_back(value);
        j++;
    }

    dirChanges.clear();
    fileChanges.clear();

    return changes;
}

SyncChange SyncDir::nextChange(){
    std::lock_guard<std::mutex> guard(addMsgMutex);

    if(dirChanges.size() > 0){
        return *dirChanges.begin();
    }else if(fileChanges.size() > 0){
        return *fileChanges.begin();
    }else{
        SyncChange c;
        c.isUp = false;
        c.isFile = false;
        c.path = "";
        return c;
    }
}

void SyncDir::popNextChange(){
    std::lock_guard<std::mutex> guard(addMsgMutex);

    if(dirChanges.size() > 0){
        dirChanges.pop_front();
    }else if(fileChanges.size() > 0){
        fileChanges.pop_front();
    }
}

bool SyncDir::hasChanges(){
    return ((dirChanges.size() + fileChanges.size()) > 0);
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
            std::this_thread::sleep_for(std::chrono::milliseconds(SyncDir::autoUpdateDelayMS));
        }
    }
}

SyncChange SyncDir::getUpFileChange(string path){
    SyncChange c;
    c.isUp = true;
    c.isFile = true;
    c.path = path;
    return c;
}

SyncChange SyncDir::getRmFileChange(string path){
    SyncChange c;
    c.isUp = false;
    c.isFile = true;
    c.path = path;
    return c;
}

SyncChange SyncDir::getUpDirChange(string path){
    SyncChange c;
    c.isUp = true;
    c.isFile = false;
    c.path = path;
    return c;
}

SyncChange SyncDir::getRmDirChange(string path){
    SyncChange c;
    c.isUp = false;
    c.isFile = false;
    c.path = path;
    return c;
}


deque<SyncChange> SyncDir::diff(SyncDir &other){
    deque<SyncChange> changes;
    for(string dir : subDirs){
        string relativePath = absolutePathToRelative(dir);
        if(other.hasDir(relativePath) == false){
            changes.push_back(getUpDirChange(dir));
        }
    }

    for(auto entry : files){
        bool addThisFile = false;
        string relativePath = absolutePathToRelative(entry.first);
        if(other.hasFile(relativePath)){
            time_t otherFileModTime = other.getModTimeOfFile(relativePath);
            if(otherFileModTime + changeTimeTolerance < entry.second.lastModification){
                addThisFile = true;
            }
        }else{
            addThisFile = true;
            log(3, "SYNC-DIR", string("Present locally but not on the remote: ") + relativePath);
        }

        if(addThisFile){
            changes.push_back(getUpFileChange(entry.first));
        }
    }

    return changes;
}