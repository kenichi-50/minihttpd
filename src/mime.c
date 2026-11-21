#include <string.h>
#include <strings.h>
#include "mime.h"

const char *obtener_mime(const char *ruta){
    if (ruta == NULL) {
        return "application/octet-stream";
    }

    size_t longitud = strcspn(ruta, "?#");

    if (longitud == 1 && ruta[0] == '/'){
        return "text/html";
    }

    const char *extension = NULL;

    for (size_t i = longitud; i > 0; i--){
        if (ruta[i - 1] == '/'){
            break;
        }
        
        if (ruta[i - 1] == '.'){
            extension = ruta + i - 1;
            break;
        }
    }

    if (extension == NULL) {
        return "application/octet-stream";
    }
    
    size_t tam = longitud - (size_t)(extension - ruta);

    if (tam == 5 && strncasecmp(extension, ".html", 5) == 0){
        return "text/html";
    }

    if (tam == 4 && strncasecmp(extension, ".css", 4) == 0){
        return "text/css";
    }

    if (tam == 3 && strncasecmp(extension, ".js", 3) == 0){
        return "application/javascript";
    }

    if (tam == 4 && strncasecmp(extension, ".png", 4) == 0){
        return "image/png";
    }

    if (tam == 4 && strncasecmp(extension, ".jpg", 4) == 0){
        return "image/jpeg";
    }

    if (tam == 5 && strncasecmp(extension, ".jpeg", 5) == 0){
        return "image/jpeg";
    }

    return "application/octet-stream";

}