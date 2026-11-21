#include <stdio.h>
#include <sys/socket.h>
#include <unistd.h>
#include <string.h>
#include "server.h"
#include "http.h"
#include <stdint.h>
#include <stdlib.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/epoll.h>
#include <strings.h>
#include <sys/types.h>
#include <sys/sendfile.h>
#include <signal.h>
#include "files.h"
#include "mime.h"


#define MAX_EVENTS 64
#define TAM_BUFFER 8192

typedef struct {
    int fd;

    char buffer[TAM_BUFFER + 1];
    size_t usados;

    char cabecera[512];
    size_t longitud_cabecera;
    size_t enviados;

    int archivo_fd;
    off_t tamano_archivo;
    off_t offset_archivo;

    int mantener_conexion;
    
} Cliente;

static int hacer_no_bloqueante(int fd) {

    int opciones = fcntl(fd, F_GETFL, 0);

    if (opciones == -1) {
        return -1;
    }

    return fcntl(fd, F_SETFL, opciones | O_NONBLOCK);
}
    

static void cerrar_cliente(int epfd, Cliente *cliente) {

    epoll_ctl(epfd, EPOLL_CTL_DEL, cliente->fd, NULL);

    if (cliente->archivo_fd >= 0) {

        close(cliente->archivo_fd);

    }

    close(cliente->fd);

    free(cliente);
}

static int preparar_respuesta(
    int epfd,
    Cliente *cliente,
    int resultado,
    int persistente,
    const char *ruta
) {
    int codigo = 500;
    const char *tipo = "text/plain";

    cliente->tamano_archivo = 0;
    cliente->offset_archivo = 0;
    cliente->enviados = 0;

    if (resultado == -1) {
        codigo = 400;
    } else if (resultado == -2) {
        codigo = 405;
    } else if (resultado == 0 && ruta != NULL) {
        codigo = abrir_archivo(
            ruta,
            &cliente->archivo_fd,
            &cliente->tamano_archivo
        );

        if (codigo == 200) {
            tipo = obtener_mime(ruta);
        }
    }

    if (codigo != 200) {
        persistente = 0;
        cliente->tamano_archivo = 0;

        if (cliente->archivo_fd >= 0) {
            close(cliente->archivo_fd);
            cliente->archivo_fd = -1;
        }
    }

    cliente->mantener_conexion = persistente;

    int n = construir_cabecera_http(
        cliente->cabecera,
        sizeof(cliente->cabecera),
        codigo,
        tipo,
        cliente->tamano_archivo,
        persistente
    );

    if (n < 0) {
        return -1;
    }

    cliente->longitud_cabecera = (size_t)n;

    struct epoll_event evento = {0};
    evento.events = EPOLLOUT;
    evento.data.ptr = cliente;

    if (epoll_ctl(epfd, EPOLL_CTL_MOD, cliente->fd, &evento) == -1) {
        return -1;
    }

    return 1;
}

static int solicita_cierre(const char *connection) {

    const char *p = connection;

    while (*p) {

        while (*p == ',' || *p == ' ' || *p == '\t') {
            p++;
        }

        const char *inicio = p;

        while (*p != '\0' && *p != ',') {
            p++;
        }

        size_t longitud = (size_t)(p - inicio);

        while (longitud > 0 &&
            (inicio[longitud - 1] == ' ' ||
             inicio[longitud - 1] == '\t')) {

            longitud--;
        }

        if (longitud == 5 &&
            strncasecmp(inicio, "close", 5) == 0) {

            return 1;
        }
    }

    return 0;
}

