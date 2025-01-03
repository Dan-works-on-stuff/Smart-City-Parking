#include "shared_functions.h"

using namespace std;

#define FLOORMASTER_PORT 55553
#define FLOORMASTER_SERVERS 55556
#define ASSIGNER_PORT 55554
const int MAX_PARCARI = 10;

atomic<bool> admin_access(false);
atomic <bool> shutdown_requested = false;
struct FloorMaster {
    int index;
    char letter;
    vector<bool> parking_spots;
    int FMsock;
    int socket_for_sensors;
};
vector<FloorMaster> FM;
mutex mtx;
//fiecarui FloorMaster i se asociaza o structura de tipul FloorMaster ce contine chestiile alea

void create_socket(int& ServerSocket);
void bind_socket(int& ServerSocket, int port);
void socket_listens(int& ServerSocket, string who, int MAX_PARKING);
void receive_data(int &AcceptSocket, string& message, string who);
void send_data(int &AcceptSocket, const string &mesaj);
int setup_epoll(int ServerSocket);
void handle_new_FloorMaster(int epollfd, int ServerSocket, int assignersocket);
void handle_communication(int epollfd, int clientsocket);
void update_parking_spots(int clientsocket, const string& data);
void server_executions(int ServerSocket, int assignersocket);
void update_the_db();
void admin_listener(int assignersocket);
void admin_commands(int assignersocket);
void signal_handler(int signal);
void save_server_pid();
pid_t read_server_pid();

int main() {
    signal(SIGPIPE, SIG_IGN);
    signal(SIGUSR1, signal_handler);

    int ServerSocketfd=-1;
    create_socket(ServerSocketfd);
    bind_socket(ServerSocketfd, FLOORMASTER_PORT);
    cout<<"Listen state on "<<FLOORMASTER_PORT<<": ";
    socket_listens(ServerSocketfd, "Server for FloorMaster", MAX_PARCARI);

    int assignersocketfd=-1;
    create_socket(assignersocketfd);
    bind_socket(assignersocketfd, ASSIGNER_PORT);
    cout<<"Listen state on "<<ASSIGNER_PORT<<": ";
    socket_listens(assignersocketfd, "Server for Assigner", MAX_PARCARI);
    int assigner_socket=accept(assignersocketfd, NULL, NULL);
    if (assigner_socket==-1) {
        cerr<<"accept() failed: "<<strerror(errno)<<endl;
    }
    else cout<<"assigner socket connected and ready for prime time "<< assigner_socket<<endl;
    save_server_pid();

    thread server_thread(server_executions, ServerSocketfd, assigner_socket);
    thread admin_thread(admin_listener, assigner_socket);
    server_thread.join();
    admin_thread.join();

    close(assignersocketfd);
    close(ServerSocketfd);
    return 0;
}

void server_executions(int ServerSocket, int assignersocket) {
    int epollfd=setup_epoll(ServerSocket);
    struct epoll_event etaje[MAX_PARCARI];
    while (!shutdown_requested) {
        int nfds=epoll_wait(epollfd, etaje, MAX_PARCARI, -1);
        if (nfds==-1) {
            cerr<<"epoll_wait() failed: "<<strerror(errno)<<endl;
        }
        for (int i=0; i<nfds; i++) {
            if (etaje[i].data.fd==ServerSocket) {
                handle_new_FloorMaster(epollfd, ServerSocket, assignersocket);
            }
            else handle_communication(epollfd, etaje[i].data.fd);
        }
    }
    close(epollfd);
    cout<<"Au revoir"<<endl;
}



void handle_new_FloorMaster(int epollfd, int ServerSocket, int assignersocket) {
    struct epoll_event ev;
    int clientsocket=accept(ServerSocket, NULL, NULL);
    if (clientsocket==-1) {
        cerr<<"accept() failed: "<<strerror(errno)<<endl;
        return;
    }
    int index=FM.size();
    if (index>=10) { //nr maxim de etaje, nu bag mana in foc ca ar merge codul bine nici macar cu 10 etaje, is 10 000 de locuri de parcare totusi
        cerr<<"Max FloorMasters reached!"<<endl;
        close(clientsocket);
        return;
    }
    char letter= 'A' + index;
    FloorMaster newFM ={index, letter, vector<bool>(100, false),clientsocket, FLOORMASTER_SERVERS+index};
    FM.push_back(newFM);

    ev.events=EPOLLIN;
    ev.data.fd=clientsocket;
    if (epoll_ctl(epollfd, EPOLL_CTL_ADD, clientsocket, &ev)==-1) {
        cerr<<"epoll_ctl() failed"<<strerror(errno)<<endl;
        close(clientsocket);
        return;
    }
    cout<<"New FloorMaster connected (Index: "<<index<<" / Letter: "<<letter<<')'<<endl;
    string level(1, letter);
    send_data(clientsocket, level);
    send_data(assignersocket, to_string(index));
    cout<<"New floor registered with the assigner as well."<<endl;
}

