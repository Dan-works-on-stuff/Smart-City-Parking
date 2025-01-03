#include "shared_functions.h"

#define MAX_SENSORS 10000
#define ASSIGNER_PORT 55555
#define SERVER_PORT 55554
#define SHUTDOWN_SIGNAL "shutdown"
#define STARTING_PORT 55556

using namespace std;

atomic<bool> shutdown_requested = false;
int available_floors = -1;
mutex floor_mutex; // Mutex for shared resources

void create_socket(int& assignersocket);
void bind_socket(int& assignersocket, int port);
void connect_socks(int &clientsocket, struct sockaddr_in &clientService);
void socket_listens(int& AssignerSocket, string who, int MAX_SENSOR);
void receive_data(int &acceptsocket, string& message, string who);
void send_data(int &acceptsocket, const string &mesaj);
int setup_epoll(int assignersocket);
void listen_to_server(int assignersocket);
void process_sensor_request(int sensorsocket, int epollfd);
void sensor_listen(int assignersocket);
void signal_handler(int signum);
void handle_new_sensors(int epollfd, int assignersocket);

int main() {
    int server_socket_fd=-1;
    create_socket(server_socket_fd);
    struct sockaddr_in serverAddress;
    connect_socks(server_socket_fd, serverAddress);

    int sensorsocketfd = -1;
    create_socket(sensorsocketfd);
    bind_socket(sensorsocketfd, ASSIGNER_PORT);
    socket_listens(sensorsocketfd, "Assigner to Sensor", MAX_SENSORS);

    thread server_listener(listen_to_server, server_socket_fd);
    thread sensor_listener(sensor_listen, sensorsocketfd);
    server_listener.join();
    sensor_listener.join();

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
    cout<<"connect() is OK."<<endl;
}

void handle_new_sensors(int epollfd, int assignersocket) {
    int sensorsocket = accept(assignersocket, NULL, NULL);
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

void sensor_listen(int assignersocket) {
    int epollfd = setup_epoll(assignersocket);
    struct epoll_event events[MAX_SENSORS];
    const int EPOLL_TIMEOUT = 5000; // Milliseconds
    while (!shutdown_requested) {
        int nfds = epoll_wait(epollfd, events, MAX_SENSORS, EPOLL_TIMEOUT);
        if (nfds == -1) {
            if (errno == EINTR) continue; // Interrupted by signal, retry
            cerr << "epoll_wait() failed: " << strerror(errno) << endl;
            break;
        }
        for (int i = 0; i < nfds; i++) {
            if (events[i].data.fd == assignersocket) {
                handle_new_sensors(epollfd, assignersocket);
            } else {
                process_sensor_request(events[i].data.fd, epollfd);
            }
        }
    }

    close(epollfd);
    close(assignersocket);
}

void listen_to_server(int assignersocket) {
    while (!shutdown_requested) {
        string message;
        receive_data(assignersocket, message, "Assigner");

        if (message.empty() || shutdown_requested) {
            continue;
        }
        if (message == SHUTDOWN_SIGNAL) {
            cout << "Shutdown signal received from server." << endl;
            shutdown_requested = 1;
            close(assignersocket);
            break;
        }
        try {
            int new_available_floors = stoi(message);

            {
                lock_guard lock(floor_mutex);
                available_floors = new_available_floors;
            }

            cout << "Updated available floors: " << available_floors << endl;
        } catch (const exception &e) {
            cerr << "Error processing floor update: " << e.what() << endl;
        }
    }
}


void process_sensor_request(int sensorsocket, int epollfd) {
    string message;
    receive_data(sensorsocket, message, "Assigner");

    if (message.empty() || shutdown_requested) {
        return;
    }

    if (message == SHUTDOWN_SIGNAL) {
        cout << "Shutdown signal received." << endl;
        shutdown_requested = 1;
        close(sensorsocket);
        epoll_ctl(epollfd, EPOLL_CTL_DEL, sensorsocket, nullptr);
        return;
    }
    try {
        int requested_floor = stoi(message);
        {
            lock_guard lock(floor_mutex);
            if (requested_floor > available_floors) {
                send_data(sensorsocket, "Invalid floor. Try again.");
                return;
            }
        }

        int port = STARTING_PORT + requested_floor; // Map floor index to a port
        send_data(sensorsocket, to_string(port));
    } catch (const exception &e) {
        cerr << "Error processing message: " << e.what() << endl;
        send_data(sensorsocket, "Invalid input. Try again.");
    }

    close(sensorsocket);
    epoll_ctl(epollfd, EPOLL_CTL_DEL, sensorsocket, nullptr);
}


void signal_handler(int signum) {
    shutdown_requested = 1;
}
