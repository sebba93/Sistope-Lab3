#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>


//Funcion que lee un archivo .raw y lo almacena como una matriz de floats
//ENTRADAS : 
//char* nombre_archivo - Nombre del archivo a leer
//int filas - Numero de filas de pixeles de la imagen
//int columnas - Numero de columnas de pixeles de la imagen
//SALIDAS :
//float** matriz_imagen - Una matriz de float con los valores de los pixeles
float** leerArchivo(char* nombre_archivo, int filas, int columnas){
    int file;
    int i,j,k;
    int tamano = filas*columnas; 
    size_t file_r;
    float* buffer = (float*)malloc(tamano*sizeof(float));
    if (buffer == NULL){
        printf("No se pudo reservar memoria\n");
        exit(-1);
    }
    //Comprobamos que la imagen se puede abrir.
    file = open(nombre_archivo, O_RDONLY);    
    if (file == -1){
        printf("No se pudo abrir la imagen.");
        return 0;
    }
    printf("%d",file);
    printf("Se abrio la imagen\n");
    //Comprobamos que la imagen se puede leer.
    file_r = read(file, buffer, tamano*sizeof(float));  
    if(file_r != tamano*sizeof(float)){
        printf("No se puedo leer el archivo.");
        return 0;
    }
    printf("Se leyo el archivo\n");
    float** matriz_imagen = (float **) malloc (filas*sizeof(float*));

    
    for (i = 0; i < filas; i++){
    	matriz_imagen[i]=(float *) malloc (columnas*sizeof(float));
    }

    if (matriz_imagen == NULL){
        printf("No se pudo reservar memoria\n");
        exit(-1);
    }

    //Guardamos los datos de la imagen como float en la matriz.
    
    k = 0;
    for (i = 0; i<filas ;i++){
        for(j = 0; j<columnas ;j++){
            matriz_imagen[i][j] = buffer[k];
            k++;
        }
    }
    

    close(file);

    free(buffer);
    
    return matriz_imagen;
}

//Funcion que toma una matriz float** crea una nueva matriz de float** con un efecto ZOOM
//ENTRADAS : 
//int filas - Numero de filas de pixeles de la imagen
//int columnas - Numero de columnas de pixeles de la imagen
//float** matriz - La matriz que representa la imagen que sera ZOOMEADA
//SALIDAS :
//float** matriz_zoom - Una matriz de float con los valores de los pixeles ZOOMEADOS
float** zoomImagen(int columnas, int filas, int factor, float** matriz){
    int i,j,k,m,n;
    float* buffer;
    float** matriz_zoom;
	
    //Se define un arreglo de tamaño de la matriz zoomeada
	buffer = (float*)malloc((factor*factor*filas*columnas)*sizeof(float));

    //Se define una matriz de float que representará la imágen
    
    matriz_zoom = (float**)malloc((filas*factor)*sizeof(float*));
    
	for (i = 0; i < (filas*factor); i++){
		matriz_zoom[i] = (float*)malloc((columnas*factor)*sizeof(float));
	}
    
    //Se guarda la imagen en ZOOM en un arreglo lineal
    k = 0;
    for (i = 0; i<filas ;i++){
        for(n = 0; n<factor ;n++){
            for(j = 0; j<columnas ;j++){
                for(m = 0; m<factor; m++){
                    buffer[k] = matriz[i][j];
                    k++;
                }
            }
        }
    }
    
    //Se guarda la imagen zoomeada en una matriz
    k = 0;
    for (i = 0; i<(filas*factor) ;i++){
        for(j = 0; j<(columnas*factor) ;j++){
            matriz_zoom[i][j] = buffer[k];
            k++;
        }
    }

    free(buffer);

    return matriz_zoom;
}

//Funcion que toma una buffer float* y escribe un archivo binario
//ENTRADAS : 
//float* buffer - El buffer que representa la imágen
//int x
//int y
//char *nombreImagen
//SALIDAS :
//int - Un int que comprueba la ejecucion del algoritmo
int escritura(float *buffer, int x, int y, char *nombreImagen){
    ssize_t file = open(nombreImagen, O_CREAT|O_RDWR|O_NONBLOCK|__O_LARGEFILE, S_IRWXO|S_IRWXU|S_IRWXG);

    //Permisos para crear el archivo
    if (file == -1){
        printf("No se pudo abrir la imagen\n");
        exit(-1);
    }

    //Escribimos el buffer en el archivo abierto
    size_t escritura = write(file, buffer, sizeof(float)*x*y);

    //Cerramos el archivo
    close(file);

    return 1;
}


