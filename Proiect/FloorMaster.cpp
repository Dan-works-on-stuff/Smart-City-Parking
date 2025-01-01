#include "functions.h"

#define SERVER_PORT 55553
using namespace std;


ofstream g("parking.txt");
string level_letter;
int level_number;
const int MAX_PARCARI = 100;  // Maximum parking spots
vector<bool> sensors(MAX_PARCARI, false); // Sensor status
mutex mtx; // Mutex for shared resources
const int update_interval = 10; // Update interval in seconds

// Function declarations
void create_socket(int& FloorMasterSocket);
void bind_socket(int& FloorMasterSocket, int port);
void connect_socks(int &clientsocket, struct sockaddr_in &clientService);
void socket_listens(int& FloorMasterSocket);
void receive_data(int &acceptsocket, string& message);
void send_data(int &acceptsocket, const string &mesaj);
int setup_epoll(int FloorMasterSocket);
void handle_sensor(int epollfd, int FloorMasterSocket);
void handle_communication(int epollfd, int sensorsocket);
void listener_thread(int FloorMasterSocket);
void sender_thread(int serversocket);
bool update_parking_spots(const string &message);
void stringToInt(const std::string& str);


int main() {
    int server_socket=-1;
    create_socket(server_socket);
    struct sockaddr_in server_service;
    connect_socks(server_socket, server_service);

    stringToInt(level_letter);

    int FloorMasterSocketfd = -1;
    int port_for_sensors= 55556+level_number;
    create_socket(FloorMasterSocketfd);
    bind_socket(FloorMasterSocketfd, port_for_sensors);
    socket_listens(FloorMasterSocketfd);

    thread listener(listener_thread, FloorMasterSocketfd);
    thread sender(sender_thread, server_socket);
    listener.join();
    sender.join();
    return 0;
}

void connect_socks(int &clientsocket, struct sockaddr_in &clientService) {
    clientService.sin_family=AF_INET;
    inet_pton(AF_INET, "127.0.0.1", &clientService.sin_addr);  //informatiile pentru a conecta socketurile intre ele
    clientService.sin_port=htons(SERVER_PORT);      //htons e o functie pentru a pune bitii in ordinea corecta pe care o poate intelege serverul
    if (connect(clientsocket, (struct sockaddr *)&clientService, sizeof(clientService))==-1) {
        cerr<<"Error with the connection(): "<< strerror(errno)<<endl;
        close(clientsocket);
        exit(1);
    }
    cout<<"FloorMaster: connect() is OK."<<endl;
    cout<<"FloorMaster: Can start sending and receiving data..."<<endl;
    receive_data(clientsocket, level_letter);

}

// Function to set the socket to listen
void socket_listens(int& FloorMasterSocket) {
    if (listen(FloorMasterSocket, MAX_PARCARI) == -1) {
        cerr << "listen() failed: " << strerror(errno) << endl;
        close(FloorMasterSocket);
        exit(1);
    }
    cout << "listen() OK, waiting for connections..." << endl;
}

// Function to set up epoll
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

// Function to handle new sensor connections
void handle_sensor(int epollfd, int FloorMasterSocket) {
    int sensorsocket = accept(FloorMasterSocket, NULL, NULL);
    if (sensorsocket == -1) {
        cerr << "accept() failed: " << strerror(errno) << endl;
        return;
    }

    struct epoll_event ev = {};
    ev.events = EPOLLIN;
    ev.data.fd = sensorsocket;

    if (epoll_ctl(epollfd, EPOLL_CTL_ADD, sensorsocket, &ev) == -1) {
        cerr << "epoll_ctl() failed: " << strerror(errno) << endl;
        close(sensorsocket);
        return;
    }

    cout << "New sensor connected." << endl;
}

// Function to receive data
void receive_data(int &acceptsocket, string& message) {
    char buffer[200];
    int bytes_received = recv(acceptsocket, buffer, sizeof(buffer), 0);
    if (bytes_received <= 0) {
        if (bytes_received < 0) {
            cerr << "recv() failed: " << strerror(errno) << endl;
        } else {
            cout << "sensor disconnected." << endl;
        }
        close(acceptsocket);
        return;
    }

    buffer[bytes_received] = '\0';
    message = buffer;
    cout << "Received: " << message << endl;
}