void handle_communication(int epollfd, int clientsocket) {
    string message;
    receive_data(clientsocket, message, "Server");
    if (message.find_first_not_of("01", 3)!=string::npos) {
        cerr<<"Invalid data received: "<< message<<endl;
        string response="Error: Invalid parking data format";
        send_data(clientsocket, response);
        return;
    }
    update_parking_spots(clientsocket,  message);
    update_the_db();
    send_data(clientsocket, "Received data successfully.");
}

void update_parking_spots(int clientsocket, const string& data) {
    for (auto& fm : FM) {
        if (fm.FMsock == clientsocket) {
            for (size_t i = 0; i < fm.parking_spots.size(); i++) {
                fm.parking_spots[i] = (data[i+3] == '1');
            }
            cout << "Parking spots updated for FloorMaster " << fm.index << " (" << fm.letter << ")" << endl;
            return;
        }
    }//trebuie sa configurez ceva aici astfel incat sa am un output intr-un fisier cu toate etajele.
    cerr << "Error: FloorMaster not found for clientsocket " << clientsocket << endl;
}

void update_the_db() {
    ofstream output_file("parking.txt", ios::trunc);
        if (!output_file) {
            cerr << "Error opening the file!" << endl;
            return;
        }
        // Lock the mutex before accessing shared data
        mtx.lock();

        // Iterate through the FM vector
        for (const auto& fm : FM) {
            output_file << "FloorMaster Letter: " << fm.letter << endl;
            output_file << "Parking Spots: ";
            for (bool spot : fm.parking_spots) {
                output_file << spot <<' ';
            }
            output_file << endl << "-------------------------" << endl;
        }
        // Unlock the mutex after writing data
        mtx.unlock();
    output_file.close();
}

void admin_listener(int assignersocket) {
    while (true) {
        string input;
        getline(cin,input);
        if (input.rfind("admin access:" , 0)!=0) {
            continue;
        }
        string admin_name=input.substr(13);
        ifstream admin_file("admins.txt");
        if (!admin_file.is_open()) {
            cerr<<"Admin database not found."<<endl;
            continue;
        }
        bool valid_admin = false;
        string valid_name;
        while (admin_file>>valid_name) {
            if (admin_name == valid_name) {
                valid_admin = true;
                break;
            }
        }
        admin_file.close();
        if (!valid_admin) {
            cerr<<"Admin ID unrecognised."<<endl;
            continue;
        }
        cout<<"Admin ID recognised, enter the admin password: ";
        getline(cin, input);
        const string password = "bacau";
        if (input == password) {
            cout<<"Admin access granted"<<endl;
            admin_access=true;
            admin_commands(assignersocket);
        }else {
            cerr<<"Incorrect password."<<endl;
        }
    }
}

void admin_commands(int assignersocket) {
    pid_t server_pid = read_server_pid();
    while (true) {
        string input;
        getline(cin,input);
        if (input.rfind("logout" , 0) == 0) {
            admin_access=false;
            cout<<"Logout request approved. Who are you?";
            break;
        }
        if (input.rfind("shutdown",  0)==0) {
            //implement a .txt to save the current parkings status
            kill(server_pid, SIGUSR1);
            cout<<"Shutdown signal sent. Bye bye!"<<endl;
            send_data(assignersocket, "shutdown");
            break;
        }
    }
}

void signal_handler(int signal) {
    if (signal == SIGUSR1) {
        shutdown_requested=true;
    }
}

void save_server_pid() {
    ofstream pid_file("server.pid");
    if (pid_file.is_open()) {
        pid_file<<getpid();
        pid_file.close();
    }else {
        cerr<<"Failed to save server PID"<<endl;
    }
}

pid_t read_server_pid() {
    ifstream pid_file("server.pid");
    pid_t pid;
    if (pid_file.is_open()) {
        pid_file>>pid;
        pid_file.close();
    }else {
        cerr<<"Could not read server PID"<<endl;
        exit(1);
    }
    return pid;
}
