#include "SyncBot.h"

SyncBot::SyncBot(OctoSyncArgs args){
    isServer = args.host;
    localDirName = getAbsolutePath(args.syncDir);
    hostPort = args.hostPort;
    hostAddress = args.hostAddress;
    hostPasswd = args.hostPasswd;

    if(!isServer){
        localPasswd = args.localPasswd;
        remotePasswd = hostPasswd;
    }else{
        localPasswd = hostPasswd;
        remotePasswd = "";
    }

    if(args.scpPort != -1){
        scpPort = args.scpPort;
    }

    remoteUpdating = localUpdating = false;

    socket = makeSocket(hostAddress, hostPort, isServer);
    if(socket == NULL){
        error("SYNC-BOT", "Error during socket creation");
        exit(1);
    }
    state = SyncBotState.SLEEP;
}

Socket* SyncBot::makeSocket(string ip, int port, bool server){
    Socket* socket;
    if(server){
        socket = new Server(ip.c_str(), port);
    }else{
        socket = new Client(ip.c_str(), port);
    }
}