#ifndef HTTP_H
#define HTTP_H

typedef struct {
    char method[8];
    char path[256];
    char version[16];
    char host[256];
    char connection[32];
    char user_agent[256];

} SolicitudHTTP;

int analizar_solicitud(const char *buffer, SolicitudHTTP *solicitud);
const char *obtener_respuesta_http(int resultado);

#endif 