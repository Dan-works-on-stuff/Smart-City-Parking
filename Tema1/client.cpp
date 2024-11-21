#include <csignal>
#include <cstring>
#include <fcntl.h>
#include <iostream>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
using namespace std;

const char *fifo_path = "/home/dan/Documents/CN2/fifoboss";
const char *fifo_path2 = "/home/dan/Documents/CN2/fifosclav";

string trim(const string &str) { // e momentul sa ma dau mare
  size_t first = str.find_first_not_of(" \n\r\t");
  size_t last = str.find_last_not_of(" \n\r\t");
  if (first == string::npos || last == string::npos)
    return "";
  return str.substr(first, last - first + 1);
}
int main() {

  int fd = open(fifo_path, O_WRONLY);
  if (fd == -1) {
    perror("Eroare la deschiderea FIFO pentru scriere");
    return 1;
  } // deschidem fifo pt scriere
  int fd2 = open(fifo_path2, O_RDONLY);
  if (fd == -1) {
    perror("Eroare la deschidere FIFO2 pentru citire");
    close (fd);
    return 1;
  } // deschidem fifo2 pt citire
  string comanda;
  char buffer[100];
  int flag = true;
  while (true) {
    if(!flag){
      int bytes_read = read(fd2, buffer, sizeof(buffer));
      if (bytes_read > 0) {
        buffer[bytes_read] = '\0'; // terminator de string
        string comanda = trim(string(buffer));
        cout << "Client: " << comanda << endl;
      }
    }
    flag=false;
    cout << "Introduceti mesajul de trimis: ";
    getline(cin, comanda);
    if (write(fd, comanda.c_str(), comanda.size()) == -1) {
      perror("Err");
    }
    if (comanda == "quit") {
      cout << "haipa" << endl;
      break;
    }
  }
  close(fd);
  close(fd2);
  return 0;
}