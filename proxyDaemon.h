//
// Created by Kai Wang on 2019-02-22.
//

#ifndef ERSS_HWK2_KW283_ZH89_PROXYDAEMON_H
#define ERSS_HWK2_KW283_ZH89_PROXYDAEMON_H

//define our request header for parse
//
class proxyDaemon {
private:

public:
    acceptReq(); // accept HTTP request from cline
    parseReq(); // parse HTTP request into method, URL and context
    conToServer(); // connect to required server
    createThread(); // create a thread
    readCache(); // read from cache
    writeCache(); // write to cache
    writeLog(); // write to log file
    responReq(); // response to client
};

class proxymanager{
public:
    void runWithCache(); // run program use cache
    void runWithoutCache(); // run program without using cache
};

#endif //ERSS_HWK2_KW283_ZH89_PROXYDAEMON_H
