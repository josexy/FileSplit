#ifndef FILESPLIT_H
#define FILESPLIT_H

#if defined(__linux__) || defined(__unix__)
#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>
#define deleteFile unlink

#define separator '/'

#else
#define _CRT_SECURE_NO_WARNINGS
#include <Windows.h>
#define deleteFile DeleteFileA

#define separator '\\'

#endif

#include <string.h>
#include <time.h>

#include <algorithm>
#include <string>
#include <vector>

using std::string;
using std::vector;

class FileSplit {
   public:
    FileSplit() {}
    ~FileSplit() {}

    static vector<string> split(const string& filepath, long int chunk_size = 0,
                                bool del_oldfiles= true,
                                const string& savedir = "data_chunk");

    static bool merge(const string& chunk_files_dir,
                      const string& save_mergefile_path,bool del_files=false);

   private:
    /* list dir */
    static vector<string> _listDir(const string& chunk_files_dir);
    /* get file size */
    static long int _calc_fileSize(const string& file);
    /* alc the chunk size from file size */
    static long int _calc_per_chunkSize(long int file_size);
};

#endif
