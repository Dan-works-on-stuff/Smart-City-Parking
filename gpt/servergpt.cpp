#include <iostream>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <string>
#include <vector>
#include <fstream>
#include <map>
#include <fcntl.h>
#include <utmp.h>
#include <pwd.h>
#include <signal.h>
#include <cstring>

std::map<std::string, bool> loggedInUsers; // Map to store logged in users
std::vector<std::string> validUsers; // Valid users from configuration file
int parentToChild[2], childToParent[2]; // Pipe descriptors

void loadUsers() {
    std::ifstream file("users.conf");
    std::string username;
    while (file >> username) {
        validUsers.push_back(username);
    }
    file.close();
}

bool isValidUser(const std::string& username) {
    for (const auto& user : validUsers) {
        if (user == username)
            return true;
    }
    return false;
}

void login(const std::string& username, int clientFd) {
    if (isValidUser(username)) {
        loggedInUsers[username] = true;
        std::string msg = "User " + username + " logged in successfully.\n";
        write(clientFd, msg.c_str(), msg.size());
    } else {
        std::string msg = "Invalid user.\n";
        write(clientFd, msg.c_str(), msg.size());
    }
}

void getLoggedUsers(int clientFd) {
    struct utmp *entry;
    setutent();
    while ((entry = getutent()) != NULL) {
        if (entry->ut_type == USER_PROCESS) {
            std::string userInfo = std::string(entry->ut_user) + " " + std::string(entry->ut_host) + " " + std::to_string(entry->ut_tv.tv_sec) + "\n";
            write(clientFd, userInfo.c_str(), userInfo.size());
        }
    }
    endutent();
}

void getProcInfo(const std::string& pid, int clientFd) {
    std::string procPath = "/proc/" + pid + "/status";
    std::ifstream file(procPath);
    if (!file.is_open()) {
        std::string errorMsg = "Process not found.\n";
        write(clientFd, errorMsg.c_str(), errorMsg.size());
        return;
    }
    
    std::string line, info;
    while (std::getline(file, line)) {
        info += line + "\n";
    }
    write(clientFd, info.c_str(), info.size());
}

void handleClient(int clientFd) {
    char buffer[256];
    while (true) {
        memset(buffer, 0, sizeof(buffer));
        int n = read(clientFd, buffer, sizeof(buffer));
        if (n <= 0) break;

        std::string command(buffer);
        if (command.rfind("login", 0) == 0) {
            std::string username = command.substr(7); // Extract username
            login(username, clientFd);
        } else if (command == "get-logged-users\n") {
            getLoggedUsers(clientFd);
        } else if (command.rfind("get-proc-info", 0) == 0) {
            std::string pid = command.substr(15); // Extract pid
            getProcInfo(pid, clientFd);
        } else if (command == "logout\n") {
            std::string msg = "User logged out.\n";
            write(clientFd, msg.c_str(), msg.size());
            loggedInUsers.clear();
        } else if (command == "quit\n") {
            close(clientFd);
            break;
        }
    }
}

int main() {
    loadUsers();

    int sockpair[2];
    socketpair(AF_UNIX, SOCK_STREAM, 0, sockpair);

    if (fork() == 0) {
        close(sockpair[0]);
        handleClient(sockpair[1]);
        exit(0);
    } else {
        close(sockpair[1]);
        char buffer[256];
        while (true) {
            memset(buffer, 0, sizeof(buffer));
            read(STDIN_FILENO, buffer, sizeof(buffer));
            write(sockpair[0], buffer, strlen(buffer));
            read(sockpair[0], buffer, sizeof(buffer));
            std::cout << buffer;
        }
    }

    return 0;
}
