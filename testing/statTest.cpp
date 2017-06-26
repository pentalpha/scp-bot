#include <iostream>
#include <chrono>
#include <string>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <vector>
#include "src/tinydir.h"
#include "src/FileUtils.h"

using namespace std;

int main(int argc, char * argv[]){
    if(argc == 3){
        compareTwoFiles(argv);
    }else if (argc == 1){
        cout << "scanning local files:\n";
        scanLocalFiles();
    }
    return 0;
}

