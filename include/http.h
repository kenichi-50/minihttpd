#ifndef HTTP_H
#define HTTP_H

#include <stddef.h>
#include <sys/types.h>

typedef struct {
    char method[8];
    char path[256];
    char version[16];
    char host[256];
    char connection[32];
    char user_agent[256];

} SolicitudHTTP;

int analizar_solicitud(
    const char *buffer,
    SolicitudHTTP *solicitud
);

int construir_cabecera_http(
    char *destino,
    size_t capacidad,
    int codigo,
    const char *tipo,
    off_t tamano,
    int persistente
);

#endif