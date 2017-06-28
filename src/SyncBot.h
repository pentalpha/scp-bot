#ifndef _SYNC_BOT_
#define _SYNC_BOT_

#include <string>
#include <map>
#include <vector>
#include <thread>
#include <mutex>
#include <sstream>
#include <unistd.h>
#include <stdio.h>
#include "logging.h"
#include "Socket.h"
#include "Server.h"
#include "Client.h"
#include "FileUtils.h"
#include "SyncDir.h"
#include "OctoSyncArgs.h"

using namespace std;

enum SyncBotState{
    SLEEP = 0,
    WAITING = 1,
    AUTH = 2,
    SYNC = 3
};

enum AuthState{
    NOT_STARTED = 0,
    WAITING_REMOTE_AUTH = 1,
    AUTHORIZED = 2
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
    //SyncBot(string dir, string ip, int port, bool server = true, string passwd = "");

    //Start SyncBot using parsed command line args
    SyncBot(OctoSyncArgs args);

    //start sync process, blocking
    bool run();
protected:
    //returns pointer to Client or Server
    static Socket* makeSocket(string ip, int port, bool server);
    static string getUserName();
    static list<string> splitStrInList(string s);

    //Tryes to get differences 
    //void lookForDiffs();
    void updateCycle();
    void update();

    void sleeping();

    void waiting();

    void authentication();
    bool someoneTryedToLogin();
    bool correctServerPassword();
    void setHostPasswdTry(string newTry);
    bool sendLoginMessage();
    bool sendAuthMessage();
    bool hasRemoteAuthorization();

    void sync();
    void updateLocalDirIfNotBusy();
    void sendChangeMsg(string change);
    void sendDirRemove(string dir);
    void sendDirAdd(string dir);
    void sendFileRemove(string file);
    void sendFileAdd(string file, time_t lastMod);
    void sendStartSync();
    void sendEndSync();
    void sendDeleteFile(string file);
    void sendFile(string localFile, string remoteFile);
    void sendDeleteDir(string dir);
    void sendMkdir(string dir);
    
    /*//Sync all info
    void syncInfo();
    //Sync only dir info
    void syncDirInfo();
    //Sync only file info
    void syncFileInfo();
    //Send message to delete file/dir on remote too
    bool sendDelete(string path, bool dir = false);
    //Send message to update file/dir on remote too
    bool sendUpdate(string path, bool dir = false);*/

    /*//Calls scp to send changes
    void sendChangesToRemote();
    //Notifies remote that a update is on the way
    bool sendRequireUpdate();
    //Notifies the remote that it can start updating
    bool sendAllowUpdate();
    //Notifies remote that the update is finished
    bool sendFinishedUpdate();*/
    
    void treatMessagesCycle();
    void treatMessage(string message);
    void login(string password);
    void auth(string userPassword, string userName, string remoteDir, int transferPort);
    void dir(string op, string dir);
    void fileUp(string file, time_t lastMod);
    void fileRemove(string file);
    void remoteStartSync();
    void remoteEndSync();
    void mkdir(string message, string dir);
    void erase(string message, string obj);

    bool isServer;
    //receiving update from remote / sending update to remote
    bool remoteUpdating, localUpdating;
    //port for local socket
    int hostPort;
    string hostAddress;
    string hostPasswd;
    mutex loginMutex;
    string hostPasswdTry;

    int scpPort = -1;
    int remoteScpPort = -1;

    SyncDir localDir, remoteDir;

    string localDirName, remoteDirName;
    string localPasswd, remotePasswd;
    string localUserName, remoteUserName;
    string remoteAddress;
    Socket *socket = NULL;
    thread *updateThread = NULL;
    thread *treatMsgThread = NULL;

    SyncBotState state;
    AuthState authState;

    bool finishFlag;
    bool authByRemote;
/*Message dialog:
    Auth message:
        auth [local password] [local sync dir] [scpPort]
            scp port is optional
    Login message:
        login [host password]
    Add directory to remote:
        dir add [dir name];
    Remove directory on remote:
        dir rm [dir name];
    Update/Add file to remote:
        file up [file-name] [last-mod-time]
    Remove file from remote:
        file rm [file-name]
    Init micro-sync:
        start-sync
    End micro-sync:
        end-sync
    Make a directory:
        mkdir [dir]
    Delete a directory:
        rm -Rf [dir]
    Delete file:
        rm -f [file]
*/
};



#endif