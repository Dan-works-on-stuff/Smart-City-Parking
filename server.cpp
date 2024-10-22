// #include <algorithm>
// #include <cstring>
// #include <fcntl.h>
// #include <fstream>
// #include <iostream>
// #include <map>
// #include <pwd.h>
// #include <signal.h>
// #include <sys/socket.h>
// #include <sys/stat.h>
// #include <sys/types.h>
// #include <sys/wait.h>
// #include <unistd.h>
// #include <utmp.h>
// #include <vector>

// using namespace std;

// string trim(const string &str) { // e momentul sa ma dau mare
//   size_t first = str.find_first_not_of(" \n\r\t");
//   size_t last = str.find_last_not_of(" \n\r\t");
//   if (first == string::npos || last == string::npos)
//     return "";
//   return str.substr(first, last - first + 1);
// }

// const char *fifo_path = "/home/dan/Documents/CN2/fifoboss";
// const char *fifo_path2 = "/home/dan/Documents/CN2/fifosclav";
// int main() {
//   int pipe_fd[2];
//   if (pipe(pipe_fd) == -1) {
//     cerr << "eroare la pipe" << endl;
//     return 1;
//   }
//   if (access(fifo_path, F_OK) != -1) {
//     cout << "FIFO-ul pt citire exista deja. Se va folosi FIFO-ul existent."
//          << endl;
//   } else {
//     // Crearea FIFO-ului
//     if (mkfifo(fifo_path, 0666) == -1) {
//       perror("Eroare la crearea FIFO");
//       return 1;
//     }
//     cout << "FIFO pt citire creat" << endl;
//   } // creat fifo1 pt client->sv
//   if (access(fifo_path2, F_OK) != -1) {
//     cout << "FIFO-ul pt scriere exista deja. Se va folosi FIFO-ul existent."
//          << endl;
//   } else {
//     // Crearea FIFO-ului
//     if (mkfifo(fifo_path2, 0666) == -1) {
//       perror("Eroare la crearea FIFO");
//       return 1;
//     }
//     cout << "FIFO pt scriere creat" << endl;
//   } // creat fifo2 pt sv->client
//   int fd = open(fifo_path, O_RDONLY);
//   int fd2 = open(fifo_path2, O_WRONLY);
//   if (fd == -1) {
//     perror("Eroare la deschidere FIFO pt citire");
//     return 1;
//   }
//   if (fd2 == -1) {
//     perror("Eroare la deschidere FIFO pt scriere");
//     return 1;
//   }

//   char buffer[100];
//   bool logat = false;

//   while (true) {
//     int bytes_read = read(fd, buffer, sizeof(buffer) - 1);
//     if (bytes_read > 0) {
//       buffer[bytes_read] = '\0'; // terminator de string
//       string comanda = trim(string(buffer));
//       cout << "Serverul a primit: " << buffer << endl;
//       if (strcmp(comanda.c_str(), "quit") == 0) {
//         cout << "haipa" << endl;
//         raise(SIGTSTP);
//       }
//       if (strcmp(comanda.c_str(), "logout") == 0 && logat == true) {
//         cout << "iti urez o zi ok" << endl;
//         logat = false;
//       } else if (strcmp(comanda.c_str(), "logout") == 0 && logat == false)
//         cout << "nu exista nici un user logat" << endl;

//       pid_t pid = fork();

//       if (pid == 0) {
//         close(pipe_fd[0]);
//         if (strncmp(comanda.c_str(), "login:", 6) == 0) {
//           string username = trim(comanda.substr(6)); // ba daca nici acum nu
//           imi vede usernameurile din config dau Alt+F4 ifstream
//           configfile("config.txt"); if (!configfile)
//             cerr << "err la txt-ul cu useri" << endl;
//           else {
//             string linie;
//             bool found = false;
//             while (getline(configfile, linie)) {
//               if (username == trim(linie)) {
//                 cout << "se permite accesul esti pe lista bosane" << endl;
//                 found = true;
//                 write(pipe_fd[1], "1", 2);
//                 break;
//               }
//             }
//             if (!found)
//               cout << "no access, user is not on the list, GTFO" << endl,
//                   write(pipe_fd[1], "0", 2);
//             configfile.close();
//           }
//         }
//         close(pipe_fd[1]);
//         _exit(0);
//       } else if (pid < 0) {
//         cerr << "fork pa" << endl;
//       } else {
//         waitpid(pid,NULL, 0);
//         char rez[2];
//         close(pipe_fd[1]);
//         read(pipe_fd[0], rez, 2);

//         if (rez[0] == '1')
//           logat = true;
//         else
//           logat = false;
//       }

//     } else if (bytes_read == -1)
//       cerr << "err" << endl;
//   }

//   close(fd);
//   close(fd2);
//   close(pipe_fd[0]);
//   return 0;
// }

#include <algorithm>
#include <cstring>
#include <fcntl.h>
#include <fstream>
#include <iostream>
#include <map>
#include <pwd.h>
#include <signal.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <utmp.h>
#include <vector>

using namespace std;

bool logat = false;

void afis(string s) { cout << s << endl; }

string trim(const string &str) { // e momentul sa ma dau mare
  size_t first = str.find_first_not_of(" \n\r\t");
  size_t last = str.find_last_not_of(" \n\r\t");
  if (first == string::npos || last == string::npos)
    return "";
  return str.substr(first, last - first + 1);
}
const char *fifo_path = "/home/dan/Documents/CN2/fifoboss";
const char *fifo_path2 = "/home/dan/Documents/CN2/fifosclav";

int fd = open(fifo_path, O_RDONLY);
int fd2 = open(fifo_path2, O_WRONLY);

int testuser(string comanda){
  string username = trim(comanda.substr(6)); // ba daca nici acum nu imi vede usernameurile din config dau Alt+F4
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
    if (!found){
      return 2; 
    }  
}

// void returnstatus(int s){
//   switch(s)
//   { case(1):
//     cerr << "err la txt-ul cu useri" << endl;
//     return;
//   }
//   if()
// }

void forcestop(string comanda) {
  if (strcmp(comanda.c_str(), "quit") == 0) {
    afis("haipa");
    raise(SIGTSTP);
  }
}

void loginprocess(string comanda) {
  if (!(strncmp(comanda.c_str(), "login:", 6) == 0))
    return;

  if (logat) {
    afis("deja esti logat wtf man");
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
    int status=testuser(comanda);
    write(tava[1],&status,sizeof(int)); //int are 4 bytes
    close(tava[1]);
  }
  else {
    close(tava[1]);
    int status;
    read(tava[0],&status,sizeof(int));
    close(tava[0]);
    string s=to_string(status);
    if(write(fd2, s.c_str(), s.size())==-1){
            perror("Err");
      }
  }
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
      cout << "Serverul a primit: " << buffer << endl;
      forcestop(comanda);
      loginprocess(comanda);

      write(fd2," ",sizeof(" "));
    }
  }
  return 0;
}