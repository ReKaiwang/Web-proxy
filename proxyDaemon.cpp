//
// Created by Kai Wang on 2019-02-22.
//

#include "proxyDaemon.h"
#include <iostream>
#include <map>
#include <netdb.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <string>
#include <sys/select.h>
#include <sys/socket.h>
#include <unistd.h>
using namespace std;
// a heap lock
pthread_rwlock_t heaplock = PTHREAD_RWLOCK_INITIALIZER;
// implementation of proxyDaemon class
void proxyDaemon::handleReq(int sock_fd) {
  pthread_t thread;
  pthread_create(&thread, NULL, proxyDaemon::acceptReq, (void *)&sock_fd);
}

/* create a proxy object to receive http req, then parse
 * and send req to target server and then
 * return response
 */
void *proxyDaemon::acceptReq(void *sock_fd) {
  proxyDaemon pd;
  int status;
  char method[4];
  status = recv(*(int *)sock_fd, method, sizeof(method) - sizeof(char), 0);
  if (status == -1) {
    cerr << "fail to get http request from client" << endl;
    return NULL;
  }
  method[3] = '\0';
  // if GET
  if (strcmp(method, "GET") == 0) {
    cout << "get request" << endl;
    pd.recvGET(*(int *)sock_fd);
  }
  // if POST
  else if (strcmp(method, "POS") == 0) {
    pd.recvPOST(*(int *)sock_fd);
  }

  // if CONNECT

  // parse the http message
  if (pd.parseReq() == -1) {
    close(*(int *)sock_fd);
    return NULL;
  }

  pd.conToServer();
  pd.responReq(*(int *)sock_fd);
  close(*(int *)sock_fd);
  return NULL;
}

/* handle request with method 'GET'
 * get the http request from client
 */
void proxyDaemon::recvGET(int sock_fd) {
  client_buff.append("GET");
  /*
  int status;
  while (1) {
    char tempbuff[256];
    status = recv(sock_fd, tempbuff, sizeof(tempbuff), 0);
    if (status == -1) {
      cerr << "fail to get message from client" << endl;
      exit(EXIT_FAILURE);
    } else {
      client_buff.append(tempbuff, status);
      if (client_buff.find("\r\n\r\n") != string::npos)
        break;
      memset(tempbuff, 0, sizeof(tempbuff));
    }
  }

  // cout << client_buff.find("\r\n\r\n");
  cout << client_buff;
  */
  recvHTTP<false>(sock_fd, 0, 0);
}

/* receive all the contents from the client*/
template <bool flag>
void proxyDaemon::recvHTTP(int sock_fd, int noncontentsize,
                           int content_length) {
  int status;
  while (1) {
    char tempbuff[256];
    status = recv(sock_fd, tempbuff, sizeof(tempbuff), 0);
    if (status == -1) {
      cerr << "fail to receive message from server" << endl;
      // exit(EXIT_FAILURE);
      terminate();
    } else {

      if (flag) {
        server_buff.append(tempbuff, status);
        if (status == 0) {
          break;
        }
        if (client_buff.size() - noncontentsize + 2 >= content_length) {
          break;
        }
      } else {
        client_buff.append(tempbuff, status);
      }

      if (!flag && client_buff.find("\r\n\r\n") != string::npos) {
        break;
      }
      memset(tempbuff, 0, sizeof(tempbuff));
    }
  }
  cout << client_buff << endl;
}

/* handle request with method 'POST'
 * get the http request from client
 */
void proxyDaemon::recvPOST(int sock_fd) {
  client_buff.append("POS");
  recvHTTP<false>(sock_fd, 0, 0);
  size_t findlength;
  int content_length = 0;
  if ((findlength = client_buff.find("Content-Length")) != string::npos) {
    // findheader = atoi(server_buff.substr(findheader+16, ))
    string tempstr = client_buff.substr(findlength + 16);
    content_length = atoi(tempstr.substr(0, tempstr.find("\r\n")).c_str());
    content_length = octToDec(content_length);
    int noncontentsize =
        client_buff.substr(0, client_buff.find("\r\n\r\n") + 4).size();
    recvHTTP<true>(sock_fd, noncontentsize, content_length);
  } else {
    cerr << "wrong POST form" << endl;
  }
  cout << client_buff << endl;
  cout << "POST SUCESS" << endl;
}

