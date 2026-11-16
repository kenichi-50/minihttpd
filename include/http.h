#ifndef HTTP_H
#define HTTP_H

typedef struct {
    char method[8];
    char path[256];
    char version[16];
} SolicitudHTTP;

int analizar_solicitud(const char *buffer, SolicitudHTTP *solicitud);

#endif 