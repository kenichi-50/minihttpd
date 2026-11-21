#ifndef FILES_H
#define FILES_H

#include <sys/types.h>

int abrir_archivo(
    const char *ruta,
    int *fd,
    off_t *tamano
);

#endif