long proxyDaemon::octToDec(long num) {
  long result = 0;
  int ot = 1;
  while (num > 0) {
    result += num % 10 * ot;
    num /= 10;
    ot = ot * 8;
  }
  return result;
}

int proxyDaemon::parseReq() {
  // parse the request line
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
  // cout<< client_buff;
  size_t sepepoint = temp.find("\r\n");
  if (sepepoint == string::npos) {
    cerr << "can't find request line" << endl;
    return -1;
  }
  string firstline = temp.substr(0, sepepoint);
  parsereqline(firstline);
  temp.erase(sepepoint, 2);
  // parse the request header
  //    remember that the request header may have multiple lines!!--which I
  //    havn't implemented prepos = spacepos; spacepos = temphttp.find(' ');
  //    myreqheader.name= temphttp.substr(prepos,spacepos-prepos);
  //    temphttp.erase(spacepos, 1);
  //    prepos = spacepos;
  //    spacepos = temphttp.find("\r\n");
  //    myreqheader.value= temphttp.substr(prepos,spacepos-prepos);
  //    temphttp.erase(spacepos, 1);
  string header = temp.substr(sepepoint, temp.find("\r\n\r\n") - sepepoint);
  parsereqhead(header);
  return 1;
}
void proxyDaemon::parsereqline(string &reqline) {
  int spacepos = reqline.find(' ');
  int prepos;
  myreqline.method = reqline.substr(0, spacepos);
  reqline.erase(spacepos, 1);
  prepos = spacepos;
  spacepos = reqline.find(' ');
  myreqline.URI = reqline.substr(prepos, spacepos - prepos);
  reqline.erase(spacepos, 1);
  myreqline.version = reqline.substr(spacepos);
}
void proxyDaemon::parsereqhead(string &reqhead) {
  // cout << reqhead;
  int hostpoint = reqhead.find("Host");
  if (hostpoint == string::npos) {
    cerr << "can't find the host" << endl;
    // exit(EXIT_FAILURE);
    terminate();
  }
  string tempsubstr = reqhead.substr(hostpoint);
  myreqheader["Host"] =
      tempsubstr.substr(hostpoint + 6, tempsubstr.find("\r\n") - hostpoint - 6);
}
void proxyDaemon::conToServer() {
  // make a socket connecting with server
  int sock_fd;
  int status;
  struct addrinfo host_info, *host_info_list;
  memset(&host_info, 0, sizeof(host_info));
  host_info.ai_family = AF_INET;
  host_info.ai_socktype = SOCK_STREAM;
  // cout<<myreqheader["Host"]<<endl;
  // get the host address information
  pthread_rwlock_wrlock(&heaplock);
  string port = "80";
  size_t findport;
  if ((findport = myreqheader["Host"].find(":")) != string::npos) {
    port = myreqheader["Host"].substr(findport + 1);
    myreqheader["Host"].erase(findport);
  }
  cout << myreqheader["Host"] << endl;
  cout << port << endl;
  status = getaddrinfo(myreqheader["Host"].c_str(), port.c_str(), &host_info,
                       &host_info_list);
  if (status != 0) {
    perror("Error: cannot get server address\n");
    // exit(EXIT_FAILURE);
    terminate();
  }
  sock_fd = socket(host_info_list->ai_family, host_info_list->ai_socktype,
                   host_info_list->ai_protocol);
  if (sock_fd == -1) {
    cerr << "Error: cannot create socket to connect with server" << endl;
    //    exit(EXIT_FAILURE);
    terminate();
  }
  // connect with server
  status =
      connect(sock_fd, host_info_list->ai_addr, host_info_list->ai_addrlen);
  pthread_rwlock_unlock(&heaplock);
  if (status == -1) {
    cerr << "fail to connect with server" << endl;
    terminate();
  }
  // string httpToserver = stickytogether();
  status = send(sock_fd, client_buff.c_str(), (size_t)client_buff.size(), 0);
  if (status == -1) {
    cerr << "fail to send message to server" << endl;
    terminate();
  }
  cout << "sendsuceess" << endl;
  //    char tempbuff[25600];
  //    status = recv(sock_fd, tempbuff, sizeof(tempbuff), MSG_WAITALL);
  //        if (status == -1) {
  //            cerr << "fail to receive message from server" << endl;
  //            exit(EXIT_FAILURE);
  //        }
  // receive content from server
  long block_size = 0;
  // first get the status line and response header
  while (1) {
    char tempbuff[256];
    status = recv(sock_fd, tempbuff, sizeof(tempbuff), 0);
    if (status == -1) {
      cerr << "fail to receive message from server" << endl;
      terminate();
    } else {
      if (status == 0)
        break;
      server_buff.append(tempbuff, status);
      block_size += status;
      cout << tempbuff;
      // what is wrong here?
      if (server_buff.find("\r\n\r\n") != string::npos)
        break;
      memset(tempbuff, 0, sizeof(tempbuff));
    }
  }
  // cout << server_buff.find("\r\n\r\n")<<endl;

  size_t findheader;
  int header_length = 0;
  if ((findheader = server_buff.find("Content-Length")) != string::npos) {
    // findheader = atoi(server_buff.substr(findheader+16, ))
    string tempstr = server_buff.substr(findheader + 16);
    header_length = atoi(tempstr.substr(0, tempstr.find("\r\n")).c_str());
  }
  // cout << header_length<<endl;
  //  cout << block_size;
  while (1) {
    char tempbuff[256];
    status = recv(sock_fd, tempbuff, sizeof(tempbuff), 0);
    if (status == -1) {
      cerr << "fail to receive message from server" << endl;
      terminate();
    } else {
      if (status == 0)
        break;
      server_buff.append(tempbuff, status);
      block_size += status;
      if (block_size >= header_length)
        break;
      memset(tempbuff, 0, sizeof(tempbuff));
    }
  }

  // deal with chunked data
  // else if((findheader = server_buff.find("Content-Length")) != string::npos)
  // cout <<server_buff;
  // server_buff.append(buff);
}
void proxyDaemon::responReq(int sock_fd) {
  int status;
  status = send(sock_fd, server_buff.c_str(), (size_t)server_buff.size(), 0);
  if (status == -1) {
    cerr << "fail to sendback to client" << endl;
    terminate();
  }
}

