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
    }else{
        cout << "scanning local files:\n";
        scanLocalFiles();
    }
    return 0;
}

