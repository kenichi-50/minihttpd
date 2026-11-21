#define _POSIX_C_SOURCE 200809L
#define _XOPEN_SOURCE 700

#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include "files.h"

static int error_archivo(int error) {
    if (error == EACCES || error == EPERM || error == ELOOP) {
        return 403;
    }

    if (error == ENOENT || error == ENOTDIR) {
        return 404;
    }

    return 500;
}

static int tiene_componentes_prohibidos(const char *nombre) {
    const char *parte = nombre;

    while (*parte != '\0') {
        const char *barra = strchr(parte, '/');
        size_t tamano;

        if (barra == NULL) {
            tamano = strlen(parte);
        } else {
            tamano = (size_t)(barra - parte);
        }

        if ((tamano == 1 && parte[0] == '.') ||
            (tamano == 2 && parte[0] == '.' && parte[1] == '.')) {
            return 1;
        }

        if (barra == NULL) {
            break;
        }

        parte = barra + 1;
    }

    return 0;
}

int abrir_archivo(const char *ruta, int *fd, off_t *tamano) {
    if (ruta == NULL || fd == NULL || tamano == NULL || ruta[0] != '/') {
        return 400;
    }

    char nombre[512];
    size_t longitud = strcspn(ruta, "?");

    if (longitud == 1) {
        memcpy(nombre, "index.html", sizeof("index.html"));
    } else {
        if (longitud < 2 || longitud > sizeof(nombre)) {
            return 400;
        }

        if (ruta[longitud - 1] == '/') {
            return 404;
        }

        memcpy(nombre, ruta + 1, longitud - 1);
        nombre[longitud - 1] = '\0';
    }

    if (strchr(nombre, '#') != NULL ||
        strchr(nombre, '\\') != NULL ||
        strchr(nombre, '%') != NULL ||
        strstr(nombre, "//") != NULL) {
        return 400;
    }

    if (tiene_componentes_prohibidos(nombre)) {
        return 403;
    }


    char raiz[PATH_MAX];
    char ruta_completa[PATH_MAX];
    char ruta_real[PATH_MAX];

    if (realpath("www", raiz) == NULL) {
        return 500;
    }

    int n = snprintf(
        ruta_completa,
        sizeof(ruta_completa),
        "%s/%s",
        raiz,
        nombre
    );

    if (n < 0 || (size_t)n >= sizeof(ruta_completa)) {
        return 400;
    }

    if (realpath(ruta_completa, ruta_real) == NULL) {
        return error_archivo(errno);
    }

    size_t longitud_raiz = strlen(raiz);

    if (strncmp(ruta_real, raiz, longitud_raiz) != 0 ||
        ruta_real[longitud_raiz] != '/') {
        return 403;
    }

    int actual = open(
        "www",
        O_RDONLY | O_DIRECTORY | O_CLOEXEC | O_NOFOLLOW
    );

    if (actual == -1) {
        return 500;
    }

    char *estado = NULL;
    char *parte = strtok_r(nombre, "/", &estado);

    if (parte == NULL) {
        close(actual);
        return 400;
    }

    while (parte != NULL) {
        char *siguiente = strtok_r(NULL, "/", &estado);
        int opciones = O_RDONLY | O_CLOEXEC | O_NOFOLLOW;

        if (siguiente != NULL) {
            opciones |= O_DIRECTORY;
        } else {
            opciones |= O_NONBLOCK;
        }

        int nuevo = openat(actual, parte, opciones);
        int error = errno;
        close(actual);

        if (nuevo == -1) {
            return error_archivo(error);
        }

        actual = nuevo;
        parte = siguiente;
    }

    struct stat info;

    if (fstat(actual, &info) == -1) {
        close(actual);
        return 500;
    }

    if (!S_ISREG(info.st_mode)) {
        close(actual);
        return 403;
    }

    if (info.st_size < 0) {
        close(actual);
        return 500;
    }

    *fd = actual;
    *tamano = info.st_size;
    return 200;
}
