#ifndef _FILE_UTILS_
#define _FILE_UTILS_

#include <iostream>
#include <chrono>
#include <string>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <vector>
#include <unordered_set>
#include "tinydir.h"

using namespace std;

/* All of the information relevant to a file */
struct FileInfo{
    //full path - sync dir
    string path;
    //name (including extension)
    string name;
    //should I even use this?
    //string extension;
    time_t lastModification;
};

/**
 * Check if a file exists
 * @return true if and only if the file exists, false else
 */
bool fileExists(const char* file);

string timeToChar(time_t t);

//time_t getLastModTime(string filePath);

time_t getLastModTime(const char* filePath);

void compareTwoFiles(char * argv[]);

vector<tinydir_file> getSubFiles(string dir = "./");

FileInfo getFileInfo(string filePath,
                    string fileName);

FileInfo getFileInfo(tinydir_file file);

//unordered_set<FileInfo> getFileInfoFromDir(bool dirs = false, string dir = "./");
unordered_set<string> getDirs(string dirToScan = "./");
//void scanLocalFiles();

#endif