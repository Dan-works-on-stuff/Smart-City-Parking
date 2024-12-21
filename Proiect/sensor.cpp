#include "functions.h"


using namespace std;

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

void send_data(int &clientsocket, const string &mesaj) {
    const char* buffer = mesaj.c_str();
    size_t buffer_len = mesaj.length();
    int bytes_sent= send(clientsocket, buffer, buffer_len, 0);
    if (bytes_sent ==-1) {
        cerr<<"send() failed: "<< strerror(errno)<<endl;
    }
    else cout<<"Client: sent "<< bytes_sent << " bytes"<<endl;
}

void receive_data(int &clientsocket, string& message) {
  char buffer[200];
  int bytes_received=recv(clientsocket, buffer, sizeof(buffer), 0);
  if (bytes_received<0) {
        cerr<<"Error with the connection(): "<< strerror(errno)<<endl;
        exit(1);
  }
  else if (bytes_received==0) {
      cout<<"Server disconnected."<<endl;
      close(clientsocket);
      exit(1);
  }
  else{
      buffer[bytes_received]='\0';
      message = buffer;
      cout<<"received data: "<<message<<endl;
  }
}

void handle_user_input(int clientsocket) {
    string message;
    cout<<"Enter message to send: "<<endl;
    getline(cin, message);
    send_data(clientsocket, message);
    if (message=="pa") {
        cout<<"Client requested to end the chat"<<endl;
        close(clientsocket);
        exit(0);
    }
}

int setup_epoll(int clientsocket) {
    int epollfd = epoll_create1(0);
    if (epollfd==-1) {
        cerr<<"epoll_create1() failed: "<< strerror(errno)<<endl;
        exit(1);
    }
    struct epoll_event ev;
    ev.events = EPOLLIN | EPOLLOUT;
    ev.data.fd = clientsocket;
    if (epoll_ctl(epollfd,EPOLL_CTL_ADD, clientsocket, &ev)==-1) {
        cerr<<"epoll_ctl() failed: "<<strerror(errno)<<endl;
        close(epollfd);
        exit(1);
    }
    return epollfd;
}

void handle_server_data(int clientsocket) {
    string message;
    receive_data(clientsocket, message);
    if (message=="pa") {
        cout<<"Server requested to end the chat."<<endl;
        close(clientsocket);
        exit(0);
    }
}

int main() {
    int client_socket=-1;
    create_socket(client_socket);
    struct sockaddr_in client_service;
    connect_socks(client_socket, client_service);
    int epollfd =setup_epoll(client_socket);
    const int MAX_EVENTS=1;
    struct epoll_event events[MAX_EVENTS];
    while (true) {
        int nfds = epoll_wait(epollfd, events, MAX_EVENTS, -1);
        if (nfds==-1) {
            cerr<<"epoll_wait() failed: "<<strerror(errno)<<endl;
            break;
        }
        if (nfds>0) {
            handle_user_input(client_socket);
            handle_server_data(client_socket);
        }
    }
    close(epollfd);
    close(client_socket);
    return 0;
}
