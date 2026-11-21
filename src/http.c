#include <stdio.h>
#include <string.h>
#include <strings.h>
#include "http.h"

int analizar_solicitud(const char *buffer, SolicitudHTTP *solicitud) {

    if (buffer == NULL || solicitud == NULL) {
        return -1;
    }

    const char *fin = strstr(buffer, "\r\n");
    const char *fin_cabeceras = strstr(buffer, "\r\n\r\n");

    if (fin == NULL || fin_cabeceras == NULL || fin > fin_cabeceras) {
        return -1;
    }

    char linea[512];
    size_t longitud = (size_t)(fin - buffer);

    if (longitud == 0 || longitud >= sizeof(linea)) {
        return -1;
    }

    memcpy(linea, buffer, longitud);
    linea[longitud] = '\0';

    char extra;
    int resultado = sscanf(
        linea,
        "%7s %255s %15s%c",
        solicitud->method,
        solicitud->path,
        solicitud->version,
        &extra
    );

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
    int connection_encontrada = 0;
    int cuerpo_no_soportado = 0;

    while (actual < fin_cabeceras) {

        const char *fin_linea = strstr(actual, "\r\n");

        if (fin_linea == NULL || fin_linea > fin_cabeceras) {
            return -1;
        }

        const char *separador = memchr(
            actual,
            ':',
            (size_t)(fin_linea - actual)
        );

        if (separador == NULL || separador == actual) {
            return -1;
        }

        size_t nombre = (size_t)(separador - actual);
        char *destino = NULL;
        size_t capacidad = 0;

        if ((nombre == 14 &&
            strncasecmp(actual, "Content-Length", 14) == 0) ||
            (nombre == 17 &&
            strncasecmp(actual, "Transfer-Encoding", 17) == 0)) {
            cuerpo_no_soportado = 1;
        }

        if (nombre == 4 && strncasecmp(actual, "Host", 4) == 0) {

            if (host_encontrado) {
                return -1;
            }

            destino = solicitud->host;
            capacidad = sizeof(solicitud->host);
            host_encontrado = 1;

        } else if (nombre == 10 &&
            strncasecmp(actual, "Connection", 10) == 0) {

            if (connection_encontrada) {
                return -1;
            }

            destino = solicitud->connection;
            capacidad = sizeof(solicitud->connection);
            connection_encontrada = 1;

        } else if (nombre == 10 &&
            strncasecmp(actual, "User-Agent", 10) == 0) {

            destino = solicitud->user_agent;
            capacidad = sizeof(solicitud->user_agent);
        }

        if (destino != NULL) {

            const char *valor = separador + 1;

            while (valor < fin_linea &&
                (*valor == ' ' || *valor == '\t')) {
                valor++;
            }

            size_t longitud_valor = (size_t)(fin_linea - valor);

            while (longitud_valor > 0 &&
                (valor[longitud_valor - 1] == ' ' ||
                 valor[longitud_valor - 1] == '\t')) {
                longitud_valor--;
            }

            if (longitud_valor == 0 || longitud_valor >= capacidad) {
                return -1;
            }

            memcpy(destino, valor, longitud_valor);
            destino[longitud_valor] = '\0';
        }

        actual = fin_linea + 2;
    }

    if (!host_encontrado) {
        return -1;
    }

    if (strcmp(solicitud->method, "GET") != 0) {
        return -2;
    }

    if (cuerpo_no_soportado) {
        return -1;
    }

    return 0;
}

int construir_cabecera_http(
    char *destino,
    size_t capacidad,
    int codigo,
    const char *tipo,
    off_t tamano,
    int persistente
) {
    if (destino == NULL || capacidad == 0 || tipo == NULL || tamano < 0) {
        return -1;
    }

    const char *motivo;

    switch (codigo) {
        case 200:
            motivo = "OK";
            break;
        case 400:
            motivo = "Bad Request";
            break;
        case 403:
            motivo = "Forbidden";
            break;
        case 404:
            motivo = "Not Found";
            break;
        case 405:
            motivo = "Method Not Allowed";
            break;
        default:
            codigo = 500;
            motivo = "Internal Server Error";
            break;
    }

    if (codigo != 200) {
        tipo = "text/plain";
        tamano = 0;
        persistente = 0;
    }

    const char *permitidos = "";

    if (codigo == 405) {
        permitidos = "Allow: GET\r\n";
    }

    int n = snprintf(
        destino,
        capacidad,
        "HTTP/1.1 %d %s\r\n"
        "Content-Type: %s\r\n"
        "%s"
        "Content-Length: %lld\r\n"
        "Connection: %s\r\n"
        "\r\n",
        codigo,
        motivo,
        tipo,
        permitidos,
        (long long)tamano,
        persistente ? "keep-alive" : "close"
    );

    if (n < 0 || (size_t)n >= capacidad) {
        return -1;
    }

    return n;
}
