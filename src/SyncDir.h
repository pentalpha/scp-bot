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

struct SyncDirDiff{
    vector<string> missingDirs, missingFiles;
    vector<string> newDirs, newFiles;
};

//Directory to sync. Can represent the data on the remote too
class SyncDir{
public:
    const int autoUpdateDelayMS = 500;
    SyncDir(string dir, bool local = true);
    ~SyncDir();
    static SyncDirDiff diff(SyncDir* other);

    void addFile(FileInfo file);
    void rmFile(string filePath);
    void addDir(string dir);
    void rmDir(string dirPath);

    unordered_set<string> getFilesSet();

    bool isUpdating();
    void stopUpdating();
    void resumeUpdating();
    void finish();

    vector<string> popChanges();
private:
    void updateCycle();
    void updateFilesAndDirs();
    void searchForNewFiles();
    void updateModTimes();
    void updateDirs();

    string directory;
    bool remote, updateFlag, finishFlag, updatingFlag;
    map<string, FileInfo> files;
    unordered_set<string> subDirs;

    mutex updateMutex, addMsgMutex;
    thread* updatingThread = NULL;

    inline void upFileMsg(string file){
        putChangeMessage("up-file", file, true);
    }
    inline void rmFileMsg(string file){
        putChangeMessage("rm-file", file, true);
    }
    inline void upDirMsg(string dir){
        putChangeMessage("up-dir", dir, false);
    }
    inline void rmDirMsg(string dir){
        putChangeMessage("rm-dir", dir, false);
    }
    static bool searchAndEraseElementInListContaining(list<string> &items, string s);
    void putChangeMessage(string type, string file, bool fileMsg);
    
    list<string> fileChanges, dirChanges;
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