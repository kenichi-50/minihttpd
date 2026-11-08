#include <stdio.h>
#include <sys/socket.h>
#include <unistd.h>
#include <netinet/in.h>


int main() {
    int servidor;
    struct sockaddr_in direccion = {0};

    servidor = socket(AF_INET, SOCK_STREAM, 0);

    if (servidor == -1) {
        perror("Error al crear socket");
        return 1;
    }

    direccion.sin_family = AF_INET;
    direccion.sin_addr.s_addr = htonl(INADDR_ANY);
    direccion.sin_port = htons(8080);

    if (bind(servidor, (struct sockaddr*)&direccion, sizeof(direccion)) == -1) {
        perror("Error al enlazar socket en bind");
        close(servidor);
        return 1;
    }

    printf("Socket asociado a la dirección y puerto\n");

    close(servidor);
    return 0;
}