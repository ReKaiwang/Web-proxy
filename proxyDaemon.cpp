//
// Created by Kai Wang on 2019-02-22.
//

#include "proxyDaemon.h"
#include <iostream>
#include <map>
#include <string>
#include <string.h>
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
    //verify method
    char method[4];
    status = recv(*(int *) sock_fd, method, sizeof(method)- sizeof(char), 0);
    if (status == -1) {
        cerr << "fail to get message from client" << endl;
        exit(EXIT_FAILURE);
    }
    method[3] = '\0';
    //if GET
    if(strcmp(method,"GET") == 0){
        pd.recvGET(*(int *) sock_fd);
    }
    //if POST
//    else if(strcmp(method,"POS")==0){
//        pd.recvPOST(*(int *) sock_fd);
//    }

    //cout << pd.client_buff;
    //parse the http message
    pd.parseReq();
    pd.conToServer();
    pd.responReq(*(int*) sock_fd);
    return NULL;
}
void proxyDaemon::recvGET(int sock_fd) {
    client_buff.append("GET");
    int status;
    while(1) {
        char tempbuff[256];
        status = recv(sock_fd, tempbuff, sizeof(tempbuff), 0);
        if (status == -1) {
            cerr << "fail to get message from client" << endl;
            exit(EXIT_FAILURE);
        }
        else {
            client_buff.append(tempbuff);
            if(client_buff.find("\r\n\r\n") != string::npos)
                break;
            memset(tempbuff, 0, sizeof(tempbuff));
        }
    }
    cout << client_buff;
}
void proxyDaemon::parseReq() {
    //parse the request line
//    int spacepos = temphttp.find(' ');
//    int prepos;
//    myreqline.method = temphttp.substr(0,spacepos);
//    temphttp.erase(spacepos, 1);
//    prepos = spacepos;
//    spacepos = temphttp.find(' ');
//    myreqline.URI = temphttp.substr(prepos,spacepos-prepos);
//    temphttp.erase(spacepos, 1);
//    prepos = spacepos;
//    spacepos = temphttp.find("\r\n");
//    myreqline.version= temphttp.substr(prepos,spacepos-prepos);
//    temphttp.erase(spacepos, 1);
    string temp = client_buff;
    //cout<< client_buff;
    int sepepoint = temp.find("\r\n");
    if(sepepoint == string::npos){
        cerr<<"can't find request line";
        //exit(EXIT_FAILURE);
    }
    string firstline = temp.substr(0,sepepoint);
    parsereqline(firstline);
    temp.erase(sepepoint,2);
    //parse the request header
//    //remember that the request header may have multiple lines!!--which I havn't implemented
//    prepos = spacepos;
//    spacepos = temphttp.find(' ');
//    myreqheader.name= temphttp.substr(prepos,spacepos-prepos);
//    temphttp.erase(spacepos, 1);
//    prepos = spacepos;
//    spacepos = temphttp.find("\r\n");
//    myreqheader.value= temphttp.substr(prepos,spacepos-prepos);
//    //temphttp.erase(spacepos, 1);
    string header = temp.substr(sepepoint,temp.find("\r\n\r\n")-sepepoint);
    parsereqhead(header);

}
void proxyDaemon::parsereqline(string& reqline) {
    int spacepos = reqline.find(' ');
    int prepos;
    myreqline.method = reqline.substr(0,spacepos);
    reqline.erase(spacepos, 1);
    prepos = spacepos;
    spacepos = reqline.find(' ');
    myreqline.URI = reqline.substr(prepos,spacepos-prepos);
    reqline.erase(spacepos, 1);
    myreqline.version = reqline.substr(spacepos);
}
void proxyDaemon::parsereqhead(string &reqhead) {
   //cout << reqhead;
    int hostpoint = reqhead.find("Host");
    if(hostpoint == string::npos){
        cerr<<"can't find the host"<<endl;
        exit(EXIT_FAILURE);
    }
    string tempsubstr = reqhead.substr(hostpoint);
    myreqheader["Host"] = tempsubstr.substr(hostpoint+6, tempsubstr.find("\r\n")-hostpoint-6);
}
void proxyDaemon::conToServer() {
    //make a socket connecting with server
    int sock_fd;
    int status;
    struct addrinfo host_info, * host_info_list;
    memset(&host_info, 0, sizeof(host_info));
    host_info.ai_family   = AF_INET;
    host_info.ai_socktype = SOCK_STREAM;
    cout<<myreqheader["Host"]<<endl;
    //get the host address information
    status = getaddrinfo(myreqheader["Host"].c_str(),"80", & host_info, &host_info_list);
    if (status != 0) {
        perror("Error: cannot get server address\n");
        exit(EXIT_FAILURE);
    }
    sock_fd  = socket(host_info_list->ai_family,
                        host_info_list->ai_socktype,
                        host_info_list->ai_protocol);
    if (sock_fd == -1) {
        cerr << "Error: cannot create socket to connect with server" << endl;
        exit(EXIT_FAILURE);
    }
    //connect with server
    status = connect(sock_fd, host_info_list->ai_addr,host_info_list->ai_addrlen);
    if(status == -1){
        cerr << "fail to connect with server" << endl;
        exit(EXIT_FAILURE);
    }
    //string httpToserver = stickytogether();
    status = send(sock_fd,client_buff.c_str(),(size_t)client_buff.size(),0 );
    if(status == -1){
        cerr << "fail to send message to server" << endl;
        exit(EXIT_FAILURE);
    }
//    char tempbuff[25600];
//    status = recv(sock_fd, tempbuff, sizeof(tempbuff), MSG_WAITALL);
//        if (status == -1) {
//            cerr << "fail to receive message from server" << endl;
//            exit(EXIT_FAILURE);
//        }
    //receive content from server
    long block_size = 0;
    //first get the response header
    while(1) {
        char tempbuff[256];
        status = recv(sock_fd, tempbuff, sizeof(tempbuff), 0);
        if (status == -1) {
            cerr << "fail to receive message from server" << endl;
            exit(EXIT_FAILURE);
        }
        else{
            server_buff.append(tempbuff);
            block_size += status;
            cout << tempbuff;
            if(server_buff.find("r\n\r\n") != string::npos)
                break;
            memset(tempbuff, 0, sizeof(tempbuff));
        }
    }
    size_t findheader;
    int header_length=0;
    if((findheader = server_buff.find("Content-Length")) != string::npos){
        //findheader = atoi(server_buff.substr(findheader+16, ))
        string tempstr = server_buff.substr(findheader+16);
        header_length = atoi(tempstr.substr(0,tempstr.find("\r\n\r\n")).c_str());
    }
    cout << header_length;
    while(1){
        char tempbuff[256];
        status = recv(sock_fd, tempbuff, sizeof(tempbuff), 0);
        if (status == -1) {
            cerr << "fail to receive message from server" << endl;
            exit(EXIT_FAILURE);
        }
        else{
            server_buff.append(tempbuff);
            block_size += status;
            if(block_size >= header_length)
                break;
            memset(tempbuff, 0, sizeof(tempbuff));
        }
    }
//    else if((findheader = server_buff.find("Content-Length")) != string::npos)
    cout <<server_buff;
    //server_buff.append(buff);
}
void proxyDaemon::responReq(int sock_fd) {
    int status;
    status = send(sock_fd,server_buff.c_str(),(size_t)server_buff.size(),0 );
    if(status == -1){
        cerr << "fail to sendback to client" << endl;
        exit(EXIT_FAILURE);
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

