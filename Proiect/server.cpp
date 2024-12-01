#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <netinet/tcp.h>
#include <cstdio>
#include<cstring>
#include <string>
#include <cerrno>
#include <sys/select.h>
#include <sys/time.h>
#include <unistd.h>
using namespace std;

void create_socket(int& serversocket) {
    serversocket=socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (serversocket == -1) {
        cerr<<"Error at socket()"<<strerror(errno)<<endl;
        exit(1);
    }
    else cout<<"socket() is ok"<<endl;
    //close(serversocket);
}

void bind_socket(int& serversocket, int port) {
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr)); //asignez la toti bytes valoarea 0 pt ca am declarat structul local
    addr.sin_family = AF_INET; //IPv4, maybe switch to IPv6 later
    addr.sin_port = htons(port);
    inet_pton(AF_INET, "127.0.0.1", &addr.sin_addr); //setup pentru test local 127.0.0.1 = adresa masinii (LOOPBACK ADDRESS)
    if (bind(serversocket, (struct sockaddr *)&addr,sizeof(addr))==-1) {    //bind() expects a pointer to a struct sockaddr
                                                                                //addr este pasat ca referinta pentru ca daca ar fi fost pasat sub forma de copie, orice
                                                                                // facuta la variabila addr nu s-ar fi reflectat si in functia bind si viceversa
                                                                                //struct sockaddr is a more general structure (works with IPv4 and could work with IPv6
        cerr<<"bind() failed: "<<strerror(errno)<<endl;
        close(serversocket);
        exit(1);
    }
    else cout<<"bind() OK"<<endl;
}

// listen(socket, int backlog); backlog=limita de clienti pt socket

void socket_listens(int& serversocket) {
    if (listen(serversocket,10)==-1) {  //am ales 10 clienti reprezentand un numar de 10 etaje teoretice posibile ale unei parcari
        cerr<<"listen():Error listening to socket "<< strerror(errno)<<endl;
        close(serversocket);
        exit(1);
    }
    else cout<<"listen() OK, waitig for connections..."<<endl;
}

//accept(socket, struct sockaddr* addr, int* addrlen)  sockaddr* addr este adresa clientului, e folositor daca vrei sa accepti informatii doar de la un client anume.
                                                     //int* addrlen size of the address structure ^^
//RETURNS A VALUE OF TYPE INT!! mai exact un ALT SOCKET, o DUBLURA cu acelasi PORT si IP_adress cu care comunica explicit cu clientul desemnat.

void accept_socket(int& serversocket, int& acceptsocket) {
    acceptsocket=accept(serversocket, NULL, NULL);
    if (acceptsocket==-1) {
        cerr<<"accept() failed: "<< strerror(errno)<<endl;
        exit(1);
    }
    else cout<<"se accepta()"<<endl;
}

int main() {
    int socketfd=-1;
    create_socket(socketfd);
    bind_socket(socketfd, 55555);
    socket_listens(socketfd);
    int acceptsocketfd=-1;
    accept_socket(socketfd,acceptsocketfd);
    return 0;
}