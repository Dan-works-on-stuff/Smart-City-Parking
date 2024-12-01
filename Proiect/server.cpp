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

int main() {
    int socketfd=-1;
    create_socket(socketfd);
    bind_socket(socketfd, 55555);
    return 0;
}