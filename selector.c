#include <stdio.h>
#include <stdlib.h>

int main() {
    FILE *muestra;                //aqui le digo cual es el puntero 
    int num;                      // número del archivo
    char nombre[200];             // guarda el nombre del archivo
    char buffer[100];             // leer en bloques de hasta 99
    size_t bytesRead;             

    printf("Numero del archivo: ");
    if (scanf("%d", &num) != 1) {  //pide al usuario el numero y lo verfica
        printf("Entrada invalida.\n");
        return 1;
    }
    
    sprintf(nombre, "C:/Users/jdjh0/Documents/embebidos/laboratorio1/archivos/archivo_%04d.txt", num);// Construir la ruta completa del archivo

    // apertura del archivo
    muestra = fopen(nombre, "r"); //usa la ruta ya creada y lo abre
    if (muestra == NULL) {
        printf("Error: No se pudo abrir el archivo %s\n", nombre);//si el archivo no existe 
        return 1;
    }

    // Leer e imprimir contenido
    while ((bytesRead = fread(buffer, 1, sizeof(buffer) - 1, muestra)) > 0) {
        buffer[bytesRead] = '\0'; // asegurar terminación de cadena
        printf("%s", buffer);
    }

    fclose(muestra);
    return 0;
}