static int procesar_buffer(int epfd, Cliente *cliente) {

    if (memchr(
            cliente->buffer,
            '\0',
            cliente->usados
        ) != NULL) {

        return preparar_respuesta(epfd, cliente, -1, 0, NULL);
    }

    const char *fin = strstr(
        cliente->buffer,
        "\r\n\r\n"
    );

    if (fin == NULL) {

        if (cliente->usados == TAM_BUFFER) {

            return preparar_respuesta(
                epfd,
                cliente,
                -1,
                0,
                NULL
            );
        }

        return 0;
    }

    SolicitudHTTP solicitud = {0};

    int resultado = analizar_solicitud(
        cliente->buffer,
        &solicitud
    );

    int persistente = 0;

    if (resultado == 0 &&
        !solicita_cierre(solicitud.connection)) {

        persistente = 1;
    }

    printf(
        "Cliente %d: %s %s (resultado %d)\n",
        cliente->fd,
        solicitud.method,
        solicitud.path,
        resultado
    );

    size_t consumidos = (size_t)(fin - cliente->buffer) + 4;

    size_t restantes = cliente->usados - consumidos;

    memmove(
        cliente->buffer,
        cliente->buffer + consumidos,
        restantes
    );

    cliente->usados = restantes;

    cliente->buffer[restantes] = '\0';

    return preparar_respuesta(
        epfd,
        cliente,
        resultado,
        persistente,
        solicitud.path
    );
}


static void aceptar_clientes(int epfd, int servidor) {

    while (1) {

        int fd = accept(servidor, NULL, NULL);

        if (fd == -1) {

            if (errno == EINTR) {
                continue;
            }

            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                break;
            }

            perror("accept");
            break;
        }

        if (hacer_no_bloqueante(fd) == -1) {
            perror("fcntl cliente");
            close(fd);
            continue;
        }

        Cliente *cliente = calloc(1, sizeof(Cliente));

        if (cliente == NULL) {
            perror("calloc");
            close(fd);
            continue;
        }

        cliente->fd = fd;

        cliente->archivo_fd = -1;

        struct epoll_event evento = {0};

        evento.events = EPOLLIN;
        evento.data.ptr = cliente;

        if (epoll_ctl(
                epfd,
                EPOLL_CTL_ADD,
                fd,
                &evento
            ) == -1) {

            perror("epoll_ctl cliente");

            close(fd);
            free(cliente);
            continue;
        }

        printf("Cliente conectado: %d\n", fd);
    }
}


static int leer_cliente(int epfd, Cliente *cliente) {

    while (1) {

        ssize_t n = recv(
            cliente->fd,
            cliente->buffer + cliente->usados,
            TAM_BUFFER - cliente->usados,
            0
        );

        if (n > 0) {

            cliente->usados += (size_t)n;

            cliente->buffer[cliente->usados] = '\0';

            int resultado = procesar_buffer(
                epfd,
                cliente
            );

            if (resultado != 0) {
                return resultado;
            }

            continue;
        }

        if (n == 0) {
            return -1;
        }

        if (errno == EINTR) {
            continue;
        }

        if (errno == EAGAIN || errno == EWOULDBLOCK) {
            return 0;
        }

        perror("recv");

        return -1;
    }
}

static int enviar_cliente(Cliente *cliente) {

    while (cliente->enviados < cliente->longitud_cabecera) {

        ssize_t n = send(
            cliente->fd,
            cliente->cabecera + cliente->enviados,
            cliente->longitud_cabecera - cliente->enviados,
            MSG_NOSIGNAL
        );

        if (n > 0) {

            cliente->enviados += (size_t)n;

            continue;
        }

        if (n == -1 && errno == EINTR) {
            continue;
        }

        if (n == -1 &&
            (errno == EAGAIN || errno == EWOULDBLOCK)) {

            return 0;
        }

        return -1;
    }

    if (cliente->archivo_fd == -1) {
        return 1;
    }

    while (cliente->offset_archivo < cliente->tamano_archivo) {

        off_t restantes =
            cliente->tamano_archivo - cliente->offset_archivo;

        size_t cantidad = 65536;

        if (restantes < (off_t)cantidad) {
            cantidad = (size_t)restantes;
        }

        ssize_t n = sendfile(
            cliente->fd,
            cliente->archivo_fd,
            &cliente->offset_archivo,
            cantidad
        );

        if (n > 0) {
            continue;
        }

        if (n == 0) {
            return -1;
        }

        if (errno == EINTR) {
            continue;
        }

        if (errno == EAGAIN || errno == EWOULDBLOCK) {
            return 0;
        }

        perror("sendfile");

        return -1;
    }

    return 1;
}

