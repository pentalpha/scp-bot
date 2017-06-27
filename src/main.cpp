#include <iostream>
#include <string>
//#include "Server.h"
//#include "Client.h"
//#include "logging.h"
//#include "run.h"
#include "OctoSyncArgs.h"

using namespace std;

//void startServer(string ip, int port);
//void startClient(string ip, int port);

int main(int argc, char * argv[]){
    OctoSyncArgs args(argc, argv);

    /*if(argc == 4){
        string op(argv[1]);
        string ip(argv[2]);
        int port = stoi(argv[3]);

        cout << "Host is in " << ip << "::" << port << endl;

        if(op == "host"){
            startServer(ip, port);
        }else if(op == "sync"){
            startClient(ip, port);
        }else{
            log("ARGS", "Invalid operation:");
            log("ARGS", op);
        }
    }*/
    
}

/*void startServer(string ip, int port){
    Server server(ip.c_str(), port);
    server.startTransaction();
    cout << "Started\n";
    server.waitToFinish();
    log("MAIN", "Server not connected anymore");
    //server.stop();
}*/

/*void startClient(string ip, int port){
    Client client(ip, port);
    client.startTransaction();
    client.waitToFinish();
}*/