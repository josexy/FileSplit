
#include "FileSplit.h"

std::vector<std::string> FileSplit::split(const std::string &filepath,
                                          long int chunk_size,
                                          bool del_oldfiles,
                                          const std::string &savedir) {
    index_ = 0;
    long __fileTotalSize = _calc_fileSize(filepath);
    if (__fileTotalSize == -1) return {};

    if (chunk_size == 0) chunk_size = _calc_per_chunkSize(__fileTotalSize);

    int per_count = __fileTotalSize / chunk_size;

    per_count /= maxThreads_;
    if (!per_count) per_count += 1;

    std::vector<std::string> vclistFiles = _listDir(savedir);
    if (!vclistFiles.empty() && del_oldfiles)
        for (auto &x : vclistFiles) deleteFile(x.c_str());

    FILE *fReader = fopen(filepath.c_str(), "rb");
    if (!fReader) return {};
    int index = 0;

#ifdef WIN32
    CreateDirectoryA(savedir.c_str(), NULL);
#else
    mkdir(savedir.c_str(), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
#endif

    // add task to thread pool
    std::vector<std::future<std::vector<std::string>>> result;
    for (int i = 0; i < maxThreads_; i++) {
        result.emplace_back(pool_.add(&FileSplit::_do_split_work, this,
                                      filepath, chunk_size, per_count, savedir,
                                      fReader));
    }
    vclistFiles.clear();
    // get result
    for (auto &f : result) {
        auto r = f.get();
        vclistFiles.reserve(vclistFiles.size() +
                            std::distance(r.begin(), r.end()));
        vclistFiles.insert(vclistFiles.end(), r.begin(), r.end());
    }

    fclose(fReader);

    return vclistFiles;
}
std::vector<std::string> FileSplit::_do_split_work(const std::string &filepath,
                                                   long int chunk_size,
                                                   int per_count,
                                                   const std::string &savedir,
                                                   FILE *fReader) {
    char *bufWrite = new char[chunk_size];
    memset(bufWrite, 0, chunk_size);

    int ret = 0;
    std::vector<std::string> vcListFiles;
    char _name_Buf[50];

    for (int i = 0; i < per_count; i++) {
        while ((ret = fread(bufWrite, sizeof(char), chunk_size, fReader)) > 0) {
            {
                std::lock_guard<std::mutex> lock(mutex_);
                ++index_;
            }
            memset(_name_Buf, 0, 50);
            // construct a chunk file : data_chunk/chunk_0001.data
            sprintf(_name_Buf, "%s%cchunk_%04d.data", savedir.c_str(),
                    separator, index_);

            vcListFiles.emplace_back(_name_Buf);
            FILE *fWriter = fopen(_name_Buf, "wb");
            if (!fWriter) continue;
            fwrite(bufWrite, sizeof(char), ret, fWriter);
            fflush(fWriter);
            fclose(fWriter);

            memset(bufWrite, 0, chunk_size);
            fflush(fReader);
        }
    }

    delete[] bufWrite;
    bufWrite = nullptr;

    return vcListFiles;
}

bool FileSplit::merge(const std::string &chunk_files_dir,
                      const std::string &save_mergefile_path, bool del_files) {
    std::vector<std::string> vclistFiles = _listDir(chunk_files_dir);
    if (vclistFiles.empty()) return false;

    sort(vclistFiles.begin(), vclistFiles.end());

    std::string f = vclistFiles.at(0);
    long int max_fileSize = _calc_fileSize(f);
    if (max_fileSize == -1) return false;

    char *_buffer = new char[max_fileSize];

    FILE *fWriter = fopen(save_mergefile_path.c_str(), "wb+");

    for (auto &_chunk_file : vclistFiles) {
        f = _chunk_file;
        FILE *fReader = fopen(f.c_str(), "rb");
        memset(_buffer, 0, max_fileSize);
        int realReadSize = fread(_buffer, sizeof(char), max_fileSize, fReader);
        fflush(fReader);
        fclose(fReader);
        fwrite(_buffer, sizeof(char), realReadSize, fWriter);
        fflush(fWriter);
    }
    delete[] _buffer;
    _buffer = nullptr;
    fclose(fWriter);

    if (del_files)
        for (auto &&x : vclistFiles) deleteFile(x.c_str());

    return true;
}

std::vector<std::string> FileSplit::_listDir(
    const std::string &chunk_files_dir) {
    std::vector<std::string> vclistFiles;

#if defined(__linux__) || defined(__unix__)
    DIR *dp = opendir(chunk_files_dir.c_str());
    if (!dp) return vclistFiles;
    dirent *direntp = NULL;
    while ((direntp = readdir(dp))) {
        if (strcmp(direntp->d_name, ".") != 0 &&
            strcmp(direntp->d_name, "..") != 0) {
            if (std::string(direntp->d_name).find("chunk") !=
                std::string::npos) {
                vclistFiles.emplace_back(chunk_files_dir + separator +
                                         std::string(direntp->d_name));
            }
        }
    }
    closedir(dp);
#else
    WIN32_FIND_DATAA fD;
    HANDLE handle =
        ::FindFirstFileA((chunk_files_dir + "\\*.data").c_str(), &fD);
    if (handle == INVALID_HANDLE_VALUE) return vclistFiles;
    vclistFiles.emplace_back(fD.cFileName);
    while (::FindNextFileA(handle, &fD)) {
        if (strcmp(fD.cFileName, ".") != 0 && strcmp(fD.cFileName, "..") != 0)
            vclistFiles.emplace_back(chunk_files_dir + separator +
                                     std::string(fD.cFileName));
    };
    FindClose(handle);
#endif
    return vclistFiles;
}

long int FileSplit::_calc_fileSize(const std::string &file) {
    FILE *f = fopen(file.c_str(), "rb");
    if (!f) return -1;
    fseek(f, 0, SEEK_END);
    long int size = ftell(f);
    fclose(f);
    return size;
}

long int FileSplit::_calc_per_chunkSize(long int file_size) {
    if (file_size <= 0) return 0;
    srand((unsigned int)time(NULL));
    long int gb = (file_size / 1024 / 1024 / 1024);
    long int kb = (file_size / 1024);
    long int default_chunk_size = kb / (rand() % 4 + 9);  // [9,12]
    if (gb > 0) default_chunk_size /= (gb / 2);
    return int(default_chunk_size) * 1024;
}
