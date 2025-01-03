#ifndef FUNCTIONS_H
#define FUNCTIONS_H
#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <netinet/tcp.h>
#include <cstdio>
#include <cstring>
#include <string>
#include <cerrno>
#include <sys/select.h>
#include <sys/time.h>
#include <unistd.h>
#include <string>
#include <sys/epoll.h>
#include <vector>
#include<stdexcept>
#include <thread>
#include <mutex>
#include <fstream>
#include <atomic>
#include <csignal>
#include <algorithm>
using namespace std;

void create_socket(int& serversocket) {
    serversocket=socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (serversocket == -1) {
        cerr<<"Error at socket()"<<strerror(errno)<<endl;
        exit(1);
    }
    else cout<<"socket() is ok"<<endl;
    //close(serversocket);
    int opt = 1;
    if (setsockopt(serversocket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) == -1) {
        cerr << "setsockopt() failed: " << strerror(errno) << endl;
        close(serversocket);
        exit(1);
    }
}

void bind_socket(int& serversocket, int port) {
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr)); //asignez la toti bytes valoarea 0 pt ca am declarat structul local
    addr.sin_family = AF_INET; //IPv4, maybe switch to IPv6 later
    addr.sin_port = htons(port);
    inet_pton(AF_INET, "127.0.0.1", &addr.sin_addr); //setup pentru test local 127.0.0.1 = adresa masinii (LOOPBACK ADDRESS)
    if (bind(serversocket, (struct sockaddr *)&addr,sizeof(addr))==-1) {    //bind() expects a pointer to a struct sockaddr
        //addr este pasat ca referinta pentru ca daca ar fi fost pasat sub forma de copie, orice modificare
        // facuta la variabila addr nu s-ar fi reflectat si in functia bind si viceversa
        //struct sockaddr is a more general structure (works with IPv4 and could work with IPv6)
        cerr<<"bind() failed: "<<strerror(errno)<<endl;
        close(serversocket);
        exit(1);
    }
    else cout<<"bind() OK"<<endl;
}

// listen(socket, int backlog); backlog=limita de clienti pt socket
void socket_listens(int& serversocket, string who,int MAX_PARCARI) {
    if (listen(serversocket,MAX_PARCARI)==-1) {  //am ales 10 clienti reprezentand un numar de 10 etaje teoretice posibile ale unei parcari
        cerr<<"listen() error in "<<who<<' '<< strerror(errno)<<endl;
        close(serversocket);
        exit(1);
    }
    else cout<<"listen() OK in "<<who<<", waiting for connections..."<<endl;
}

void send_data(int &AcceptSocket, const string &mesaj) {
    const char* buffer = mesaj.c_str();
    size_t buffer_len = mesaj.length();
    int bytes_sent= send(AcceptSocket, buffer, buffer_len, 0);
    if (bytes_sent ==-1) {
        cerr<<"send() failed: "<< strerror(errno)<<endl;
    }
    else cout<<"Server: sent "<< bytes_sent << " bytes ("<<mesaj<<')'<<endl;
}

void receive_data(int &AcceptSocket, string& message, string who) {
    char buffer[200];
    int bytes_received = recv(AcceptSocket, buffer, sizeof(buffer), 0);
    if (bytes_received<0) {
        cerr<<"Error with the connection() in"<<who<<": "<< strerror(errno)<<endl;
        return;
    }
    if (bytes_received==0) {
        cout<<"client disconnected"<<endl;
        close(AcceptSocket);
    }
    else{
        buffer[bytes_received]='\0';
        message = buffer;
        cout<<"received data: "<<message<<endl;
    }
}

int setup_epoll(int socket) {
    int epollfd=epoll_create1(0);
    if (epollfd==-1) {
        cerr<<"epoll_create1() failed "<< strerror(errno)<<endl;
        exit(1);
    }
    struct epoll_event ev;
    ev.events = EPOLLIN;
    ev.data.fd=socket;
    if (epoll_ctl(epollfd, EPOLL_CTL_ADD, socket, &ev)==-1) {
        cerr<<"epoll_ctl() failed: "<<strerror(errno)<<endl;
        close(epollfd);
        exit(1);
    }
    return epollfd;
}
#endif //FUNCTIONS_H



//SOCKET CREATION!!
//For the creator: create(), bind(), listen(), accept()
//For the other party: create(), connect()