#include <stdio.h>
#include <sys/socket.h>
#include <unistd.h>

int main() {
    int servidor;

    servidor = socket(AF_INET, SOCK_STREAM, 0);

    if (servidor == -1) {
        perror("Error al crear socket");
        return 1;
    }

    printf("Socket creado correctamente\n");
    printf("Descriptor: %d\n", servidor);

    close(servidor);
    return 0;
}