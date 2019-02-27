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
#include <arpa/inet.h>
#include <unistd.h>
#include <ctime>
#include <vector>
using namespace std;
#define CONNECTBUFFSIZE 1
#define BUFFSIZE 300
#define mylove
// a heap lock
pthread_rwlock_t heaplock = PTHREAD_RWLOCK_INITIALIZER;
// implementation of proxyDaemon class
void proxyDaemon::handleReq(int* sock_fd) {
  pthread_t thread;
  pthread_create(&thread, NULL, proxyDaemon::acceptReq, (void *)sock_fd);
}

/* create a proxy object to receive http req, then parse
 * and send req to target server and then
 * return response
 */
void *proxyDaemon::acceptReq(void *client_fd) {
  proxyDaemon pd;
  int status;
  char method[4];
  status = recv(*(int *)client_fd, method, sizeof(method) - sizeof(char), 0);
  if (status == -1) {
    cerr << "fail to get http request from client" << endl;
    close(*(int *)client_fd);
    delete (int*) client_fd;
    return NULL;
  }
  method[3] = '\0';
  // if GET
  if (strcmp(method, "GET") == 0) {
    pd.recvGET(*(int *)client_fd);
  }
  // if POST
  else if (strcmp(method, "POS") == 0) {
    pd.recvPOST(*(int *)client_fd);
  }
  // if CONNECT
  else if(strcmp(method,"CON") == 0){
      pd.recvCONNECT(*(int *)client_fd);
  }
  else{
      cout << "Not support " << method <<endl;
      close(*(int *)client_fd);
      delete (int*) client_fd;
      return NULL;
  }
  // parse the http message
  if (pd.parseReq() == -1) {
    close(*(int *)client_fd);
    delete (int*) client_fd;
    return NULL;
  }

  int server_fd = pd.conToServer();
  if(server_fd == -1){
    close(*(int *)client_fd);
    delete (int*) client_fd;
    return NULL;
  }
  if(pd.myreqline.method == "CONNECT") {
    pd.ssresponReq(*(int *) client_fd, server_fd);
  }
  else {
    pd.responReq(*(int *) client_fd, server_fd);
  }
  close(server_fd);
  close(*(int *)client_fd);
  delete (int*) client_fd;
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
  recvHTTP<false>(sock_fd, client_buff,0, 0);
}

/* receive all the contents from the client*/
template <bool flag>
void proxyDaemon::recvHTTP(int sock_fd,string& recvbuff, int noncontentsize,
                           int content_length) {
  int status;
  while (1) {
      char tempbuff[BUFFSIZE];

    status = recv(sock_fd, tempbuff, sizeof(tempbuff), 0);
    if (status == -1) {
      cerr << "fail to receive message" << endl;
      close(sock_fd);
      // exit(EXIT_FAILURE);
      terminate();
      //pthread_exit((void*) 0);
    } else {
      recvbuff.append(tempbuff, status);
      if (flag) {
        if (status == 0) {
          break;
        }
        if (recvbuff.size() - noncontentsize>= content_length) {
          break;
        }
      }

      if (!flag && recvbuff.find("\r\n\r\n") != string::npos) {
        break;
      }
      //memset(tempbuff, 0, sizeof(tempbuff));
    }
  }
  cout << recvbuff << endl;
}
void proxyDaemon::recvSSLHTTP(int sock_fd, string& recvbuff){
  int status;
  while (1) {
    char tempbuff;
    status = recv(sock_fd, &tempbuff, sizeof(tempbuff), 0);
    if (status == -1) {
      cerr << "fail to receive message from client" << endl;
      // exit(EXIT_FAILURE);
      //terminate();
      close(sock_fd);
      terminate();
      //pthread_exit((void*) 0);
    } else {
        recvbuff.push_back(tempbuff);
      if (status == 0) {
        break;
      }
      if (recvbuff.find("\r\n\r\n") != string::npos) {
        break;
      }
    }
  }
  cout << client_buff<<endl;
}

/* handle request with method 'POST'
 * get the http request from client
 */
