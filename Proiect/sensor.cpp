#include "functions.h"

using namespace std;

void create_socket(int& serversocket);

void connect_to_assigner(int &sensorsocket, struct sockaddr_in &sensorService) {
    sensorService.sin_family=AF_INET;
    inet_pton(AF_INET, "127.0.0.1", &sensorService.sin_addr);  //informatiile pentru a conecta socketurile intre ele
    sensorService.sin_port=htons(55555);      //htons e o functie pentru a pune bitii in ordinea corecta pe care o poate intelege serverul
    if (connect(sensorsocket, (struct sockaddr *)&sensorService, sizeof(sensorService))==-1) {
        cerr<<"error with the connection(): "<< strerror(errno)<<endl;
        close(sensorsocket);
        exit(1);
    }
    cout<<"sensor: connect() is OK."<<endl;
}

void connect_to_FloorMaster(int &sensorsocket, struct sockaddr_in &sensorService, int port) {
    sensorService.sin_family=AF_INET;
    inet_pton(AF_INET, "127.0.0.1", &sensorService.sin_addr);  //informatiile pentru a conecta socketurile intre ele
    sensorService.sin_port=htons(port);      //htons e o functie pentru a pune bitii in ordinea corecta pe care o poate intelege serverul
    if (connect(sensorsocket, (struct sockaddr *)&sensorService, sizeof(sensorService))==-1) {
        cerr<<"error with the connection(): "<< strerror(errno)<<endl;
        close(sensorsocket);
        exit(1);
    }
    cout<<"sensor: connect() is OK."<<endl;
}

void send_data(int &sensorsocket, const string &mesaj) {
    const char* buffer = mesaj.c_str();
    size_t buffer_len = mesaj.length();
    int bytes_sent= send(sensorsocket, buffer, buffer_len, 0);
    if (bytes_sent ==-1) {
        cerr<<"send() failed: "<< strerror(errno)<<endl;
    }
    else cout<<"sensor: sent "<< bytes_sent << " bytes"<<endl;
}

void receive_data(int &sensorsocket, string& message) {
  char buffer[200];
  int bytes_received=recv(sensorsocket, buffer, sizeof(buffer), 0);
  if (bytes_received<0) {
        cerr<<"Error with the connection(): "<< strerror(errno)<<endl;
        exit(1);
  }
  else if (bytes_received==0) {
      cout<<"Server disconnected."<<endl;
      close(sensorsocket);
      exit(1);
  }
  else{
      buffer[bytes_received]='\0';
      message = buffer;
      cout<<"received data: "<<message<<endl;
  }
}

void handle_user_input(int sensorsocket) {
    string message;
    cout<<"Enter message to send: "<<endl;
    getline(cin, message);
    send_data(sensorsocket, message);
    if (message=="pa") {
        cout<<"sensor requested to end the chat"<<endl;
        close(sensorsocket);
        exit(0);
    }
}

int setup_epoll(int sensorsocket) {
    int epollfd = epoll_create1(0);
    if (epollfd==-1) {
        cerr<<"epoll_create1() failed: "<< strerror(errno)<<endl;
        exit(1);
    }
    struct epoll_event ev;
    ev.events = EPOLLIN | EPOLLOUT;
    ev.data.fd = sensorsocket;
    if (epoll_ctl(epollfd,EPOLL_CTL_ADD, sensorsocket, &ev)==-1) {
        cerr<<"epoll_ctl() failed: "<<strerror(errno)<<endl;
        close(epollfd);
        exit(1);
    }
    return epollfd;
}

int stringToInt(const string& str) {
    int floor_nr=-1;
    if (str.length() == 1 && str[0] >= 'A' && str[0] <= 'Z') {
        floor_nr= floor_nr * 26 + (str[0] - 'A');
    }
    return floor_nr;
}

int main() {
    int sensor_socket=-1;
    create_socket(sensor_socket);
    struct sockaddr_in sensor_service;
    connect_to_assigner(sensor_socket, sensor_service);
    int level=-1;
    cin>>level;
    send_data(sensor_socket, to_string(level));
    string message;
    receive_data(sensor_socket, message);
    close(sensor_socket);
    create_socket(sensor_socket);
    connect_to_FloorMaster(sensor_socket, sensor_service, stringToInt(message));
    int epollfd =setup_epoll(sensor_socket);
    const int MAX_EVENTS=1;
    struct epoll_event events[MAX_EVENTS];
    while (true) {
        int nfds = epoll_wait(epollfd, events, MAX_EVENTS, -1);
        if (nfds==-1) {
            cerr<<"epoll_wait() failed: "<<strerror(errno)<<endl;
            break;
        }
        if (nfds>0) {
            handle_user_input(sensor_socket);
        }
    }
    close(epollfd);
    close(sensor_socket);
    return 0;
}
