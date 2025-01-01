#include "functions.h"

#define MAX_EVENTS 10000
#define ASSIGNER_PORT 55555
#define SERVER_PORT 55554
#define SHUTDOWN_SIGNAL "shutdown"

using namespace std;

mutex mtx; // Mutex for shared resources

void create_socket(int& assignersocket);
void bind_socket(int& assignersocket, int port);
void connect_socks(int &clientsocket, struct sockaddr_in &clientService);
void socket_listens(int& assignersocket);
void receive_data(int &acceptsocket, string& message);
void send_data(int &acceptsocket, const string &mesaj);
int setup_epoll(int assignersocket);
void handle_server(int epollfd, int assignersocket);
void handle_server_communication(int epollfd, int serversocket);
void listen_to_server (int assignersocket);
void handle_sensors(int epollfd, int sensorsocket);

int main() {
    int server_fd=-1;
    create_socket(server_fd);
    struct sockaddr_in serverAddress;
    connect_socks(server_fd, serverAddress);

    int sensorsocketfd = -1;
    create_socket(sensorsocketfd);
    bind_socket(sensorsocketfd, ASSIGNER_PORT);
    socket_listens(sensorsocketfd);

    thread server_listener(handle_server, server_fd);
    thread sensor_listener(handle_sensors, sensorsocketfd);
}


void connect_socks(int &clientsocket, struct sockaddr_in &clientService) {
    clientService.sin_family=AF_INET;
    inet_pton(AF_INET, "127.0.0.1", &clientService.sin_addr);  //informatiile pentru a conecta socketurile intre ele
    clientService.sin_port=htons(55554);      //htons e o functie pentru a pune bitii in ordinea corecta pe care o poate intelege serverul
    if (connect(clientsocket, (struct sockaddr *)&clientService, sizeof(clientService))==-1) {
        cerr<<"Error with the connection(): "<< strerror(errno)<<endl;
        close(clientsocket);
        exit(1);
    }
    cout<<"FloorMaster: connect() is OK."<<endl;
    cout<<"FloorMaster: Can start sending and receiving data..."<<endl;
}

void socket_listens(int& FloorMasterSocket) {
    if (listen(FloorMasterSocket, 2) == -1) {
        cerr << "listen() failed: " << strerror(errno) << endl;
        close(FloorMasterSocket);
        exit(1);
    }
    cout << "listen() OK, waiting for connections..." << endl;
}

int setup_epoll(int FloorMasterSocket) {
    int epollfd = epoll_create1(0);
    if (epollfd == -1) {
        cerr << "epoll_create1() failed: " << strerror(errno) << endl;
        exit(1);
    }

    struct epoll_event ev = {};
    ev.events = EPOLLIN;
    ev.data.fd = FloorMasterSocket;

    if (epoll_ctl(epollfd, EPOLL_CTL_ADD, FloorMasterSocket, &ev) == -1) {
        cerr << "epoll_ctl() failed: " << strerror(errno) << endl;
        close(epollfd);
        exit(1);
    }

    return epollfd;
}