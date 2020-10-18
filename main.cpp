#include<iostream>
#include"FileSplit.h"
using namespace std;

int main(int argc,char **argv) {
    if(argc<=2) return 0;
	FileSplit fs;
    if(argc == 3 && argv[1][0]=='s')
    	fs.split(argv[2]);
	else if(argc == 4 && argv[1][0]=='m')
        fs.merge(argv[2], argv[3]);
	return 0;
}
