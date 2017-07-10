#include <iostream>
#include <string>
#include "../include/SyncArgs.h"
#include "../include/SyncBot.h"

using namespace std;

int main(int argc, char * argv[]){
    SyncArgs args(argc, argv);
    if(args.success){
        SyncBot bot(args);
        bot.run();
    }
    
}