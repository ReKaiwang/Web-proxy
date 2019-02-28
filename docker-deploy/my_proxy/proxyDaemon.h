//
// Created by Kai Wang on 2019-02-22.
//

#ifndef ERSS_HWK2_KW283_ZH89_PROXYDAEMON_H
#define ERSS_HWK2_KW283_ZH89_PROXYDAEMON_H

// define our request header for parse
//
#include <iostream>
#include <map>
#include <pthread.h>
#include <string>
#include "logger.h"
using std::map;
using std::string;
using std::cout;
using std::endl;
using std::cerr;
using std::terminate;
// first line in request
struct _reqline {
  string method;
  string URI;
  string version;
};
typedef _reqline reqline;

struct clientIP{
    int sock_fd;
    string IP4;
    clientIP(int sk, string ip_4):sock_fd(sk),IP4(ip_4){}
};
/*struct _reqheader{
    string name;
    string value;
};
typedef _reqheader reqheader;*/

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
  // reqheader myreqheader;
  map<string, string> myreqheader;
  // http message from client
  string client_buff;
  string server_buff;

public:
  static int createListenFd();
  static void handleReq(clientIP* sock_fd);
  static void *acceptReq(void *sock_fd); // accept HTTP request from cline
  int parseReq();     // parse HTTP request into method, URL and context
  int conToServer(logger& mylogger); // connect to required server
  void responReq(int client_fd, int server_fd,logger& mylogger); //receive response from server
  void ssresponReq(int client_fd, int server_fd, logger& mylogger);
  template <bool flag>
  void recvHTTP(int sock_fd, string& recvbuff,int noncontentsize = 0, int content_length = 0);
  void recvSSLHTTP(int sock_fd, string& recvbuff);

  //    readCache(); // read from cache
  //    writeCache(); // write to cache
  //    writeLog(); // write to log file
  //    responReq(); // response to client
private:
  // transform the http from client to server
  // string stickytogether();
  void recvGET(int sock_fd);
  void recvPOST(int sock_fd);
  void recvCONNECT(int sock_fd);
  void parsereqline(string &reqline);
  void parsereqhead(string &reqhead);
  void parsereqheadhelp(string &perline);
 // int HexToDec(int num);
  int recvChunkedsize(int sock_fd,string& chunkedstr);
  void recvChunkedbody(int sock_fd, string& recvbuff );
  int selectRecv(int recv_fd, int send_fd);
  void recvChunked(int server_fd, int client_fd);
};

class proxymanager {
public:
  static void runWithCache();                // run program use cache
  //static void runWithoutCache(char *port_n); // run program without using cache
};

#endif // ERSS_HWK2_KW283_ZH89_PROXYDAEMON_H
