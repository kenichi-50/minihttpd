# MiniHTTPd

MiniHTTPd es un servidor HTTP/1.1 desarrollado en C para Linux. Utiliza sockets TCP, concurrencia mediante epoll y conexiones persistentes para atender múltiples clientes y servir archivos estáticos.

## Estructura del proyecto

```text
minihttpd/
├── Makefile
├── README.md
├── include/
│   ├── server.h
│   ├── http.h
│   ├── files.h
│   └── mime.h
├── src/
│   ├── main.c
│   ├── server.c
│   ├── http.c
│   ├── files.c
│   └── mime.c
└── www/
    ├── index.html
    ├── style.css
    └── image.png
```

## Flujo de ejecución

El servidor sigue el siguiente flujo de procesamiento:

1. **server.c:** crea el socket TCP, establece el puerto y habilita la escucha de conexiones.
2. **main.c:** utiliza epoll para aceptar clientes y gestionar los eventos de entrada y salida.
3. **http.c:** analiza las solicitudes HTTP, el método GET y los encabezados.
4. **files.c:** valida las rutas mediante realpath() y accede a los archivos del directorio www/.
5. **mime.c:** identifica el tipo de contenido del archivo solicitado.
6. **http.c:** construye la cabecera de respuesta HTTP.
7. **main.c:** envía la respuesta mediante send() y sendfile(). Finalmente, mantiene o cierra la conexión según corresponda.

**Flujo general:**

`Cliente → Socket TCP → epoll → HTTP → Archivos → MIME → Respuesta HTTP → Cliente`