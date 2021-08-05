#ifndef FUNCIONES_H_INCLUDED
#define FUNCIONES_H_INCLUDED

//Lectura de Archivo
float** leerArchivo(char* nombre_archivo, int filas, int columnas);

//Zoom de Imagen
float** zoomImagen(int columnas, int filas, int factor, float** matriz);

//Escritura de Archivo
int escritura(float *buffer, int x, int y, char *nombreImagen);

//Convertir matriz a buffer para ser escrita
float *convertirBuffer(float **imagen, int x, int y, float *bufferImagen);

//Suavizados
float *suavizadoPrimero(int x, int y, float **imagen, float **bufferSiguiente, int yBuffer);
float *suavizadoUltimo(int x, int y, float **imagen, float **bufferAnterior, int yBuffer);
float *suavizadoMedio(int x, int y, float **imagen, float **bufferAnterior, int yBufferA, float **bufferSiguiente, int yBufferS);

//Delineado
float** deliniado(int columnas, int filas, float** matriz);
float *delineadoPrimero(int x, int y, float **imagen, float **bufferSiguiente, int xBuffer, int yBuffer);
float *delineadoUltimo(int x, int y, float **imagen, float **bufferAnterior, int xBuffer, int yBuffer);
float *delineadoMedio(int x, int y, float **imagen, float **bufferAnterior, int xBufferA, int yBufferA, float **bufferSiguiente, int xBufferS, int yBufferS);

#endif