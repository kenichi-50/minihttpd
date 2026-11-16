#include <stdio.h>
#include <sys/socket.h>
#include <unistd.h>
#include <netinet/in.h>
#include "server.h"

int iniciar_servidor(int puerto) {
    int servidor;
    struct sockaddr_in direccion = {0};

    servidor = socket(AF_INET, SOCK_STREAM, 0);

    if (servidor == -1) {
        perror("Error al crear socket");
        return -1;
    }

    direccion.sin_family = AF_INET;
    direccion.sin_addr.s_addr = htonl(INADDR_ANY);
    direccion.sin_port = htons(puerto);

    if (bind(servidor, (struct sockaddr*)&direccion, sizeof(direccion)) == -1) {
        perror("Error al enlazar socket en bind");
        close(servidor);
        return -1;
    }

    printf("Socket asociado a la dirección y puerto\n");

    if (listen(servidor, 5) == -1) {
        perror("Error al escuchar en el socket");
        close(servidor);
        return -1;
    }

    printf("Servidor escuchando en el puerto %d\n", puerto);

    return servidor;
}