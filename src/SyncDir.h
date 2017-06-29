#ifndef _SYNC_DIR_
#define _SYNC_DIR_

#include <string>
#include <vector>
#include <unordered_set>
#include <map>
#include <thread>
#include <mutex>
#include <chrono>
#include <queue>
#include <list>
#include "FileUtils.h"
#include "logging.h"
#include "OctoSyncArgs.h"

struct SyncDirDiff{
    vector<string> missingDirs, missingFiles;
    vector<string> newDirs, newFiles;
};

struct SyncChange{
    bool isUp, isFile;
    string path;
};



//Directory to sync. Can represent the data on the remote too
class SyncDir{
public:
    const int autoUpdateDelayMS = 500;
    SyncDir(string dir, bool local = true);
    SyncDir();
    void setDir(string dir);
    ~SyncDir();
    static SyncDirDiff diff(SyncDir* other);
    void updateFilesAndDirs(bool delay = false);
    void addFile(FileInfo file);
    void modFile(string filePath, time_t newModTime);
    void rmFile(string filePath);
    void addDir(string dir);
    void rmDir(string dirPath);

    unordered_set<string> getFilesSet();

    bool isUpdating();
    void stopUpdating();
    void resumeUpdating();
    void finish();

    vector<SyncChange> popChanges();
    SyncChange nextChange();
    void popNextChange();
    bool hasChanges();

    string getFilePathWithoutSyncDir(string filePath);
    
    bool hasDir(string dir);
    bool hasFile(string file);
    time_t getModTimeOfFile(string filePath);
private:
    void updateCycle();
    
    void searchForNewFiles();
    void updateModTimes();
    void updateDirs();

    string directory;
    bool remote, updateFlag, finishFlag, updatingFlag;
    map<string, FileInfo> files;
    unordered_set<string> subDirs;

    mutex updateMutex, addMsgMutex;
    thread* updatingThread = NULL;

    static SyncChange getUpFileChange(string path);
    static SyncChange getRmFileChange(string path);
    static SyncChange getUpDirChange(string path);
    static SyncChange getRmDirChange(string path);

    void putChangeMessage(SyncChange change);

    inline void upFileMsg(string file){
        putChangeMessage(getUpFileChange(file));
    }
    inline void rmFileMsg(string file){
        putChangeMessage(getRmFileChange(file));
    }
    inline void upDirMsg(string dir){
        putChangeMessage(getUpDirChange(dir));
    }
    inline void rmDirMsg(string dir){
        putChangeMessage(getRmDirChange(dir));
    }
    //static bool searchAndEraseElementInListContaining(list<string> &items, string s);
    void eraseChangesWhichInvolve(list<SyncChange> items, string path);
    
    list<SyncChange> fileChanges, dirChanges;

    string getNoticeToLogIfRemote();
};

/*
Change messages catalogue:
up-file [file-path]:
    file created or modified
rm-file [file-path]:
    file removed
up-dir [dir-path]:
    dir created
rm-dir [dir-path]:
    dir removed
*/

#endif