#include "functions.h"

using namespace std;

const int MAX_PARCARI = 10;

atomic<bool> admin_access(false);
atomic <bool> shutdown_requested = false;
struct FloorMaster {
    int index;
    char letter;
    vector<bool> parking_spots;
    int FMsock;
    int port;
};
vector<FloorMaster> FM;
int next_port = 50001;
//fiecarui FloorMaster i se asociaza o structura de tipul FloorMaster ce contine chestiile alea

void create_socket(int& serversocket);
void bind_socket(int& serversocket, int port);
void socket_listens(int& serversocket);
void receive_data(int &acceptsocket, string& message);
void send_data(int &acceptsocket, const string &mesaj);
void handle_new_FloorMaster(int epollfd, int serversocket);
void handle_communication(int epollfd, int clientsocket);
void update_parking_spots(int clientsocket, const string& data);
int setup_epoll(int serversocket);
void server_executions(int serversocket);
void admin_listener();
void admin_commands();
void signal_handler(int signal);
void save_server_pid();
pid_t read_server_pid();

int main() {
    signal(SIGUSR1, signal_handler);

    int serversocketfd=-1;
    create_socket(serversocketfd);
    set_socket_options(serversocketfd);
    bind_socket(serversocketfd, 55554);
    socket_listens(serversocketfd);

    save_server_pid();

    thread server_thread(server_executions, serversocketfd);
    thread admin_thread(admin_listener);
    server_thread.join();
    admin_thread.join();

    close(serversocketfd);
    return 0;
}

void server_executions(int serversocket) {
    int epollfd=setup_epoll(serversocket);
    struct epoll_event etaje[MAX_PARCARI];

    while (!shutdown_requested) {
        int nfds=epoll_wait(epollfd, etaje, MAX_PARCARI, -1);
        if (nfds==-1) {
            if (errno == EINTR) break;
            cerr<<"epoll_wait() failed: "<<strerror(errno)<<endl;
            continue;
        }
        for (int i=0; i<nfds; i++) {
            if (etaje[i].data.fd==serversocket) {
                handle_new_FloorMaster(epollfd, serversocket);
            }
            else handle_communication(epollfd, etaje[i].data.fd);
        }
    }
    close(epollfd);
    cout<<"Au revoir"<<endl;
}

// listen(socket, int backlog); backlog=limita de clienti pt socket
void socket_listens(int& serversocket) {
    if (listen(serversocket,MAX_PARCARI)==-1) {  //am ales 10 clienti reprezentand un numar de 10 etaje teoretice posibile ale unei parcari
        cerr<<"listen():Error listening to socket "<< strerror(errno)<<endl;
        close(serversocket);
        exit(1);
    }
    else cout<<"listen() OK, waiting for connections..."<<endl;
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

void handle_new_FloorMaster(int epollfd, int serversocket) {
    struct epoll_event ev;
    int clientsocket=accept(serversocket, nullptr, nullptr);
    if (clientsocket==-1) {
        cerr<<"accept() failed: "<<strerror(errno)<<endl;
        return;
    }
    int index=FM.size();
    if (index>=MAX_PARCARI) { //nr maxim de etaje, nu bag mana in foc ca ar merge codul bine nici macar cu 10 etaje, is 10 000 de locuri de parcare totusi
        cerr<<"Max FloorMasters reached!"<<endl;
        close(clientsocket);
        return;
    }
    int assigned_port = next_port++;
    int newsocket;
    create_socket(newsocket);
    set_socket_options(newsocket);
    bind_socket(newsocket, assigned_port);
    socket_listens(newsocket);
    char letter = 'A' + index;
    FloorMaster newFM = {index, letter, vector<bool>(100, false), newsocket, assigned_port};
    FM.push_back(newFM);
    send_data(clientsocket, to_string(assigned_port));  //modify this if necessary
    cout<<"New FloorMaster connected (Index: "<<index<<" / Letter: "<<letter<<", Port: "<<assigned_port<<')'<<endl;
}

void handle_communication(int epollfd, int clientsocket) {
    string message;
    receive_data(clientsocket, message);
    if (message=="pa") {
        cout<<"One FloorMaster disconnected."<<endl;
        epoll_ctl(epollfd, EPOLL_CTL_DEL, clientsocket, nullptr);
        close(clientsocket);
        return;
    }
    if (message.length()>100 || message.find_first_not_of("01")!=string::npos) {
        cerr<<"Invalid data received: "<< message<<endl;
        string response="Error: Invalid parking data format";
        send_data(clientsocket, response);
        return;
    }
    update_parking_spots(clientsocket,  message);
    send_data(clientsocket, message);
}

void update_parking_spots(int clientsocket, const string& data) {
    int floorIndex=-1;
    for (int i=0; i<FM.size(); i++) {
        if (FM[i].FMsock==clientsocket) {
            floorIndex=i;
            break;
        }
    }
    if (floorIndex==-1) {
        cerr<<"Error: FloorMaster not found for clientsocket "<<clientsocket<<endl;
        return;
    }
    if (data.size()!=FM[floorIndex].parking_spots.size()) {
        cerr<<"Error: Data size mismatch for FloorMaster "<<floorIndex<<endl;
        return;
    }
    for (int i=0;i<data.size(); i++) {
        FM[floorIndex].parking_spots[i]=(data[i]=='1');
    }
    //trebuie sa configurez ceva aici astfel incat sa am un output intr-un fisier cu toate etajele.
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

void admin_listener() {
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
            admin_commands();
        }else {
            cerr<<"Incorrect password."<<endl;
        }
    }
}

void admin_commands() {
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