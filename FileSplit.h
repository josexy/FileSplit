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
#include <functional>
#include <string>
#include <vector>

#include "ThreadPool.h"

class FileSplit {
   public:
    FileSplit(size_t maxThreads)
        : pool_(maxThreads), maxThreads_(maxThreads), index_(0) {}
    ~FileSplit() {}

    std::vector<std::string> split(const std::string& filepath,
                                   long int chunk_size = 0,
                                   bool del_oldfiles = true,
                                   const std::string& savedir = "data_chunk");

    bool merge(const std::string& chunk_files_dir,
               const std::string& save_mergefile_path, bool del_files = false);

   private:
    std::vector<std::string> _do_split_work(const std::string& filepath,
                                            long int chunk_size, int per_count,
                                            const std::string& savedir, FILE*);

    std::vector<std::string> _listDir(const std::string& chunk_files_dir);
    long int _calc_fileSize(const std::string& file);
    long int _calc_per_chunkSize(long int file_size);

   private:
    ThreadPool pool_;
    int maxThreads_;
    int index_;

    std::mutex mutex_;
};

#endif
