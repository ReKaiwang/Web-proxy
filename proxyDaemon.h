//
// Created by Kai Wang on 2019-02-22.
//

#ifndef ERSS_HWK2_KW283_ZH89_PROXYDAEMON_H
#define ERSS_HWK2_KW283_ZH89_PROXYDAEMON_H

//define our request header for parse
//
#include <iostream>
#include <map>
#include <string>
#include <pthread.h>
using namespace std;
struct _reqline{
       string method;
       string URI;
       string version;
};
typedef _reqline reqline;

struct _reqheader{
    string name;
    string value;
};
typedef _reqheader reqheader;

class proxyDaemon {
private:
    // make a inner class for request line
//    class reqline{
//    private:
//        string method;
//        string URI;
//        string version;
//    public:
//        reqline():method(),URI(),version(){}
//    };
//    class reqheader{
//    private:
//        string name;
//        string value;
//    public:
//        reqheader():name(),value(){}
//    };
    reqline myreqline;
    reqheader myreqheader;
    //http message from client
    char client_buff[256];
public:
    static void createThread(int sock_fd);
    static void* acceptReq(void *sock_fd); // accept HTTP request from cline
    void parseReq(); // parse HTTP request into method, URL and context
    void conToServer(); // connect to required server
//    createThread(); // create a thread
//    readCache(); // read from cache
//    writeCache(); // write to cache
//    writeLog(); // write to log file
//    responReq(); // response to client
};

class proxymanager{
public:
    static void runWithCache(); // run program use cache
    static void runWithoutCache(char* port_n); // run program without using cache
};

#endif //ERSS_HWK2_KW283_ZH89_PROXYDAEMON_H