//Aplica el filtro de suavizado a una matriz entregada
//ENTRADAS :
//int x - Entra el alto de la imagen
//int y - Ancho de la imagen
//float **imagen - Matriz donde se encuentran los pixeles de la imagen
//SALIDA :
//float *nuevaImagen - Un buffer con los pixeles que se desean anotar al escribir
float *filtroSuavizado(int x, int y, float **imagen){
	
    //Creamos el buffer para su escritura
    float* nuevaImagen = (float*)malloc(sizeof(float)*x*y);

    
    if (nuevaImagen == NULL){
        printf("Error al alocar memoria\n");
        exit(-1);
    }

    float numAux;
    float nuevoPixel;

    int contadorBuffer = 0;

    for (int i = 0; i < x; i++){
        for (int j = 0; j < y; j++){
            
            //Caso superior izquierdo
            if (i == 0 && j == 0){
                numAux = imagen[0][1] + imagen[1][0] + imagen[1][1];
                nuevoPixel = numAux / 3;
            }
            //Caso superior derecho
            else if (i == 0 && j == y - 1){
                numAux = imagen[0][y - 1] + imagen[1][y - 1] + imagen[1][y - 2];
                nuevoPixel = numAux / 3; 
            }
            //Caso inferior izquierdo
            else if (i == x - 1 && j == 0){
                numAux = imagen[x - 2][0] + imagen[x - 2][1] + imagen[x - 1][1];
                nuevoPixel = numAux / 3;
            }
            //Caso inferior derecho
            else if (i == x - 1 && j == y - 1){
                numAux = imagen[x - 2][y - 2] + imagen[x - 2][y - 1] + imagen[x - 1][y - 2];
                nuevoPixel = numAux / 3;
            }
            //Caso izquierdo
            else if(j == 0 && i != 0 && i != x-1){
                numAux = imagen[i - 1][0] + imagen[i + 1][0] + imagen[i - 1][1] + imagen[i][1] + imagen[i + 1][1];
                nuevoPixel = numAux / 5;
            }
            //Caso superior
            else if(i == 0 && j != 0 && j != y-1){
                numAux = imagen[0][j - 1] + imagen[0][j + 1] + imagen[1][j - 1] + imagen[1][j] + imagen[1][j + 1];
                nuevoPixel = numAux / 5;
            }
            //Caso derecho
            else if(i != 0 && i != x-1 && j == y - 1){
                numAux = imagen[i - 1][y - 2] + imagen[i][y - 2] + imagen[i + 1][y - 2] + imagen[i - 1][y - 1] + imagen[i + 1][y - 1];
                nuevoPixel = numAux / 5;
            }
            //Caso inferior
            else if(i == x - 1 && j != 0 && j != y-1){
                numAux = imagen[x - 2][j - 1] + imagen[x - 2][j] + imagen[x - 2][j + 1] + imagen[x - 1][j - 1] + imagen[x - 1][j + 1];
                nuevoPixel = numAux / 5;
            }
            //Caso general
            else{
                numAux = imagen[i - 1][j - 1] + imagen[i - 1][j] + imagen[i - 1][j + 1] + imagen[i][j - 1] + imagen[i][j + 1] + imagen[i + 1][j - 1] + imagen[i + 1][j] + imagen[i + 1][j + 1];
                nuevoPixel = numAux / 8;
            }

            //Agregamos el pixel a la nueva matriz
            
            nuevaImagen[contadorBuffer] = nuevoPixel;
            contadorBuffer++;
        }
    }




    return nuevaImagen;
    
}

//Convierte una matriz en un buffer unidimensional para poder escribirlo en un archivo binario
//ENTRADAS : 
//float **imagen - Matriz con lo imagen a convertir
//int x - Numero de filas de pixeles de la imagen
//int y - Numero de columnas de pixeles de la imagen
//float *bufferImagen - Buffer previamente inicializado unidimensional
//SALIDAS :
//float* bufferImagen - Buffer con la imagen en un arreglo unidimensional
float *convertirBuffer(float **imagen, int x, int y, float *bufferImagen){
    int espacio = 0;
    for (int i = 0; i < x; i++){
        for (int j = 0; j < y; j++){
            bufferImagen[espacio] = imagen[i][j];
            espacio++;
        } 
    }
    return bufferImagen;
}

//Aplica la transformación de delineado a una matriz entregada
//ENTRADAS :
//int columnas- Entra el alto de la imagen
//int filas - Ancho de la imagen
//float **matriz - Matriz donde se encuentran los pixeles de la imagen
//SALIDA :
//float **matriz_delineado - Un buffer con los pixeles que se desean anotar al escribir
float** deliniado(int columnas, int filas, float** matriz){
    int i,j,k,l;
    float* buffer;
    float** matriz_delineado;
	
    //Se define un arreglo de tamaño de la matriz zoomeada
	buffer = (float*)malloc((filas*columnas)*sizeof(float));

    //Se define una matriz de float que representará la imágen
    
    matriz_delineado = (float**)malloc((filas)*sizeof(float*));
    
	for (i = 0; i < (filas); i++){
		matriz_delineado[i] = (float*)malloc((columnas)*sizeof(float));
	}

    for (k = 0; k < filas; k++){
        for (l = 0; l < columnas; l++){

            if(k == 0 || l == 0 || k == (filas-1) || l == (columnas-1) ){
                matriz_delineado[k][l] = matriz[k][l];
            }
            else{                              
                matriz_delineado[k][l] = (matriz[k-1][l-1]*-1) + (matriz[k-1][l]*-1) + (matriz[k-1][l+1]*-1) + (matriz[k][l-1]*-1) + (matriz[k][l]*8) + (matriz[k][l+1]*-1) + (matriz[k+1][l-1]*-1) + (matriz[k+1][l]*-1) + (matriz[k+1][l+1]*-1);
            }
        }
    }

    return matriz_delineado;

}

