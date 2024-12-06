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
using namespace std;

const int MAX_ETAJE = 10;

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

void socket_listens(int& serversocket) {
    if (listen(serversocket,MAX_ETAJE)==-1) {  //am ales 10 clienti reprezentand un numar de 10 etaje teoretice posibile ale unei parcari
        cerr<<"listen():Error listening to socket "<< strerror(errno)<<endl;
        close(serversocket);
        exit(1);
    }
    else cout<<"listen() OK, waitig for connections..."<<endl;
}

//accept(socket, struct sockaddr* addr, int* addrlen)  sockaddr* addr este adresa clientului, e folositor daca vrei sa accepti informatii doar de la un client anume.
                                                     //int* addrlen size of the address structure ^^
//RETURNS A VALUE OF TYPE INT!! mai exact un ALT SOCKET, o DUBLURA cu acelasi PORT si IP_adress cu care comunica explicit cu clientul desemnat.

//de modificat: in acest moment serverul NU este concurent, networkingul se face sincron

void accept_socket(int& serversocket, int& acceptsocket) {
    acceptsocket=accept(serversocket, NULL, NULL);
    if (acceptsocket==-1) {
        cerr<<"accept() failed: "<< strerror(errno)<<endl;
        exit(1);
    }
    else cout<<"se accepta()"<<endl;
}

void receive_data(int &acceptsocket, struct sockaddr_in &serverService, string& message) {
    char buffer[200];
    int bytes_received = recv(acceptsocket, buffer, sizeof(buffer), 0);
    if (bytes_received<0) {
        cerr<<"Error with the connection(): "<< strerror(errno)<<endl;
        exit(1);
    }
    else if (bytes_received==0) {
        cout<<"client disconnected"<<endl;
        close(acceptsocket);
    }
    else{
        buffer[bytes_received]='\0';
        message = buffer;
        cout<<"received data: "<<message<<endl;
    }
}

void send_data(int &acceptsocket, const string &mesaj) {
    const char* buffer = mesaj.c_str();
    size_t buffer_len = mesaj.length();
    int bytes_sent= send(acceptsocket, buffer, buffer_len, 0);
    if (bytes_sent ==-1) {
        cerr<<"send() failed: "<< strerror(errno)<<endl;
    }
    else cout<<"Server: sent "<< bytes_sent << " bytes"<<endl;
}

void handle_client(int epollfd, int serversocket) {
    struct epoll_event ev;
    int clientsocket=accept(serversocket, NULL, NULL);
    if (clientsocket==-1) {
        cerr<<"accept() failed: "<<strerror(errno)<<endl;
        return;
    }
    ev.events=EPOLLIN;
    ev.data.fd=clientsocket;
    if (epoll_ctl(epollfd, EPOLL_CTL_ADD, clientsocket, &ev)==-1) {
        cerr<<"epoll_ctl() failed"<<strerror(errno)<<endl;
        close(clientsocket);
        return;
    }
    cout<<"New client connected"<<endl;
}

void handle_communication(int epollfd, int clientsocket) {
    struct sockaddr_in server_service;
    string message;
    receive_data(clientsocket, server_service, message);
    if (message=="pa") {
        cout<<"client requested to end the chat."<<endl;
        close(clientsocket);
        exit(0);
    }
    cout<<"enter message to be sent: "<<endl;
    getline(cin, message);
    send_data(clientsocket, message);
    if (message=="pa") {
        cout<<"Server requested to end the chat."<<endl;
        close(clientsocket);
        exit(0);
    }
}

int setup_epoll(int serversocket) {
    int epollfd=epoll_create1(0);
    if (epollfd==-1) {
        cerr<<"epoll_create1() failed "<< strerror(errno)<<endl;
        exit(1);
    }
    struct epoll_event ev;
    ev.events = EPOLLIN;
    ev.data.fd=serversocket;
    if (epoll_ctl(epollfd, EPOLL_CTL_ADD, serversocket, &ev)==-1) {
        cerr<<"epoll_ctl() failed: "<<strerror(errno)<<endl;
        close(epollfd);
        exit(1);
    }
    return epollfd;
}


int main() {
    int serversocketfd=-1;
    create_socket(serversocketfd);
    bind_socket(serversocketfd, 55555);
    socket_listens(serversocketfd);
    // int acceptsocketfd=-1;
    // accept_socket(socketfd,acceptsocketfd);
    // struct sockaddr_in server_service;
    // string mesaj_primit;
    // string mesaj;
    // do {
    //     receive_data(acceptsocketfd, server_service, mesaj_primit);
    //     cout << "Received message: " << mesaj_primit << endl;
    //     if (mesaj_primit == "pa") {
    //         cout << "Client requested to end the chat." << endl;
    //         break;
    //     }
    //                                                                              implementare facuta pt un server sincron, nu asincron
    //     cout << "Enter message to send: " << endl;
    //     getline(cin, mesaj);
    //     send_data(acceptsocketfd, mesaj);
    //     if (mesaj == "pa") {
    //         cout << "Server requested to end the chat." << endl;
    //         break;
    //     }
    // } while (true);
    // close(acceptsocketfd);
    // close(socketfd);
    int epollfd=setup_epoll(serversocketfd);

    struct epoll_event etaje[MAX_ETAJE];
    while (true) {
        int nfds=epoll_wait(epollfd, etaje, MAX_ETAJE, -1);
        if (nfds==-1) {
            cerr<<"epoll_wait() failed: "<<strerror(errno)<<endl;
            close(epollfd);
            close(serversocketfd);
            exit(1);
        }
        for (int i=0; i<nfds; i++) {
            if (etaje[i].data.fd==serversocketfd) {
                handle_client(epollfd, serversocketfd);
            }
            else handle_communication(epollfd, etaje[i].data.fd);
        }
    }
    close(epollfd);
    close(serversocketfd);
    return 0;
}