void proxyDaemon::recvPOST(int sock_fd) {
  client_buff.append("POS");
  recvHTTP<false>(sock_fd, client_buff,0, 0);
  //recvSSLHTTP(sock_fd,server_buff);
  size_t findlength;
  int content_length = 0;
  if ((findlength = client_buff.find("Content-Length")) != string::npos) {
    // findheader = atoi(server_buff.substr(findheader+16, ))
    string tempstr = client_buff.substr(findlength + 16);
    content_length = atoi(tempstr.substr(0, tempstr.find("\r\n")).c_str());
    //content_length = octToDec(content_length);
    int noncontentsize =
        client_buff.substr(0, client_buff.find("\r\n\r\n") + 4).size();
    recvHTTP<true>(sock_fd, client_buff,noncontentsize, content_length);
  } else {
    cerr << "wrong POST form" << endl;
    close(sock_fd);
    terminate();
    //pthread_exit((void*) 0);
  }
  cout << client_buff << endl;
  cout << "POST SUCESS" << endl;
}

void proxyDaemon::recvCONNECT(int sock_fd){
  client_buff.append("CON");
  recvSSLHTTP(sock_fd, client_buff);
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
  //string header = temp.substr(sepepoint, temp.find("\r\n\r\n") - sepepoint);
  string header = temp.substr(sepepoint);
  parsereqhead(header);
  return 1;
}
void proxyDaemon::parsereqline(string & reqline) {
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
    //pthread_exit((void*) 0);
  }
  string tempsubstr = reqhead.substr(hostpoint);
  myreqheader["Host"] =
      tempsubstr.substr(6, tempsubstr.find("\r\n") - 6);
}
int proxyDaemon::conToServer() {
  // make a socket connecting with server
  int sock_fd;
  int status;
  struct addrinfo host_info, *host_info_list;
  memset(&host_info, 0, sizeof(host_info));

  host_info.ai_family = AF_INET;
  host_info.ai_socktype = SOCK_STREAM;

  // get the host address information
  pthread_rwlock_wrlock(&heaplock);
  string port = "80";
  size_t findport;
  if ((findport = myreqheader["Host"].find(":")) != string::npos) {
    port = myreqheader["Host"].substr(findport + 1);
    myreqheader["Host"].erase(findport);
  }

  status = getaddrinfo(myreqheader["Host"].c_str(), port.c_str(), &host_info,
                       &host_info_list);
  //create a smart pointer
  //unique_ptr<struct addrinfo> sptr(host_info_list);
  if (status != 0) {
    perror("Error: cannot get server address\n");
    // exit(EXIT_FAILURE);
    //terminate();
    //pthread_exit((void*) 0);
    return -1;
  }
  sock_fd = socket(host_info_list->ai_family, host_info_list->ai_socktype,
                   host_info_list->ai_protocol);
  if (sock_fd == -1) {
    cerr << "Error: cannot create socket to connect with server" << endl;
    //    exit(EXIT_FAILURE);
    //terminate();
    //pthread_exit((void*) 0);
    return -1;
  }
  // connect with server
  status =
      connect(sock_fd, host_info_list->ai_addr, host_info_list->ai_addrlen);
  pthread_rwlock_unlock(&heaplock);
  if (status == -1) {
    cerr << "fail to connect with server" << endl;
    //terminate();
    //pthread_exit((void*) 0);
    return -1;
  }
  freeaddrinfo(host_info_list);
  return sock_fd;
  // deal with chunked data
  // else if((findheader = server_buff.find("Content-Length")) != string::npos)
  // cout <<server_buff;
  // server_buff.append(buff);
}
void proxyDaemon::ssresponReq(int client_fd, int server_fd) {
  int status;
  fd_set master; //master  fd
  fd_set read_fds; // temperate read list
  int fdmax=0; // maximum file descriptor number
  FD_ZERO(&master);    // clear the master and temp sets
  FD_ZERO(&read_fds);
  //send 200 OK to client
  char* okbuff = (char *)"HTTP/1.1 200 OK\r\n\r\n";
  status = send(client_fd,okbuff, 40, 0);
  if(status == -1){
    cerr << "fails to send to client"<<endl;
    //terminate();
    close(client_fd);
    close(server_fd);
    terminate();
    //pthread_exit((void*) 0);
  }
//  status = send(server_fd,client_buff.c_str(),(size_t)client_buff.size(),0);
//  if(status == -1){
//    cerr << "fails to send to server"<<endl;
//    terminate();
//  }
  cout << okbuff<<endl;
  FD_SET(client_fd, &master); // add to master set
  if (client_fd > fdmax) {
    fdmax = client_fd;
  }
  FD_SET(server_fd, &master); // add to master set
  if (server_fd > fdmax) {
    fdmax = server_fd;
  }
  while(1) {
    read_fds = master;
    if (select(fdmax + 1, &read_fds, NULL, NULL, NULL) == -1) {
       perror("select");
       //exit(4);
       //terminate();
      close(client_fd);
      close(server_fd);
      terminate();
      //pthread_exit((void*) 0);
      }

      if(FD_ISSET(client_fd, & read_fds)){
        if(selectRecv(client_fd,server_fd) == 0) {
          return;
        }
      }
      if(FD_ISSET(server_fd, & read_fds)){
        if(selectRecv(server_fd,client_fd)){
          return;
        }
      }
  }
}


