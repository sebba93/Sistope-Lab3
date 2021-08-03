#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

int main(int argc, char* argv[]){
    //Revisar los datos de entrada

    //

    int numeroHebras = 4;

    pthread_t numeroHebras[cantidadHebras];

    int cantidadFilas = 45;
    int cantidadColumnas = 67;

    //Flag 0 = par, flag 1 = impar
    int flagHebras;

    //Revisamos la cantidad de filas que van a existir por hebra
    if (numeroFilas%numeroHebras == 0){
        flagHebras = 0;
    }
    else {
        flagHebras = 1;
    }
    

    

    

    for (int i; i < numeroHebras; i++){
        if()
    }
}