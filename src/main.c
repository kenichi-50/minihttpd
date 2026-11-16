#include <stdio.h>
#include <sys/socket.h>
#include <unistd.h>
#include <string.h>
#include "server.h"
#include "http.h"



int main() {

    char buffer[1024];

    SolicitudHTTP solicitud;

    int servidor = iniciar_servidor(8080);

    if (servidor == -1) {
        return 1;
    }

    printf("Servidor iniciado. Esperando conexiones...\n");
    
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
        buffer[bytes_recibidos] = '\0'; 
        printf("Datos recibidos: %s\n", buffer);
    }

    if (analizar_solicitud(buffer, &solicitud) == -1) {
        printf("Solicitud HTTP inválida\n");
        close(cliente);
        close(servidor);
        return 1;
    }

    printf("Método: %s\n", solicitud.method);
    printf("Ruta: %s\n", solicitud.path);
    printf("Versión: %s\n", solicitud.version);


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