// Function to send data
void send_data(int &acceptsocket, const string &mesaj) {
    const char* buffer = mesaj.c_str();
    size_t buffer_len = mesaj.length();
    int bytes_sent = send(acceptsocket, buffer, buffer_len, 0);
    if (bytes_sent == -1) {
        cerr << "send() failed: " << strerror(errno) << endl;
    }
    //  else {
    //     cout << "Sent: " << mesaj << endl;
    // }
}

// Function to process parking spot updates
bool update_parking_spots(const string &message) {
    lock_guard<mutex> lock(mtx);
    try {
        size_t pos = message.find(':');
        if (pos == string::npos) throw invalid_argument("Invalid message format");
        int spot = stoi(message.substr(0, pos));
        if (spot >= 0 && spot < sensors.size()) {
            if (message.substr(pos + 1)=="1" || message.substr(pos+1)=="0") {
                bool status = stoi(message.substr(pos + 1));
                sensors[spot] = status;
                cout << "Spot " << spot << " is now " << (status ? "occupied" : "free") << "." << endl;
                return 0;
            }
            else {
                cerr<<"Invalid status entered, the only possible 2 statuses are 1 for occupied and 0 for unnocupied"<<endl;
            }
        } else {
            cerr << "Invalid spot number." << endl;
        }
    } catch (const exception &e) {
        cerr << "Error processing message: " << e.what() << endl;
    }
    return 1;
}

// Function to handle communication with a sensor
void handle_communication(int epollfd, int sensorsocket) {
    string message;
    receive_data(sensorsocket, message);

    if (message.empty() || message=="pa") {
        return; // No message received or sensor disconnected
    }

    bool status=update_parking_spots(message);
    if (status==0)
        send_data(sensorsocket, "Parking spot updated.");
    else
        send_data(sensorsocket, "Error marking the parking spot");
}

// Listener thread to handle incoming connections and messages
void listener_thread(int FloorMasterSocket) {
    int epollfd = setup_epoll(FloorMasterSocket);
    struct epoll_event events[MAX_PARCARI];

    while (true) {
        int nfds = epoll_wait(epollfd, events, MAX_PARCARI, -1);
        if (nfds == -1) {
            cerr << "epoll_wait() failed: " << strerror(errno) << endl;
            continue; // Retry on error
        }

        for (int i = 0; i < nfds; i++) {
            if (events[i].data.fd == FloorMasterSocket) {
                handle_sensor(epollfd, FloorMasterSocket);
            } else {
                handle_communication(epollfd, events[i].data.fd);
            }
        }
    }
    close(epollfd);
    close(FloorMasterSocket);
}

void sender_thread(int serversocket) {
    while (true) {
        this_thread::sleep_for(chrono::seconds(update_interval));
        mtx.lock();
        if (!g.is_open()) {
            cerr<<"Erroare: Nu se poate accesa fisierul txt dorit. "<< strerror(errno)<<endl;
            continue;
        }
        // for (int i=0; i<sensors.size(); i++) {
        //     g<<sensors[i];
        // }
        // g<<endl<<endl;
        string message;
        for (int i=0; i<sensors.size(); i++) {
            message += sensors[i] ? '1' : '0';
        }
        message = level_letter + ": " + message;
        send_data(serversocket, message);
        mtx.unlock();
        //de implementat trimiterea catre server (sa ma uit cum se intampla in sensor)

    }
}

void send_data(int &serversocket, string mesaj) {
        const char* buffer = mesaj.c_str();
        size_t buffer_len = mesaj.length();
        int bytes_sent= send(serversocket, buffer, buffer_len, 0);
        if (bytes_sent ==-1) {
            cerr<<"send() failed: "<< strerror(errno)<<endl;
        }
        else cout<<"sensor: sent "<< bytes_sent << " bytes"<<endl;
}


void stringToInt(const string& str) {
    if (str.length() == 1 && str[0] >= 'A' && str[0] <= 'Z') {
        level_number = level_number * 26 + (str[0] - 'A');
    }
}