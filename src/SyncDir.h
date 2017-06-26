#ifndef _SYNC_DIR_
#define _SYNC_DIR_

#include <string>
#include <vector>
#include <set>
#include <map>
#include <thread>
#include <mutex>
#include <chrono>
#include "FileUtils.h"
#include "logging.h"

struct SyncDirDiff{
    vector<string> missingDirs, missingFiles;
    vector<string> newDirs, newFiles;
};

//Directory to sync. Can represent the data on the remote too
class SyncDir{
public:
    SyncDir(string dir, bool local = true);
    ~SyncDir();
    static SyncDirDiff diff(SyncDir* other);

    void addFile(FileInfo file);
    void rmFile(string filePath);
    void addDir(FileInfo dir);
    void rmDir(string dirPath);

    set<string> getFilesSet();

    void finish();

private:
    void updateCycle();
    void updateFilesAndDirs();
    void searchForNewFiles();
    void updateModTimes();
    void updateDirs();

    string directory;
    bool remote, updateFlag, finishFlag;
    map<string, FileInfo> files;
    map<string, FileInfo> subDirs;

    mutex updateMutex;
    thread* updatingThread = NULL;
};

#endif