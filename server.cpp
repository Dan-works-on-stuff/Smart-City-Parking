#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <fcntl.h>
#include <fstream>
#include <iostream>
#include <pwd.h>
#include <signal.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <utmp.h>

using namespace std;

bool logat = false;
const char *fifo_path = "/home/dan/Documents/CN2/fifoboss";
const char *fifo_path2 = "/home/dan/Documents/CN2/fifosclav";
int fd = open(fifo_path, O_RDONLY);
int fd2 = open(fifo_path2, O_WRONLY);

void afis(string s) {
  // printf("%s\n",s.c_str());
  string modifs = to_string(s.length()) + s;
  if (write(fd2, modifs.c_str(), modifs.size()) == -1) {
    perror("err afis");
  }
}

string trim(const string &str) { // e momentul sa ma dau mare
  size_t first = str.find_first_not_of(" \n\r\t");
  size_t last = str.find_last_not_of(" \n\r\t");
  if (first == string::npos || last == string::npos)
    return "";
  return str.substr(first, last - first + 1);
}

int testuser(string comanda) {
  string username = trim(comanda.substr(
      6)); // ba daca nici acum nu imi vede usernameurile din config dau Alt+F4
  ifstream configfile("config.txt");
  if (!configfile) {
    return 1;
  }

  string linie;
  bool found = false;
  while (getline(configfile, linie)) {
    if (username == trim(linie)) {
      found = true;
      configfile.close();
      return 3;
    }
  }
  configfile.close();
  if (!found) {
    return 2;
  }
  return 0;
}

void returnstatus(int status, string &response) {
  if (status == 1) {
    response = "config.txt not found";
  } else if (status == 2) {
    response = "user not on the list";
  } else if (status == 3) {
    response = "access allowed you are on the list";
  }
}

void forcestop(string comanda) {
  if (comanda == "quit") {
    afis("haipa");
    raise(SIGTSTP);
  }
}

void loginprocess(string comanda) {
  if (!(strncmp(comanda.c_str(), "login:", 6) == 0))
    return;

  if (logat) {
    afis("deja exista un user logat");
    return;
  }
  int tava[2];
  pipe(tava);

  pid_t pid = fork();
  if (pid < 0) {
    afis("err fork");
    return;
  }
  if (pid == 0) {
    close(tava[0]);
    int status = testuser(comanda);
    write(tava[1], &status, sizeof(int)); // int are 4 bytes
    close(tava[1]);
    exit(1);
  } else {
    close(tava[1]);
    int status;
    read(tava[0], &status, sizeof(int));
    close(tava[0]);
    string response;
    returnstatus(status, response);
    if (status == 3) {
      logat = true;
    }
    afis(response);
  }
}

void logout(string comanda) {
  if (strcmp(comanda.c_str(), "logout") == 0)
    if (logat == true)
      logat = false, afis("logout succesful");
    else
      afis("no user is logged in brev");
}

void show_users(string comanda) {
  if (!(comanda == "get-logged-users"))
    return;
  if (!logat) {
    afis("no user logged in, access denied");
    return;
  }
  // int socket_fds[2];
  // if (socketpair(AF_UNIX, SOCK_STREAM, 0, socket_fds) == -1) {
  //   exit(-1);
  // }
  // int sonSocket = socket_fds[0];
  // int parentSocket = socket_fds[1];
  // pid_t pid = fork();
  // if (pid == -1) {
  //   exit(-1);
  // }

  // if (pid == 0) {
    // close(parentSocket);
    setutent();
    struct utmp *userinfo;
    while ((userinfo = getutent()) != nullptr) {
      if (userinfo->ut_type == USER_PROCESS) {
        afis(userinfo->ut_user);
        if (userinfo->ut_host[0] != '\0') {
          afis(userinfo->ut_host); // remote host
        } else {
          afis("Host local");
        }
        time_t logintime = userinfo->ut_tv.tv_sec;
        afis(ctime(&logintime));
      }
    }
    endutent();
  }
// }
/// somehow the while is not working idk why???

void get_info(string comanda, pid_t pid) {
  if (!(comanda == "get-proc-info:pid"))
    return;
  if (!logat) {
    afis("no user logged in, access denied");
    return;
  }
  // rest of the logic that idk
}

int main() {
  if (access(fifo_path, F_OK) != -1) {
    cout << "FIFO-ul pt citire exista deja. Se va folosi FIFO-ul existent."
         << endl;
  } else {
    // Crearea FIFO-ului
    if (mkfifo(fifo_path, 0666) == -1) {
      perror("Eroare la crearea FIFO");
      return 1;
    }
    cout << "FIFO pt citire creat" << endl;
  } // creat fifo1 pt client->sv
  if (access(fifo_path2, F_OK) != -1) {
    cout << "FIFO-ul pt scriere exista deja. Se va folosi FIFO-ul existent."
         << endl;
  } else {
    // Crearea FIFO-ului
    if (mkfifo(fifo_path2, 0666) == -1) {
      perror("Eroare la crearea FIFO");
      return 1;
    }
    cout << "FIFO pt scriere creat" << endl;
  } // creat fifo2 pt sv->client

  if (fd == -1) {
    perror("Eroare la deschidere FIFO pt citire");
    return 1;
  }
  if (fd2 == -1) {
    perror("Eroare la deschidere FIFO pt scriere");
    return 1;
  }
  char buffer[100];

  for (int i = 0; i < 1000; i++) {
    int bytes_read = read(fd, buffer, sizeof(buffer) - 1);
    if (bytes_read > 0) {
      buffer[bytes_read] = '\0'; // terminator de string
      string comanda = trim(string(buffer));
      cout << "Serverul a primit: " << comanda << endl;
      forcestop(comanda);
      loginprocess(comanda);
      logout(comanda);
      show_users(comanda);
      if (!(comanda.substr(0, 6) == "login:" || comanda == "quit" ||
            comanda == "logout" || comanda == "get-logged-users")) {
        afis(comanda);
      }
    }
  }
  return 0;
}