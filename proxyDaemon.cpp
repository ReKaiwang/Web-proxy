//
// Created by Kai Wang on 2019-02-22.
//

#include "proxyDaemon.h"
#include <iostream>
#include <map>
#include <string>
#include <sys/socket.h>
#include <sys/select.h>
#include <netdb.h>
#include <pthread.h>
using namespace std;
//implementation of proxyDaemon class
void proxyDaemon::createThread(int sock_fd) {
    pthread_t thread;
    pthread_create(&thread, NULL, proxyDaemon::acceptReq,(void *)&sock_fd);
}
void* proxyDaemon::acceptReq(void *sock_fd) {
    //make a proxy object to receive http, parse, send to target server and then return response
    proxyDaemon pd;
    int status;
    //thread detach
    //...
    //receive http from client
    status = recv(*(int*)sock_fd, pd.client_buff, sizeof(pd.client_buff), MSG_WAITALL);
    if(status == -1){
        cerr << "fail to get message from client" << endl;
        exit(EXIT_FAILURE);
    }
    //parse the http message
    pd.parseReq();
    pd.conToServer();
    return NULL;
}
void proxyDaemon::parseReq() {
    //parse the request line
    string temphttp(client_buff);
    int spacepos = temphttp.find(' ');
    int prepos;
    myreqline.method = temphttp.substr(0,spacepos);
    temphttp.erase(spacepos, 1);
    prepos = spacepos;
    spacepos = temphttp.find(' ');
    myreqline.URI = temphttp.substr(prepos,spacepos-prepos);
    temphttp.erase(spacepos, 1);
    prepos = spacepos;
    spacepos = temphttp.find('\n');
    myreqline.version= temphttp.substr(prepos,spacepos-prepos);
    temphttp.erase(spacepos, 1);

    //parse the request header
    //remember that the request header may have multiple lines!!--which I havn't implemented
    prepos = spacepos;
    spacepos = temphttp.find(' ');
    myreqheader.name= temphttp.substr(prepos,spacepos-prepos);
    temphttp.erase(spacepos, 1);
    prepos = spacepos;
    spacepos = temphttp.find('\n');
    myreqheader.value= temphttp.substr(prepos,spacepos-prepos);
    temphttp.erase(spacepos, 1);

}
void proxyDaemon::conToServer() {
    //make a socket connecting with server
    int sock_fd
    int status;
    struct addrinfo host_info, * host_info_list;
    status = getaddrinfo(reqheader.value.c_str(),"80", & listen_info, &listen_info_list);
    if (status != 0) {
        perror("Error: cannot get address info for listen\n");
        return EXIT_FAILURE;
    }
}

//implementation of proxymanager class
void proxymanager::runWithoutCache(char* port_n) {
    // proxyDaemon pd;
    //pthread_t thread;
    int listen_sofd;
    struct addrinfo host_info, *host_info_list;
    struct sockaddr_in client_addr;
    int status;
    //char buff[256];
    //initialize the host_info with 0
    memset(&host_info, 0, sizeof(host_info));

    //set the socket type
    host_info.ai_family = AF_INET;
    host_info.ai_socktype = SOCK_STREAM;
    host_info.ai_flags = AI_PASSIVE;

    //get the proxy address information
    if ((status = getaddrinfo(NULL, port_n, &host_info, &host_info_list)) != 0) {
        cerr << "getaddrinfo for listen error:" << gai_strerror(status);
        exit(EXIT_FAILURE);
    }
    //make a socket for listen client
    listen_sofd = socket(host_info_list->ai_family,
                         host_info_list->ai_socktype,
                         host_info_list->ai_protocol);

    if (listen_sofd == -1) {
        cerr << "Error: cannot create socket" << endl;
        exit(EXIT_FAILURE);
    }

    //begin listening
    if ((status = bind(listen_sofd, host_info_list->ai_addr, host_info_list->ai_addrlen)) == -1) {
        cerr << "Error: cannot bind socket" << endl;
        exit(EXIT_FAILURE);
    }
    //status = listen(socket_fd, 100);
    //later change print error to throw exception
    if ((status = listen(listen_sofd, 100)) == -1) {
        cerr << "Error: cannot listen on socket" << endl;
        exit(EXIT_FAILURE);
    }
    socklen_t addr_size = sizeof(client_addr);
//    int new_fd = accept(listen_sofd, (struct sockaddr * ) (&client_addr), & addr_size);
//    status = recv(new_fd, buff, sizeof(buff), 0);
//    cout << buff << endl;
    while (1) {
        int new_fd = accept(listen_sofd, (struct sockaddr *) (&client_addr), &addr_size);
        proxyDaemon::createThread(new_fd);

//        // 'GET' or 'POST'
//        if (methodstr == "GET") {
//            // ...
//            // create socket connected to target server
//            // send request
//            // recv response
//            // send back to client
//        }
//        else if(methodstr == "POST") {
//            // ...
//            // create socket connected to target server
//            // send request
//            // recv response
//        }
//        else {
//            // ...thread exception
//        }
//

    }
}

//main function here
int main(int argc, char** argv){
    if(argc != 2){
        cerr<< "Usage error: give me a port number"<<endl;
        return -1;
    }
    proxymanager::runWithoutCache(argv[1]);
    return 1;

}