<<<<<<< HEAD
void proxyDaemon::selectRecv(int recv_fd, int send_fd) {
  char tempbuff[500];
=======
int proxyDaemon::selectRecv(int recv_fd, int send_fd) {
  char tempbuff[5000];
>>>>>>> 6e68c7c3817f3159ddc2f949159c034952d9dcc0
  int status;
  status = recv(recv_fd, tempbuff, sizeof(tempbuff), 0);
  if (status < 0) {
    if (status == 0) {
      close(recv_fd);
      close(send_fd);
      return 0;
      //terminate();
      //pthread_exit((void*) 0);
//    } else {
//      cerr << "some wrong happen during SSL" << endl;
//      close(recv_fd);
//      close(send_fd);
//      terminate();
      //pthread_exit((void*) 0);
    }
  }
  status = send(send_fd, tempbuff, status, 0);
  return 1;
}
void proxyDaemon::responReq(int client_fd, int server_fd) {
  int status;

  // string httpToserver = stickytogether();
  status = send(server_fd, client_buff.c_str(), (size_t)client_buff.size(), 0);
  if (status == -1) {
    cerr << "fail to send message to server" << endl;
    close(client_fd);
    close(server_fd);
    terminate();
    //pthread_exit((void*) 0);
  }
  cout << "sendsuceess" << endl;
  //    char tempbuff[25600];
  //    status = recv(client_fd, tempbuff, sizeof(tempbuff), MSG_WAITALL);
  //        if (status == -1) {
  //            cerr << "fail to receive message from server" << endl;
  //            exit(EXIT_FAILURE);
  //        }
  // receive content from server
//  long block_size = 0;
//  // first get the status line and response header
//  while (1) {
//    char tempbuff[BUFFSIZE];
//    status = recv(server_fd, tempbuff, sizeof(tempbuff), 0);
//    if (status == -1) {
//      cerr << "fail to receive message from server" << endl;
//      close(client_fd);
//      close(server_fd);
//      terminate();
//    } else {
//      if (status == 0)
//        break;
//      server_buff.append(tempbuff, status);
//      block_size += status;
//      cout << tempbuff;
//      // what is wrong here?
//      if (server_buff.find("\r\n\r\n") != string::npos)
//        break;
//      memset(tempbuff, 0, sizeof(tempbuff));
//    }
//  }
  // cout << server_buff.find("\r\n\r\n")<<endl;
  //recvHTTP<false>(server_fd, server_buff,0, 0);
  recvSSLHTTP(server_fd, server_buff);
  size_t findlength;
  int content_length = 0;
  if ((findlength = server_buff.find("Content-Length")) != string::npos) {
    // findheader = atoi(server_buff.substr(findheader+16, ))
    string tempstr = server_buff.substr(findlength + 16);
    content_length = atoi(tempstr.substr(0, tempstr.find("\r\n")).c_str());
    //content_length = octToDec(content_length);
    int noncontentsize =
            server_buff.substr(0, server_buff.find("\r\n\r\n") + 4).size();
    recvHTTP<true>(server_fd,server_buff, noncontentsize, content_length);
    status = send(client_fd, server_buff.c_str(), (size_t)server_buff.size(), 0);
    if (status == -1) {
      cerr << "fail to sendback to client" << endl;
      //terminate();
      close(client_fd);
      close(server_fd);
      terminate();
      //pthread_exit((void*) 0);
    }
  } else if((findlength = server_buff.find("Transfer-Encoding: chunked")) != string::npos){
    status = send(client_fd, server_buff.c_str(), (size_t)server_buff.size(), 0);
    if (status == -1) {
      cerr << "fail to sendback to client" << endl;
      //terminate();
      close(client_fd);
      close(server_fd);
      terminate();
      //pthread_exit((void*) 0);
    }
    recvChunked(server_fd,client_fd);
    return;
  }
  else{
    cerr << "wrong form from server" << endl;
  }
//  if ((findheader = server_buff.find("Content-Length")) != string::npos) {
//    // findheader = atoi(server_buff.substr(findheader+16, ))
//    string tempstr = server_buff.substr(findheader + 16);
//    header_length = atoi(tempstr.substr(0, tempstr.find("\r\n")).c_str());
//  }
//  // cout << header_length<<endl;
//  //  cout << block_size;
//  while (1) {
//    char tempbuff[BUFFSIZE];
//    status = recv(server_fd, tempbuff, sizeof(tempbuff), 0);
//    if (status == -1) {
//      cerr << "fail to receive message from server" << endl;
//      close(client_fd);
//      close(server_fd);
//      terminate();
//    } else {
//      if (status == 0)
//        break;
//      server_buff.append(tempbuff, status);
//      block_size += status;
//      if (block_size >= header_length)
//        break;
//      memset(tempbuff, 0, sizeof(tempbuff));
//    }
//  }
  // deal with chunked data
  // else if((findheader = server_buff.find("Content-Length")) != string::npos)


}


int proxyDaemon::recvChunkedsize(int sock_fd, string& chunkedstr) {
  string tempcount;
  int status;
  int result;
  while(1) {
    char tempbuff;
    status = recv(sock_fd, &tempbuff, sizeof(tempbuff), 0);
    if(status < 0){
      cerr << "error in recving chunked size"<<endl;
      return -1;
    }
    tempcount.push_back(tempbuff);
    if(tempcount.find("\r\n")!=string::npos){
      result = atoi(tempcount.c_str());
      cout << tempcount;
      chunkedstr.append(tempcount);
      //result = HexToDec(result);
      break;
    }
  }
  return result;
}
void proxyDaemon::recvChunkedbody(int sock_fd, string& chunkedstr ) {
  string tempstr;
  //int count = 0;
  int status;
  while (1) {
    char tempbuff;
    status = recv(sock_fd, &tempbuff, sizeof(tempbuff), 0);
    if (status < 0) {
      cerr << "error in recving chunked" << endl;
      return;
    }
    //out += status;
    tempstr.push_back(tempbuff);
    if (tempstr.find("\r\n") != string::npos) {
      chunkedstr.append(tempstr);
      break;
    }

  }
}
void proxyDaemon::recvChunked(int server_fd, int client_fd){
    int status;
   // char tempbuff;
    while(1) {
      string chunkedstr;
      int chunkedlen = recvChunkedsize(server_fd,chunkedstr);
      if(chunkedlen == 0) {
        chunkedstr.append("\r\n");
      }
      else{
        recvChunkedbody(server_fd,chunkedstr);
      }
      status = send(client_fd, chunkedstr.c_str(), (size_t)chunkedstr.size(), 0);
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
    freeaddrinfo(host_info_list);
    exit(EXIT_FAILURE);
  }
  // make a socket for listen client
  listen_sofd = socket(host_info_list->ai_family, host_info_list->ai_socktype,
                       host_info_list->ai_protocol);

  if (listen_sofd == -1) {
    cerr << "Error: cannot create socket" << endl;
    freeaddrinfo(host_info_list);
    exit(EXIT_FAILURE);
  }

  // begin listening
  if ((status = bind(listen_sofd, host_info_list->ai_addr,host_info_list->ai_addrlen)) == -1) {
    cerr << "Error: cannot bind socket" << endl;
    freeaddrinfo(host_info_list);
    exit(EXIT_FAILURE);
  }
  // status = listen(socket_fd, 100);
  // later change print error to throw exception
  if ((status = listen(listen_sofd, 100)) == -1) {
    cerr << "Error: cannot listen on socket" << endl;
    freeaddrinfo(host_info_list);
    exit(EXIT_FAILURE);
  }
  freeaddrinfo(host_info_list);
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
    int * sock_fd = new int(new_fd);
    proxyDaemon::handleReq(sock_fd);

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
