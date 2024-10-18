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

const char *fifo_path = "/home/dan/Documents/CN2/fifoboss";
const char *fifo_path2 = "/home/dan/Documents/CN2/fifosclav";
int main() {

  if (access(fifo_path, F_OK) != -1) {
    cout << "FIFO-ul exista deja. Se va folosi FIFO-ul existent." << endl;
  } else {
    // Crearea FIFO-ului
    if (mkfifo(fifo_path, 0666) == -1) {
      perror("Eroare la crearea FIFO");
      return 1;
    }
    cout << "FIFO creat, așteptând trimiterea de date..." << endl;
  } // creat fifo1 pt client->sv
  if (access(fifo_path2, F_OK) != -1) {
    cout << "FIFO-ul exista deja. Se va folosi FIFO-ul existent." << endl;
  } else {
    // Crearea FIFO-ului
    if (mkfifo(fifo_path2, 0666) == -1) {
      perror("Eroare la crearea FIFO");
      return 1;
    }
    cout << "FIFO creat, așteptând date..." << endl;
  } // creat fifo2 pt sv->client
  int fd = open(fifo_path, O_RDONLY);
  int fd2 = open(fifo_path2, O_WRONLY);
  if (fd == -1) {
    perror("Eroare la deschidere FIFO");
    return 1;
  }
  char buffer[100];
  while (true) {
    int bytes_read = read(fd, buffer, sizeof(buffer));
    if (bytes_read > 0) {
      buffer[bytes_read] = '\0'; // terminator de string
      cout << "Serverul a primit: " << buffer << endl;
      pid_t pid=fork();

      if (pid == 0) {
        if (strncmp(buffer, "login:", 6) == 0) {
          char *remaining_buffer = buffer + 6;
          ifstream configfile("config.txt");
          if (!configfile)
            cerr << "err la txt-ul cu useri" << endl;
          else {
            string linie;
            bool found=false;
            while (getline(configfile, linie)) {
              if (strcmp(remaining_buffer, linie.c_str()) == 0) {
                cout << "se permite accesul esti pe lista bosane" << endl,
                    found = true;
                break;
              }
            }if (!found)
                cout << "no access, user is not on the list";
            configfile.close();
          }
        }
        _exit(0);
      } 
      else if(pid<0){
        cerr<<"fork failed"<<endl;
      }
    }else if(bytes_read==-1)
      cerr<<"err"<<endl;
  }

  close(fd);
  return 0;
}