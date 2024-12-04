//
// Created by maxcox on 12/4/24.
//

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

void create_socket(int& clientsocket) {
    clientsocket=socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (clientsocket == -1) {
        cerr<<"Error at socket()"<<strerror(errno)<<endl;
        exit(1);
    }
    else cout<<"socket() is ok"<<endl;
    //close(serversocket);
}

void connect_socks(int &clientsocket, struct sockaddr_in &clientService) {
    clientService.sin_family=AF_INET;
    inet_pton(AF_INET, "127.0.0.1", &clientService.sin_addr);  //informatiile pentru a conecta socketurile intre ele
    clientService.sin_port=htons(55555);      //htons e o functie pentru a pune bitii in ordinea corecta pe care o poate intelege serverul
    if (connect(clientsocket, (struct sockaddr *)&clientService, sizeof(clientService))==-1) {
        cerr<<"error with the connection(): "<< strerror(errno)<<endl;
        close(clientsocket);
        exit(1);
    }
    cout<<"Client: connect() is OK."<<endl;
    cout<<"Client: Can start sending and receiving data..."<<endl;
}

// void receive_data(int &clientsocket, struct sockaddr_in &clientService) {
//   char buffer[1024];
//   int bytes_received=recv(clientsocket, buffer, sizeof(buffer), 0);
//   if (bytes_received==-1) {
//     cerr<<"Error with the connection(): "<< strerror(errno)<<endl;
//     //close(clientsocket);
//   }
// }
void send_data(int &clientsocket, const string &mesaj) {
    const char* buffer = mesaj.c_str();
    size_t buffer_len = mesaj.length();
    int bytes_sent= send(clientsocket, buffer, buffer_len, 0);
    if (bytes_sent ==-1) {
        cerr<<"send() failed: "<< strerror(errno)<<endl;
    }
    else cout<<"Client: sent "<< bytes_sent << " bytes"<<endl;
}


int main() {
    int client_socket=-1;
    create_socket(client_socket);
    struct sockaddr_in client_service;
    connect_socks(client_socket, client_service);
    string message;
    cout<<"Enter message to send..."<<endl;
    getline(cin, message);
    send_data(acceptsocketfd,message);
    return 0;
}
