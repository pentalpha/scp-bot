#include <iostream>
#include <string>
#include "OctoSyncArgs.h"
#include "SyncBot.h"

using namespace std;

int main(int argc, char * argv[]){
    OctoSyncArgs args(argc, argv);
    if(args.success){
        SyncBot bot(args);
        bot.run();
    }
    
}