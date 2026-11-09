#include <stdio.h>
#include <sys/socket.h>
#include <unistd.h>
#include <netinet/in.h>
#include <string.h>



int main() {
    int servidor;
    struct sockaddr_in direccion = {0};
    char buffer[1024];

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

    if (listen(servidor, 5) == -1) {
        perror("Error al escuchar en el socket");
        close(servidor);
        return 1;
    }

    printf("Servidor escuchando en el puerto 8080\n");

    int cliente = accept(servidor, NULL, NULL);
    
    if (cliente == -1) {
        perror("Error al aceptar conexión");
        close(servidor);
        return 1;
    }

    printf("Cliente conectado. Descriptor: %d\n", cliente);
    
    ssize_t bytes_recibidos = recv(cliente, buffer, sizeof(buffer) - 1, 0);
    if (bytes_recibidos == -1) {
        perror("Error al recibir datos");
        close(cliente);
        close(servidor);
        return 1;
    }

    if (bytes_recibidos == 0) {
        printf("Cliente desconectado\n");
        close(cliente);
        close(servidor);
        return 0;
    } else {
        buffer[bytes_recibidos] = '\0'; // Asegurarse de que el buffer esté terminado en nulo
        printf("Datos recibidos: %s\n", buffer);
    }

    const char *repuestas = 
        "HTTP/1.1 200 OK\r\n"
        "Content-Type: text/plain\r\n"
        "Content-Length: 6\r\n"
        "Connection: close\r\n"
        "\r\n"
        "Hola!\n";

    ssize_t enviados = send(cliente, repuestas, strlen(repuestas), 0);
    if (enviados == -1) {
        perror("Error al enviar respuesta");
        close(cliente);
        close(servidor);
        return 1;

    }

    printf("Respuesta enviada al cliente\n");
    
    close(cliente);
    close(servidor);

    return 0;
}