#include "functions.h"

#define MAX_EVENTS 10000
#define ASSIGNER_PORT 55555
#define SERVER_PORT 55554
#define SHUTDOWN_SIGNAL "shutdown"

using namespace std;

volatile sig_atomic_t shutdown_requested = 0;
mutex floor_mutex;
int available_floors = -1;

void signal_handler(int signum) {
    shutdown_requested = 1;
}

void receive_data(int &acceptsocket, string& message) {
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
        if (message=="pa") {
            return;
        }
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
    else cout<<"Server: sent "<< bytes_sent << " bytes ("<<mesaj<<')'<<endl;
}

// Create a socket and connect to the server
void create_and_connect_socket(int &socket_fd, int port) {
    create_socket(socket_fd);

    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);

    if (inet_pton(AF_INET, "127.0.0.1", &addr.sin_addr) <= 0) {
        cerr << "inet_pton() failed: " << strerror(errno) << endl;
        close(socket_fd);
        exit(1);
    }

    if (connect(socket_fd, (struct sockaddr *)&addr, sizeof(addr)) == -1) {
        cerr << "connect() failed: " << strerror(errno) << endl;
        close(socket_fd);
        exit(1);
    }

    cout << "Successfully connected to socket on port " << port << endl;
}

// EPOLL setup for non-blocking I/O
int setup_epoll(int socket_fd) {
    int epoll_fd = epoll_create1(0);
    if (epoll_fd == -1) {
        cerr << "epoll_create1() failed: " << strerror(errno) << endl;
        close(socket_fd);
        exit(1);
    }

    struct epoll_event ev;
    ev.events = EPOLLIN;
    ev.data.fd = socket_fd;

    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, socket_fd, &ev) == -1) {
        cerr << "epoll_ctl() failed: " << strerror(errno) << endl;
        close(socket_fd);
        close(epoll_fd);
        exit(1);
    }

    return epoll_fd;
}

// Handle client request received on a socket
void process_client_request(int client_socket, int epoll_fd) {
    string message;
    receive_data(client_socket, message);

    if (message.empty() || shutdown_requested) {
        return; // Client disconnected or shutdown signal handled
    }

    if (message == SHUTDOWN_SIGNAL) {
        cout << "Shutdown signal received." << endl;
        shutdown_requested = 1;
        close(client_socket);
        epoll_ctl(epoll_fd, EPOLL_CTL_DEL, client_socket, nullptr);
        return;
    }

    try {
        int requested_floor = stoi(message);
        {
            lock_guard<mutex> lock(floor_mutex);
            if (requested_floor > available_floors) {
                send_data(client_socket, "Invalid floor. Try again.");
                return;
            }
        }

        int port = 55556 + requested_floor; // Map floor index to a port
        send_data(client_socket, to_string(port));
    } catch (const exception &e) {
        cerr << "Error processing message: " << e.what() << endl;
        send_data(client_socket, "Invalid input. Try again.");
    }

    close(client_socket);
    epoll_ctl(epoll_fd, EPOLL_CTL_DEL, client_socket, nullptr);
}

// Accept new sensor connections and add them to epoll
void handle_new_sensor_connection(int server_socket, int epoll_fd) {
    int client_socket = accept(server_socket, nullptr, nullptr);
    if (client_socket == -1) {
        cerr << "accept() failed: " << strerror(errno) << endl;
        return;
    }

    cout << "New sensor connection accepted." << endl;

    struct epoll_event ev;
    ev.events = EPOLLIN;
    ev.data.fd = client_socket;

    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, client_socket, &ev) == -1) {
        cerr << "epoll_ctl() failed: " << strerror(errno) << endl;
        close(client_socket);
    }
}

// Handle sensor connections and requests in a thread
void sensor_handling_thread(int server_socket) {
    int epoll_fd = setup_epoll(server_socket);
    struct epoll_event events[MAX_EVENTS];
    const int EPOLL_TIMEOUT = 5000; // Milliseconds

    while (!shutdown_requested) {
        int nfds = epoll_wait(epoll_fd, events, MAX_EVENTS, EPOLL_TIMEOUT);
        if (nfds == -1) {
            if (errno == EINTR) continue; // Interrupted by signal, retry
            cerr << "epoll_wait() failed: " << strerror(errno) << endl;
            break;
        }

        for (int i = 0; i < nfds; ++i) {
            if (events[i].data.fd == server_socket) {
                handle_new_sensor_connection(server_socket, epoll_fd);
            } else {
                process_client_request(events[i].data.fd, epoll_fd);
            }
        }
    }

    close(epoll_fd);
}

// Receive updates from the server about floor availability
void floor_availability_thread(int server_socket) {
    while (!shutdown_requested) {
        int client_socket = accept(server_socket, nullptr, nullptr);
        if (client_socket == -1) {
            cerr << "Server accept() failed: " << strerror(errno) << endl;
            continue;
        }

        string message;
        receive_data(client_socket, message);

        if (message.empty() || shutdown_requested) {
            continue;
        }

        if (message == SHUTDOWN_SIGNAL) {
            cout << "Shutdown signal received from server." << endl;
            shutdown_requested = 1;
            close(client_socket);
            break;
        }

        try {
            int new_available_floors = stoi(message);

            {
                lock_guard<mutex> lock(floor_mutex);
                available_floors = new_available_floors;
            }

            cout << "Updated available floors: " << available_floors << endl;
        } catch (const exception &e) {
            cerr << "Error processing floor update: " << e.what() << endl;
        }

        close(client_socket);
    }
}

int main() {
    signal(SIGINT, signal_handler);

    int sensor_socket, server_socket;
    //create_and_connect_socket(sensor_socket, ASSIGNER_PORT);
    create_and_connect_socket(server_socket, SERVER_PORT);

    thread sensor_thread(sensor_handling_thread, sensor_socket);
    thread floor_thread(floor_availability_thread, server_socket);

    sensor_thread.join();
    floor_thread.join();

    cout << "Shutting down assigner." << endl;
    close(sensor_socket);
    close(server_socket);
    return 0;
}
