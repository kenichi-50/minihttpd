#include <stdio.h>
#include <string.h>
#include "http.h"

int analizar_solicitud(const char *buffer, SolicitudHTTP *solicitud){
    const char *fin = strstr(buffer, "\r\n");
    if (fin == NULL) {
        return -1; 
    }

    char linea[512];
    size_t longitud = fin - buffer;
    if (longitud == 0 || longitud >= sizeof(linea)) {
        return -1; 
    }

    memcpy(linea, buffer, longitud);
    linea[longitud] = '\0';

    char extra;

    int resultado = sscanf(linea, "%7s %255s %15s%c", 
        solicitud->method, 
        solicitud->path,
        solicitud->version,
        &extra);
    
    if (resultado != 3) {
        return -1; 
    }

    if (strcmp(solicitud->method, "GET") != 0) {
        return -2;
    }
    
    return 0;

}