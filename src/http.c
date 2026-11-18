#include <stdio.h>
#include <string.h>
#include "http.h"
#include <strings.h>

int analizar_solicitud(const char *buffer, SolicitudHTTP *solicitud){

    const char *fin = strstr(buffer, "\r\n");

    const char *fin_cabeceras = strstr(buffer, "\r\n\r\n");

    if (fin == NULL || fin_cabeceras == NULL) {
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

    if (strcmp(solicitud->version, "HTTP/1.1") != 0) {
        return -1;
    }

    if (solicitud->path[0] != '/') {
        return -1;
    }

    const char *actual = fin + 2;

    int host_encontrado = 0;

    while (actual < fin_cabeceras) {

        const char *fin_linea = strstr(actual, "\r\n");

        if (fin_linea == NULL || fin_linea > fin_cabeceras) {
            return -1;
        }

        const char *separador = memchr(actual, ':', fin_linea - actual);

        if (separador == NULL || separador == actual) {
            return -1;
        }

        size_t nombre = separador - actual;
        char *destino = NULL;
        size_t capacidad = 0;

        if (nombre == 4 && strncasecmp(actual, "Host", 4) == 0) {

            if (host_encontrado) {
                return -1;
            }

            destino = solicitud->host;
            capacidad = sizeof(solicitud->host);
            host_encontrado = 1;

        } else if (nombre == 10 && strncasecmp(actual, "Connection", 10) == 0) {

            destino = solicitud->connection;
            capacidad = sizeof(solicitud->connection);

        } else if (nombre == 10 && strncasecmp(actual, "User-Agent", 10) == 0) {

            destino = solicitud->user_agent;
            capacidad = sizeof(solicitud->user_agent);
        }

        if (destino != NULL) {

            const char *valor = separador + 1;

            while (valor < fin_linea && (*valor == ' ' || *valor == '\t')) {
                valor++;
            }

            size_t longitud = fin_linea - valor;

            while (longitud > 0 &&
                (valor[longitud - 1] == ' ' || valor[longitud - 1] == '\t')) {
                longitud--;
            }

            if (longitud == 0 || longitud >= capacidad) {
                return -1;
            }

            memcpy(destino, valor, longitud);
            destino[longitud] = '\0';
        }


        actual = fin_linea + 2;
    }

    if (!host_encontrado) {
        return -1;
    }

    if (strcmp(solicitud->method, "GET") != 0) {
        return -2;
    }

    return 0;
}





const char *obtener_respuesta_http(int resultado) {

    if (resultado == -1) {
        return "HTTP/1.1 400 Bad Request\r\n"
               "Content-Length: 0\r\n"
               "Connection: close\r\n"
               "\r\n";
    }

    if (resultado == -2) {
        return "HTTP/1.1 405 Method Not Allowed\r\n"
               "Allow: GET\r\n"
               "Content-Length: 0\r\n"
               "Connection: close\r\n"
               "\r\n";
    }

    return "HTTP/1.1 200 OK\r\n"
           "Content-Type: text/plain\r\n"
           "Content-Length: 6\r\n"
           "Connection: close\r\n"
           "\r\n"
           "Hola!\n";
}


