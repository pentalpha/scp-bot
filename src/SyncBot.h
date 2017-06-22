#ifndef _SYNC_BOT_
#define _SYNC_BOT_

#include <string>
#include <map>
#include <vector>
#include <thread>
#include <mutex>
#include "logging.h"
#include "Socket.h"
#include "Server.h"
#include "Client.h"

using namespace std;

/* All of the information relevant to a file */
struct FileInfo{
    //full path - sync dir
    string path;
    //name (including extension)
    string name;
    //should I even use this?
    //string extension;
    //What format to use for the time?
    //lastModification;
};

/* Friendly bot that helps synchronizing files and dirs*/
class SyncBot{
public:
    /*default mode: server, no passwd required
    dir = local dir to sync
    ip = hosting machine ipv4
    port = hosting machine port
    server = True if hosting, False if only sync
    passwd = hosting machine passwd, necessary if not a host*/
    SyncBot(string dir, string ip, int port, bool server = true, string passwd = "");

    //start sync process, blocking
    bool startSync();
protected:
    //returns pointer to Client or Server
    Socket* makeSocket(string ip, int port, bool server);

    //Tryes to get differences 
    void lookForDiffs();

    //Sync all info
    void syncInfo();
    //Sync only dir info
    void syncDirInfo();
    //Sync only file info
    void syncFileInfo();
    //Send message to delete file/dir on remote too
    bool sendDelete(string path, bool dir = false);
    //Send message to update file/dir on remote too
    bool sendUpdate(string path, bool dir = false);

    //Calls scp to send changes
    void sendChangesToRemote();
    //Notifies remote that a update is on the way
    bool sendStartingUpdate();
    //Notifies remote that the update is finished
    bool sendFinishedUpdate();
    
    bool isServer;
    //receiving update from remote / sending update to remote
    bool remoteUpdating, localUpdating;
    int port;
    SyncDir localDir, remoteDir;
    string localDirName, ip, remotePassWd;
    Socket *socket = NULL;
};

//Directory to sync. Can represent the data on the remote too
class SyncDir{
public:
    SyncDir(string dir, bool local = true);
    something diff(SyncDir* other);

private:
    void updateFilesAndDirs();
    void searchForNewFiles();
    void updateModTimes();
    void updateDirs();

    bool remote, updateFlag, finishFlag;
    map<string, FileInfo> files;
    vector<string> subDirs;

    mutex updateMutex;
};

#endif