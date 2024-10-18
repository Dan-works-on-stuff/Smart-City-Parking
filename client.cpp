#include <iostream>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <cstring>
#include <fcntl.h>
using namespace std;

const char *fifo_path= "/home/dan/Documents/CN2/fifoboss";
const char *fifo_path2 = "/home/dan/Documents/CN2/fifosclav";

int main(){

    int fd = open(fifo_path, O_WRONLY);
    if (fd == -1) {
        perror("Eroare la deschiderea FIFO pentru scriere");
        return 1;
    } //deschidem fifo pt scriere
    int fd2 = open(fifo_path2, O_RDONLY);
    if (fd == -1) {
        perror("Eroare la deschidere FIFO2 pentru citire");
        return 1;
    } //deschidem fifo2 pt citire
    string comanda;

    while(true){
        cout << "Introduceti mesajul de trimis: ";
        getline(cin, comanda);
        comanda +='\n';
        if(write(fd, comanda.c_str(), comanda.size())==-1){
            perror("Err");
        }
    }
    close(fd);
    return 0;
}