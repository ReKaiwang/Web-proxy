//
// Created by Kai Wang on 2019-02-22.
//

#include "proxyDaemon.h"
void proxymanager::runWithCache() {
    proxyDaemon pd;
    while(1) {
        string requestContent = pd.acceptReq();
        pd.createThread();
        string methodstr, urlstr;
        pd.parseReq(requestContent, methodstr, urlstr);

        // 'GET' or 'POST'
        if (methodstr == "GET") {
            // ...
            // create socket connected to target server
            // send request
            // recv response
            // send back to client
        }
        else if(methodstr == "POST") {
            // ...
            // create socket connected to target server
            // send request
            // recv response
        }
        else {
            // ...thread exception
        }
    }
}