int proxyDaemon::createListenFd(char *port_n) {
  int listen_sofd;
  struct addrinfo host_info, *host_info_list;
  int status;

  // initialize the host_info with 0
  memset(&host_info, 0, sizeof(host_info));

  // set the socket type
  host_info.ai_family = AF_INET;
  host_info.ai_socktype = SOCK_STREAM;
  host_info.ai_flags = AI_PASSIVE;

  // get the proxy address information
  if ((status = getaddrinfo(NULL, port_n, &host_info, &host_info_list)) != 0) {
    cerr << "getaddrinfo for listen error:" << gai_strerror(status);
    exit(EXIT_FAILURE);
  }
  // make a socket for listen client
  listen_sofd = socket(host_info_list->ai_family, host_info_list->ai_socktype,
                       host_info_list->ai_protocol);

  if (listen_sofd == -1) {
    cerr << "Error: cannot create socket" << endl;
    exit(EXIT_FAILURE);
  }

  // begin listening
  if ((status = bind(listen_sofd, host_info_list->ai_addr,
                     host_info_list->ai_addrlen)) == -1) {
    cerr << "Error: cannot bind socket" << endl;
    exit(EXIT_FAILURE);
  }
  // status = listen(socket_fd, 100);
  // later change print error to throw exception
  if ((status = listen(listen_sofd, 100)) == -1) {
    cerr << "Error: cannot listen on socket" << endl;
    exit(EXIT_FAILURE);
  }

  return listen_sofd;
}

// implementation of proxymanager class
void proxymanager::runWithoutCache(char *port_n) {
  struct sockaddr_in client_addr;
  int listen_sofd = proxyDaemon::createListenFd(port_n);
  socklen_t addr_size = sizeof(client_addr);

  while (1) {
    int new_fd =
        accept(listen_sofd, (struct sockaddr *)(&client_addr), &addr_size);
    proxyDaemon::handleReq(new_fd);

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

// main function here
int main(int argc, char **argv) {
  if (argc != 2) {
    cerr << "Usage error: give me a port number" << endl;
    return -1;
  }

  proxymanager::runWithoutCache(argv[1]);
  return 1;
}
