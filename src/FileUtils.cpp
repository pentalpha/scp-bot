#include "FileUtils.h"

bool fileExists(const char* file) {
    struct stat buf;
    return (stat(file, &buf) == 0);
}

string timeToChar(time_t t){
    int timeLen = 22;
    char buff[timeLen];
    strftime(buff, timeLen, "%H:%M:%S, %Y-%m-%d", localtime(&t));
    return string(buff);
}

/*time_t getLastModTime(string filePath){
    return getLastModTime(filePath.c_str());
}*/

time_t getLastModTime(const char* filePath){
    //cout << "Trying to find mod time for " << filePath << endl;
    struct stat x;
    stat(filePath, &x);
    return x.st_ctime;
}

void compareTwoFiles(char * argv[]){
    if (fileExists(argv[1])){
        cout << argv[1] << " exists" << endl;
    }else{
        cout << argv[1] << " does not exists" << endl;
    }
    if (fileExists(argv[2])){
        cout << argv[2] << " exists" << endl;
    }else{
        cout << argv[2] << " does not exists" << endl;
    }
    time_t a = getLastModTime(argv[1]);
    cout << "File 1: " << timeToChar(a) << endl;
    time_t b = getLastModTime(argv[2]);
    cout << "File 2: " << timeToChar(b) << endl;
    bool aOlder = a < b;
    cout << "File 1 is older than File 2? " << aOlder << endl;
    cout << "File 1 is newer than File 2? " << !aOlder << endl;
    cout << "Current time: " << timeToChar(time(NULL)) << endl;
}

vector<tinydir_file> getSubFiles(bool dirs, string dirToScan){
    vector<tinydir_file> files;
    tinydir_dir dir;
    tinydir_open(&dir, dirToScan.c_str());

    while(dir.has_next){
        tinydir_file file;
        tinydir_readfile(&dir, &file);
        bool condition = !file.is_dir;
        if(dirs){
            condition = file.is_dir;
        }
        if(condition){
            //cout << file.path << endl;
            //string fileName = file.path;
            files.push_back(file);
        }
        tinydir_next(&dir);
    }

    tinydir_close(&dir);

    return files;
}

FileInfo getFileInfo(tinydir_file file){
    struct FileInfo info = getFileInfo(file.path, file.name);
    return info;
}

FileInfo getFileInfo(string filePath,
                    string fileName){
    struct FileInfo info;
    info.path = filePath;
    info.name = fileName;
    info.lastModification = getLastModTime(filePath.c_str());
    return info;
}

vector<FileInfo> getFileInfoFromDir(bool dirs, string dirToScan){
    if(dirs){
        //cout << "Getting subdirs";
    }else{
        cout << "Getting subfiles";
    }
    //cout << " for " << dirToScan << endl;
    vector<tinydir_file> tinyFiles = getSubFiles(dirs, dirToScan);
    //cout << "got them" << endl;
    vector<FileInfo> fileInfos;
    for(tinydir_file f : tinyFiles)
    {
        //cout << "Trying to get file info for " << f.path << endl;
        FileInfo info = getFileInfo(f);
        //cout << "Got the info\n";
        fileInfos.push_back(info);
    }
    return fileInfos;
}

void scanLocalFiles(){
    vector<FileInfo> localFiles = getFileInfoFromDir();
    cout << "Local files:\n";
    for(FileInfo s : localFiles){
        cout << "\t" << s.path << "\t" << timeToChar(s.lastModification) << endl;
    }
    vector<FileInfo> localDirs = getFileInfoFromDir(true);
    cout << "Local dirs:\n";
    for(FileInfo s : localDirs){
        cout << "\t" << s.path << "\t" << timeToChar(s.lastModification) << endl;
    }
    vector<FileInfo> userFiles = getFileInfoFromDir(false, "/home/pitagoras/");
    cout << "User files:\n";
    for(FileInfo s : userFiles){
        cout << "\t" << s.path << "\t" << timeToChar(s.lastModification) << endl;
    }
}