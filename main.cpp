
#include "FileSplit.h"

#include <iostream>

void split(const string &filepath) {
    FileSplit fileSplit(std::thread::hardware_concurrency());
    fileSplit.split(filepath);
}

void merge(const string &data_files_dir, const string &save_file) {
    FileSplit fileSplit(std::thread::hardware_concurrency());
    fileSplit.merge(data_files_dir, save_file);
}

int main(int argc, char **argv) {
    if (argc <= 1) return 0;
    if (argc == 3 && argv[1][0] == 's')
        split(argv[2]);
    else if (argc == 4 && argv[1][0] == 'm') {
        merge(argv[2], argv[3]);
    } else {
        std::cout << "Error\n";
    }
    return 0;
}
