#include <iostream>
#include <chrono>
#include <string>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <vector>
#include <thread>
#include "../src/tinydir.h"
#include "../src/FileUtils.h"
#include "../src/SyncDir.h"

using namespace std;

int main(int argc, char * argv[]){
    if (argc == 3){
        string dir(argv[1]);
        SyncDir syncDir(dir, true);
        int s = stoi(argv[2]);
        log("MAIN", string("Scanning for ") + to_string(s) + "s:");
        std::this_thread::sleep_for(std::chrono::milliseconds(s*1000));
        vector<string> changes = syncDir.popChanges();
        for(string change : changes){
            log("MAIN", string("Change made:\n\t ") + change);
        }
        syncDir.finish();
    }
    return 0;
    
}

