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

bool isValidDirToScan(string path){
    bool open = true;
    char last, last2, last3;
    int len = path.length();
    last = path[len-1];
    if(last == '.'){
        if(len >= 2){
            last2 = path[len-2];
            if(last2 == '/'){
                open = false;
            }else if(last2 == '.'){
                if(len >= 3){
                    last3 = path[len-3];
                    if(last3 == '/'){
                        open = false;
                    }
                }
            }
        }
    }
    return open;
}

vector<tinydir_file> getSubFiles(string dirToScan){
    //cout << "scanning " << dirToScan << endl;
    vector<tinydir_file> files;
    tinydir_dir dir;
    tinydir_open(&dir, dirToScan.c_str());

    while(dir.has_next){
        tinydir_file file;
        tinydir_readfile(&dir, &file);
        bool isDir = file.is_dir;
        if(isDir){
            bool open = isValidDirToScan(file.path);
            //cout << "open? '" << file.path << "' : " << open << endl;
            if(open){
                vector<tinydir_file> subFiles = getSubFiles(file.path);
                if(subFiles.size() > 0){
                    //cout << file.path << " adding subfiles:\n";
                    files.insert(std::end(files), std::begin(subFiles),
                            std::end(subFiles));
                }else{
                    //cout << file.path << " is empty\n";
                }
                files.push_back(file);
            }
        }else{
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

/*unordered_set<FileInfo> getFileInfoFromDir(bool dirs, string dirToScan){
    if(dirs){
        //cout << "Getting subdirs";
    }else{
        cout << "Getting subfiles";
    }
    //cout << " for " << dirToScan << endl;
    vector<tinydir_file> tinyFiles = getSubFiles(dirs, dirToScan);
    //cout << "got them" << endl;
    unordered_set<FileInfo> fileInfos;
    for(tinydir_file f : tinyFiles)
    {
        //cout << "Trying to get file info for " << f.path << endl;
        FileInfo info = getFileInfo(f);
        //cout << "Got the info\n";
        fileInfos.insert(info);
    }
    return fileInfos;
}*/

unordered_set<string> getDirs(string dirToScan){
    unordered_set<string> dirs;
    tinydir_dir dir;

    tinydir_open(&dir, dirToScan.c_str());
    while(dir.has_next){
        tinydir_file file;
        tinydir_readfile(&dir, &file);
        bool isDir = file.is_dir;
        if(file.is_dir){
            bool open = isValidDirToScan(file.path);
            //cout << "open? '" << file.path << "' : " << open << endl;
            if(open){
                unordered_set<string> subDirs = getDirs(file.path);
                if(subDirs.size() > 0){
                    //cout << file.path << " adding subfiles:\n";
                    dirs.insert(subDirs.begin(), subDirs.end());
                }else{
                    //cout << file.path << " is empty\n";
                }
                dirs.insert(file.path);
            }
        }
        tinydir_next(&dir);
    }
    tinydir_close(&dir);

    return dirs;
}

string getAbsolutePath(string hint){
    char absPath[PATH_MAX];
    realpath(hint.c_str(), absPath);
    return string(absPath);
}

/*void scanLocalFiles(){
    auto localFiles = getFileInfoFromDir();
    cout << "Local files:\n";
    for(FileInfo s : localFiles){
        cout << "\t" << s.path << "\t" << timeToChar(s.lastModification) << endl;
    }
    auto localDirs = getFileInfoFromDir(true);
    cout << "Local dirs:\n";
    for(FileInfo s : localDirs){
        cout << "\t" << s.path << "\t" << timeToChar(s.lastModification) << endl;
    }
    auto userFiles = getFileInfoFromDir(false, "/home/pitagoras/");
    cout << "User files:\n";
    for(FileInfo s : userFiles){
        cout << "\t" << s.path << "\t" << timeToChar(s.lastModification) << endl;
    }
}*/