static int reanudar_cliente(int epfd, Cliente *cliente) {

    if (cliente->archivo_fd >= 0) {

        close(cliente->archivo_fd);

        cliente->archivo_fd = -1;
    }

    cliente->longitud_cabecera = 0;
    cliente->enviados = 0;

    cliente->tamano_archivo = 0;
    cliente->offset_archivo = 0;

    if (cliente->usados > 0) {

        int resultado = procesar_buffer(
            epfd,
            cliente
        );

        if (resultado != 0) {
            return resultado;
        }
    }

    struct epoll_event evento = {0};

    evento.events = EPOLLIN;
    evento.data.ptr = cliente;

    if (epoll_ctl(
            epfd,
            EPOLL_CTL_MOD,
            cliente->fd,
            &evento
        ) == -1) {

        return -1;
    }

    return 0;
}

int main(void) {

    if (signal(SIGPIPE, SIG_IGN) == SIG_ERR) {
        perror("signal SIGPIPE");
        return 1;
    }

    int servidor = iniciar_servidor(8080);

    if (servidor == -1) {

        return 1;
    }

    if (hacer_no_bloqueante(servidor) == -1) {
        perror("fcntl servidor");
        close(servidor);
        return 1;
    }

    int epfd = epoll_create1(0);

    if (epfd == -1) {
        perror("epoll_create1");
        close(servidor);
        return 1;
    }

    struct epoll_event evento = {0};

    evento.events = EPOLLIN;
    evento.data.ptr = NULL;

    if (epoll_ctl(
            epfd,
            EPOLL_CTL_ADD,
            servidor,
            &evento
        ) == -1) {

        perror("epoll_ctl servidor");
        close(servidor);
        close(epfd);
        return 1;
    }

    printf("Servidor concurrente con epoll iniciado\n");

    struct epoll_event eventos[MAX_EVENTS];

    while (1) {

        int cantidad = epoll_wait(
            epfd, 
            eventos, 
            MAX_EVENTS, 
            -1
        );
    
        if (cantidad == -1) {

            if (errno == EINTR) {
                continue;
            }

            perror("epoll_wait");
            break;
        }

        for (int i = 0; i < cantidad; i++) {

            Cliente *cliente = eventos[i].data.ptr;
            uint32_t flags = eventos[i].events;
            if (cliente == NULL) {

                aceptar_clientes(epfd, servidor);

                continue;
            }

            if (flags & EPOLLERR) {

                cerrar_cliente(epfd, cliente);

                continue;
            }
            if (cliente->longitud_cabecera == 0 && (flags & EPOLLIN)) {

                if (leer_cliente(epfd, cliente) == -1) {

                    cerrar_cliente(epfd, cliente);
                }

                continue;
            }

            if (cliente->longitud_cabecera > 0 && (flags & EPOLLOUT)) {

                int resultado = enviar_cliente(cliente);

                if (resultado == -1) {

                    cerrar_cliente(epfd, cliente);

                } else if (resultado == 1) {

                    if (!cliente->mantener_conexion) {

                        cerrar_cliente(epfd, cliente);

                    } else {

                        if (reanudar_cliente(epfd, cliente) == -1) {

                            cerrar_cliente(epfd, cliente);
                        }
                    }
                }

                continue;
            }

            if (flags & EPOLLHUP) {

                cerrar_cliente(epfd, cliente);

            }
        }
    }

    close(servidor);
    close(epfd);

    return 0;
}