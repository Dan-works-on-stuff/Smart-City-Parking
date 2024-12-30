#include "functions.h"

#define MAX_EVENTS 10000
#define BUFFER_SIZE 5
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

void create_server_socket(int &server_socket, int port) {
    create_socket(server_socket);

    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;

    if (inet_pton(AF_INET, "127.0.0.1", &server_addr.sin_addr) <= 0) {
        cerr << "inet_pton() failed: " << strerror(errno) << endl;
        close(server_socket);
        exit(1);
    }

    server_addr.sin_port = htons(port);

    if (connect(server_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1) {
        cerr << "connect() failed: " << strerror(errno) << endl;
        close(server_socket);
        exit(1);
    }

    cout << "Successfully connected to socket at " << port << ":" << port << endl;
}

int setup_epoll(int server_socket) {
    int epoll_fd = epoll_create1(0);
    if (epoll_fd == -1) {
        cerr << "epoll_create1() failed: " << strerror(errno) << endl;
        close(server_socket);
        exit(1);
    }

    struct epoll_event ev;
    ev.events = EPOLLIN;
    ev.data.fd = server_socket;

    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, server_socket, &ev) == -1) {
        cerr << "epoll_ctl() failed: " << strerror(errno) << endl;
        close(server_socket);
        close(epoll_fd);
        exit(1);
    }

    return epoll_fd;
}

void send_data(int &sensorsocket, const string &mesaj) {
    const char* buffer = mesaj.c_str();
    size_t buffer_len = mesaj.length();
    int bytes_sent = send(sensorsocket, buffer, buffer_len, 0);
    if (bytes_sent == -1) {
        cerr << "send() failed: " << strerror(errno) << endl;
    } else {
        cout << "Assigner: sent " << bytes_sent << " bytes" << endl;
    }
}

void handle_client_request(int client_socket, int epoll_fd) {
    char buffer[BUFFER_SIZE];
    int bytes_received = recv(client_socket, buffer, sizeof(buffer) - 1, 0);

    if (bytes_received <= 0) {
        if (bytes_received == 0) {
            cout << "Sensor disconnected." << endl;
        } else {
            cerr << "recv() failed: " << strerror(errno) << endl;
        }
        close(client_socket);
        epoll_ctl(epoll_fd, EPOLL_CTL_DEL, client_socket, nullptr); // Remove from epoll
        return;
    }

    buffer[bytes_received] = '\0';
    string client_message = buffer;

    if (client_message == SHUTDOWN_SIGNAL) {
        cout << "Shutdown signal received." << endl;
        shutdown_requested = 1;
        close(client_socket);
        epoll_ctl(epoll_fd, EPOLL_CTL_DEL, client_socket, nullptr); // Remove from epoll
        return;
    }

    try {
        int requested_floor = stoi(client_message);
        {
            lock_guard<mutex> lock(floor_mutex);
            if (requested_floor > available_floors) {
                send_data(client_socket, "Invalid floor. Try again.");
                return;
            }
        }
        int port = 55556 + requested_floor;
        send_data(client_socket, to_string(port));
        close(client_socket);
        epoll_ctl(epoll_fd, EPOLL_CTL_DEL, client_socket, nullptr); // Remove from epoll
    } catch (const exception &e) {
        send_data(client_socket, "Invalid input. Try again.");
        return;
    }
}

void handle_new_sensor(int server_socket, int epoll_fd) {
    struct sockaddr_in client_addr;
    socklen_t client_len = sizeof(client_addr);
    int client_socket = accept(server_socket, (struct sockaddr *)&client_addr, &client_len);

    if (client_socket == -1) {
        cerr << "accept() failed: " << strerror(errno) << endl;
        return;
    }

    cout << "New connection accepted." << endl;

    struct epoll_event ev;
    ev.events = EPOLLIN;
    ev.data.fd = client_socket;

    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, client_socket, &ev) == -1) {
        cerr << "epoll_ctl() failed: " << strerror(errno) << endl;
        close(client_socket);
    }
}

void sensor_processing_thread(int server_socket) {
    int epoll_fd = setup_epoll(server_socket);
    struct epoll_event events[MAX_EVENTS];
    const int EPOLL_TIMEOUT = 5000; // Milliseconds

    while (!shutdown_requested) {
        int nfds = epoll_wait(epoll_fd, events, MAX_EVENTS, EPOLL_TIMEOUT);
        if (nfds == -1) {
            if (errno == EINTR) {
                break;
            }
            cerr << "epoll_wait() failed: " << strerror(errno) << endl;
            break;
        }
        if (nfds == 0) {
            continue;
        }
        for (int i = 0; i < nfds; ++i) {
            if (events[i].data.fd == server_socket) {
                handle_new_sensor(server_socket, epoll_fd);
            } else {
                handle_client_request(events[i].data.fd, epoll_fd);
            }
        }
    }

    close(epoll_fd);
}

void server_communication_thread(int server_socket) {
    while (!shutdown_requested) {
        int server_client_socket;
        struct sockaddr_in server_client_addr;
        socklen_t server_client_len = sizeof(server_client_addr);

        server_client_socket = accept(server_socket, (struct sockaddr *)&server_client_addr, &server_client_len);
        if (server_client_socket == -1) {
            cerr << "Server accept() failed: " << strerror(errno) << endl;
            continue;
        }

        char buffer[BUFFER_SIZE];
        int bytes_received = recv(server_client_socket, buffer, sizeof(buffer) - 1, 0);
        if (bytes_received <= 0) {
            close(server_client_socket);
            continue;
        }

        buffer[bytes_received] = '\0';
        string server_message = buffer;

        if (server_message == SHUTDOWN_SIGNAL) {
            cout << "Shutdown signal received from server." << endl;
            shutdown_requested = 1;
            close(server_client_socket);
            break;
        }

        int new_floors = stoi(server_message);

        {
            lock_guard<mutex> lock(floor_mutex);
            available_floors = new_floors;
        }

        cout << "Updated available floors to: " << available_floors << endl;
        close(server_client_socket);
    }
}

int main() {
    signal(SIGINT, signal_handler);

    int sensor_socket, server_socket;
    create_server_socket(sensor_socket, ASSIGNER_PORT);
    create_server_socket(server_socket, SERVER_PORT);

    thread sensor_thread(sensor_processing_thread, sensor_socket);
    thread server_thread(server_communication_thread, server_socket);

    sensor_thread.join();
    server_thread.join();

    cout << "Shutting down assigner." << endl;
    close(sensor_socket);
    close(server_socket);
    return 0;
}
