#ifndef STRUCTURE_H
#define STRUCTURE_H

#include <bits/stdc++.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <stdlib.h>
#include <signal.h>
#include <future>
#include <chrono>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/stat.h>
#include <fstream>
#include <chrono>
#include "md5.h"

using namespace std;

#define CheckAlive 1
#define ReplyAlive 2
#define JobAssign 3
#define Result 4
#define IAmUp 5
#define JobSend 6
#define InputSend 7
#define Mapping 8
#define Query 9
#define HeartBeatTime 5
#define TIMEOUT 2
#define MUTEX 25
#define MACHINES 10
#define MAX_CONN 10

#define MAX 256

inline string filerename(string from) {
    MD5 md5;
    unsigned long t = chrono::duration_cast<chrono::milliseconds>(chrono::system_clock::now().time_since_epoch()).count();
    char buf[MAX];
    strcpy(buf,from.c_str());
    strcat(buf, to_string(t).c_str());
    string exf(md5.digestString(buf));
    rename(from.c_str(),exf.c_str());
    return exf;
}

struct Job{
	string execFile, ipFile;
	string jobId,ownerId; //ownerId is ID of owner
};

#endif