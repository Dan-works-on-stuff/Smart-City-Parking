#include "shared_functions.h"

using namespace std;

void create_socket(int& ServerSocket);
void connect_to_assigner(int &SensorSocket, struct sockaddr_in &sensorService);
void connect_to_FloorMaster(int &SensorSocket, struct sockaddr_in &sensorService, int port);
void send_data(int &SensorSocket, const string &mesaj);
void receive_data(int &AcceptSocket, string& message, string who);
void handle_user_input(int SensorSocket);
int setup_sensor_epoll(int SensorSocket);
int port_computer(const string& str);
string get_right_value(string& message);

int main() {
    int sensor_socket=-1;
    create_socket(sensor_socket);
    struct sockaddr_in sensor_service;
    connect_to_assigner(sensor_socket, sensor_service);
    string right_port;
    string level = get_right_value(right_port);
    send_data(sensor_socket, level);
    receive_data(sensor_socket, right_port, "Sensor");
    close(sensor_socket);
    create_socket(sensor_socket);
    connect_to_FloorMaster(sensor_socket, sensor_service, port_computer(right_port));
    int epollfd =setup_sensor_epoll(sensor_socket);
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


void connect_to_assigner(int &SensorSocket, struct sockaddr_in &sensorService) {
    sensorService.sin_family=AF_INET;
    inet_pton(AF_INET, "127.0.0.1", &sensorService.sin_addr);  //informatiile pentru a conecta socketurile intre ele
    sensorService.sin_port=htons(55555);      //htons e o functie pentru a pune bitii in ordinea corecta pe care o poate intelege serverul
    if (connect(SensorSocket, (struct sockaddr *)&sensorService, sizeof(sensorService))==-1) {
        cerr<<"error with the connection(): "<< strerror(errno)<<endl;
        close(SensorSocket);
        exit(1);
    }
    cout<<"sensor: connect() is OK."<<endl;
}

void connect_to_FloorMaster(int &SensorSocket, struct sockaddr_in &sensorService, int port) {
    sensorService.sin_family=AF_INET;
    inet_pton(AF_INET, "127.0.0.1", &sensorService.sin_addr);  //informatiile pentru a conecta socketurile intre ele
    sensorService.sin_port=htons(port);      //htons e o functie pentru a pune bitii in ordinea corecta pe care o poate intelege serverul
    if (connect(SensorSocket, (struct sockaddr *)&sensorService, sizeof(sensorService))==-1) {
        cerr<<"error with the connection(): "<< strerror(errno)<<endl;
        close(SensorSocket);
        exit(1);
    }
    cout<<"sensor: connect() is OK."<<endl;
}

void handle_user_input(int SensorSocket) {
    string message;
    cout<<"Enter message to send: "<<endl;
    getline(cin, message);
    send_data(SensorSocket, message);
    if (message=="pa") {
        cout<<"sensor requested to end the chat"<<endl;
        close(SensorSocket);
        exit(0);
    }
}

int setup_sensor_epoll(int SensorSocket) {
    int epollfd = epoll_create1(0);
    if (epollfd==-1) {
        cerr<<"epoll_create1() failed: "<< strerror(errno)<<endl;
        exit(1);
    }
    struct epoll_event ev;
    ev.events = EPOLLIN | EPOLLOUT;
    ev.data.fd = SensorSocket;
    if (epoll_ctl(epollfd,EPOLL_CTL_ADD, SensorSocket, &ev)==-1) {
        cerr<<"epoll_ctl() failed: "<<strerror(errno)<<endl;
        close(epollfd);
        exit(1);
    }
    return epollfd;
}

int port_computer(const string& str) {
    int port=0;
    for (char c: str) {
        port=port*10+(c-'0');
    }
    return port;
}

string get_right_value(string& message) {
    string level;
    do{
        cin >> level;
        try {
            int levelNum = stoi(level);
            if (levelNum >= 0 && levelNum <= 10) {
                return level;
            } else {
                cout << "Invalid input. Please enter a number between 0 and 10." << endl;
            }
        } catch (const std::invalid_argument& e) {
            cout << "Invalid input. Please enter a valid number." << endl;
            cin.clear(); // Clear error flags
            cin.ignore(numeric_limits<streamsize>::max(), '\n'); // Discard invalid input
        }
    }while (message.size()>5 || message.size()==0);
}