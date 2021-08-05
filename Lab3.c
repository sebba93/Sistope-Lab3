#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "funciones.h"
#include <pthread.h>

//Structs basados en ejemplo dado en clases//

typedef struct {
    int filas;
    int columnas;

    int **buffer;
    int in, out;
    int full, empty;

    int tamanoBuffer;

    char *nombreArchivo; //Necesario dado que vamos a pasar buffer_t como argumento al crear las hebras

    pthread_mutex_t mutex;
    pthread_cond_t notFull, notEmpty;
} buffer_t;

typedef struct {
    int id;
    int cantidadHebras;
    int filasPorHebra;
    int columnas;
    int factor;
    char *nombreArchivoSalida;
    pthread_mutex_t * mutex;
    buffer_t * buffer;
    buffer_t * bufferAux;
    pthread_barrier_t * barrera;
} consumer_t;



buffer_t * bufferInit(int filas, int columnas, int tamanoBuffer, char* nombreArchivo){
    buffer_t * buf=(buffer_t*)malloc(sizeof(buffer_t));
    buf->in=0;
    buf->out=0;
    buf->full=0;
    buf->empty=1;
    buf->filas=filas;
    buf->columnas=columnas;

    buf->tamanoBuffer = tamanoBuffer;
    buf->nombreArchivo = nombreArchivo;

    buf->buffer=(int**)malloc(sizeof(int*)*filas);
    for(int i=0;i<filas;i++){
        buf->buffer[i]=(int*)malloc(sizeof(int)*columnas);
    }
    return buf;
}

void put_in_buffer(buffer_t * buf,int * fila, int columnas){
    int in=buf->in;
    for(int j=0;j<columnas;j++){
        buf->buffer[in][j]=fila[j];
    }
    buf->in=in +1;
    if(buf->in>=buf->filas){
        buf->full=1;
        buf->empty=0;
        buf->in=0;
    }
}

int * take_from_buffer(buffer_t * buf,int columnas){
    int * retorno=(int*)malloc(sizeof(int)*columnas);
    for(int j=0;j<columnas;j++){
        retorno[j]=buf->buffer[buf->out][j];
    }
    buf->out=buf->out+1;
    if(buf->out>=buf->filas){
        buf->full=0;
        buf->empty=1;
        buf->out=0;
    }
    return retorno;
}


void *producer(void *arg){
    buffer_t *buffer;
    buffer = (buffer_t *) arg;
    int filas=buffer->filas;
    int columnas= buffer->columnas;

    char* nombre_archivo_in = buffer->nombreArchivo;

    //Procedemos a leer la imagen

    float **archivoLeido;

    archivoLeido = leerArchivo(nombre_archivo_in, filas, columnas);

    //Hacemos un buffer para poder pasar los datos de la imagen 
    float * bufferAux = (float*)malloc(sizeof(float)*columnas);

    //Ubicacion actual en el buffer
    //int ubBuff = 0;
    
    //int maxBuffer = buffer->tamanoBuffer;

    for (int i = 0; i < filas; i++){
        //Revisamos si es que llegamos al
        for (int j = 0; j < columnas; j++){
            //Le agregamos los elementos al buffer creado
            bufferAux[j] = archivoLeido[i][j];
            }
            //Una vez pasada la fila completa, pasamos a la seccion critica
            pthread_mutex_lock(&buffer->mutex);
            //Esperamos a que el buffer se descargue
            while (buffer->full == 1){
                pthread_cond_wait (&buffer->notFull, &buffer->mutex);
            }

            //Agregamos la columna al buffer
            put_in_buffer(buffer, bufferAux, columnas);

            //Esperamos a que se vacie
            pthread_cond_signal(&buffer->notEmpty);
            //Desbloqueamos el mutex y salimos de la seccion critica
            pthread_mutex_unlock(&buffer->mutex);
        
        

    }


}


void *consumer (void *arg){

    consumer_t * informacion;
    informacion=(consumer_t *) arg;

    buffer_t *buffer;
    buffer = informacion->buffer;

    pthread_barrier_t * barrera;
    barrera=informacion->barrera;

    pthread_mutex_t * mutex=informacion->mutex;

    int filasPorHebra=informacion->filasPorHebra;
    int columnas=informacion->columnas;

    float ** salida=(float**)malloc(sizeof(float*)*filasPorHebra);
    pthread_mutex_lock(&mutex[informacion->id]); //bloquea esta hebra
    int i=0;
    while (i<filasPorHebra) {
        pthread_mutex_lock (&buffer->mutex);
        while (buffer->empty) {
            pthread_cond_wait (&buffer->notEmpty, &buffer->mutex);
        }

        salida[i] = take_from_buffer(buffer,columnas);   //consume()

        pthread_cond_signal(&buffer->notFull);
        pthread_mutex_unlock(&buffer->mutex);
        i++;
    }

    if(informacion->id !=informacion->cantidadHebras - 1){ // desbloquea la siguiente hebra
        pthread_mutex_unlock(&mutex[informacion->id +1]);
    }
    else{
        pthread_mutex_unlock(&mutex[0]); //o la primera si esta era la ultima hebra
    }

    pthread_barrier_wait(barrera);  // barrera para sincronizacion

    pthread_mutex_lock(&mutex[informacion->id]);//bloquea esta hebra
    //print
    printf("llegue id: %d\n",informacion->id);
    for(i=0;i<filasPorHebra;i++){
        for(int j=0; j<columnas; j++){
            printf("%d ",salida[i][j]);
        }
        printf("\n");
    }
    
    if(informacion->id !=informacion->cantidadHebras - 1){// desbloquea la siguiente hebra
        pthread_mutex_unlock(&mutex[informacion->id +1]);
    }
    else{// desbloquea la primera hebra
        pthread_mutex_unlock(&mutex[0]);
    }

    //################################################//
    //######################ZOOM######################//
    //################################################//
    pthread_barrier_wait(barrera); //Barrera para sincronizacion

    //Se crea una matriz del tamaño del zoom pedido
    int fZoom = informacion->filasPorHebra*informacion->factor;
    int cZoom = informacion->columnas*informacion->factor;

    float **matriz_zoom=(float **)malloc(fZoom*sizeof(float*));
    for (int i = 0; i < fZoom; ++i){
        matriz_zoom[i]=(float *) malloc (cZoom*sizeof(float));
    }

    //Se llama a la función zoom
    matriz_zoom = zoomImagen(informacion->columnas, informacion->filasPorHebra, informacion->factor, salida);

    //################################################//
    //################################################//
    //################################################//





    ////////////////////////////filas anteriores
    pthread_barrier_wait(barrera);  // barrera para sincronizacion

    pthread_mutex_lock(&mutex[informacion->id]);//bloquea esta hebra
    // se envia al buffer de mensajes la ultima fila de la hebra, exepto la ultima hebra.
    if(informacion->id !=informacion->cantidadHebras - 1){
        put_in_buffer(informacion->bufferAux, matriz_zoom[fZoom-1],cZoom);
    }
    if(informacion->id !=informacion->cantidadHebras - 1){// desbloquea la siguiente hebra
        pthread_mutex_unlock(&mutex[informacion->id +1]);
    }
    else{// desbloquea la primera hebra
        pthread_mutex_unlock(&mutex[0]);
    }


    pthread_barrier_wait(barrera);  // barrera para sincronizacion
    int * filaAnterior=(float*)malloc(sizeof(float)*cZoom);
    pthread_mutex_lock(&mutex[informacion->id]);//bloquea esta hebra
    // se lee del buffer la fila anterior excepto la primera hebra.
    if(informacion->id !=0){
        filaAnterior=take_from_buffer(informacion->bufferAux, cZoom);
    }
    if(informacion->id !=informacion->cantidadHebras - 1){// desbloquea la siguiente hebra
        pthread_mutex_unlock(&mutex[informacion->id +1]);
    }
    else{// desbloquea la primera hebra
        pthread_mutex_unlock(&mutex[0]);
    }
    ////////////////////////////////////////////////////////////////////////////////////////////////////////

    //////////////////////////////////////Filas siguientes
    pthread_barrier_wait(barrera);  // barrera para sincronizacion

    pthread_mutex_lock(&mutex[informacion->id]);//bloquea esta hebra
    // se envia al buffer de mensajes la ultima fila de la hebra, exepto la ultima hebra.
    if(informacion->id !=0){
        put_in_buffer(informacion->bufferAux, matriz_zoom[0],cZoom);
    }
    if(informacion->id !=informacion->cantidadHebras - 1){// desbloquea la siguiente hebra
        pthread_mutex_unlock(&mutex[informacion->id +1]);
    }
    else{// desbloquea la primera hebra
        pthread_mutex_unlock(&mutex[0]);
    }
    pthread_barrier_wait(barrera);  // barrera para sincronizacion
    float * filaSig;
    pthread_mutex_lock(&mutex[informacion->id]);//bloquea esta hebra
    // se lee del buffer la fila siguiente excepto la ultima hebra.
    if(informacion->id !=informacion->cantidadHebras - 1){
        filaSig=take_from_buffer(informacion->bufferAux, cZoom);
    }
    if(informacion->id !=informacion->cantidadHebras - 1){// desbloquea la siguiente hebra
        pthread_mutex_unlock(&mutex[informacion->id +1]);
    }
    else{// desbloquea la primera hebra
        pthread_mutex_unlock(&mutex[0]);
    }

    //################################################//
    //###################SUAVIZADO####################//
    //################################################//
    pthread_barrier_wait(barrera); //Barrera para sincronizacion

    float **matriz_suave=(float **)malloc(fZoom*sizeof(float*));
    for (int i = 0; i < fZoom; ++i){
        matriz_suave[i]=(float *) malloc (cZoom*sizeof(float));
    }

    if (informacion->id == 0){
        matriz_suave = suavizadoPrimero(fZoom, cZoom, zoomImagen, filaSig, cZoom);
    }
    else if (informacion->id == informacion->cantidadHebras - 1){
        matriz_suave = suavizadoUltimo(fZoom, cZoom, zoomImagen, filaAnterior, cZoom);
    }
    else {
        matriz_suave = suavizadoMedio(fZoom, cZoom, zoomImagen, filaAnterior, cZoom, filaSig, cZoom);
    }
    //################################################//
    //################################################//
    //################################################//

    ////////////////////////////filas anteriores
    pthread_barrier_wait(barrera);  // barrera para sincronizacion

    pthread_mutex_lock(&mutex[informacion->id]);//bloquea esta hebra
    // se envia al buffer de mensajes la ultima fila de la hebra, exepto la ultima hebra.
    if(informacion->id !=informacion->cantidadHebras - 1){
        put_in_buffer(informacion->bufferAux, matriz_suave[fZoom-1],cZoom);
    }
    if(informacion->id !=informacion->cantidadHebras - 1){// desbloquea la siguiente hebra
        pthread_mutex_unlock(&mutex[informacion->id +1]);
    }
    else{// desbloquea la primera hebra
        pthread_mutex_unlock(&mutex[0]);
    }


    pthread_barrier_wait(barrera);  // barrera para sincronizacion
    int * filaAnterior=(float*)malloc(sizeof(float)*cZoom);
    pthread_mutex_lock(&mutex[informacion->id]);//bloquea esta hebra
    // se lee del buffer la fila anterior excepto la primera hebra.
    if(informacion->id !=0){
        filaAnterior=take_from_buffer(informacion->bufferAux, cZoom);
    }
    if(informacion->id !=informacion->cantidadHebras - 1){// desbloquea la siguiente hebra
        pthread_mutex_unlock(&mutex[informacion->id +1]);
    }
    else{// desbloquea la primera hebra
        pthread_mutex_unlock(&mutex[0]);
    }
    ////////////////////////////////////////////////////////////////////////////////////////////////////////

    //////////////////////////////////////Filas siguientes
    pthread_barrier_wait(barrera);  // barrera para sincronizacion

    pthread_mutex_lock(&mutex[informacion->id]);//bloquea esta hebra
    // se envia al buffer de mensajes la ultima fila de la hebra, exepto la ultima hebra.
    if(informacion->id !=0){
        put_in_buffer(informacion->bufferAux, matriz_suave[0],cZoom);
    }
    if(informacion->id !=informacion->cantidadHebras - 1){// desbloquea la siguiente hebra
        pthread_mutex_unlock(&mutex[informacion->id +1]);
    }
    else{// desbloquea la primera hebra
        pthread_mutex_unlock(&mutex[0]);
    }
    pthread_barrier_wait(barrera);  // barrera para sincronizacion
    float * filaSig;
    pthread_mutex_lock(&mutex[informacion->id]);//bloquea esta hebra
    // se lee del buffer la fila siguiente excepto la ultima hebra.
    if(informacion->id !=informacion->cantidadHebras - 1){
        filaSig=take_from_buffer(informacion->bufferAux, cZoom);
    }
    if(informacion->id !=informacion->cantidadHebras - 1){// desbloquea la siguiente hebra
        pthread_mutex_unlock(&mutex[informacion->id +1]);
    }
    else{// desbloquea la primera hebra
        pthread_mutex_unlock(&mutex[0]);
    }

    //################################################//
    //###################DELINEADO####################//
    //################################################//
    pthread_barrier_wait(barrera); //Barrera para sincronizacion

    float **matriz_delineado=(float **)malloc(fZoom*sizeof(float*));
    for (int i = 0; i < fZoom; ++i){
        matriz_delineado[i]=(float *) malloc (cZoom*sizeof(float));
    }

    if (informacion->id == 0){
        matriz_delineado = delineadorPrimero(fZoom, cZoom, zoomImagen, filaSig, cZoom);
    }
    else if (informacion->id == informacion->cantidadHebras - 1){
        matriz_delineado = delineadorUltimo(fZoom, cZoom, zoomImagen, filaAnterior, cZoom);
    }
    else {
        matriz_delineado = delineadorMedio(fZoom, cZoom, zoomImagen, filaAnterior, cZoom, filaSig, cZoom);
    }
    //################################################//
    //################################################//
    //################################################//














    pthread_barrier_wait(barrera);  // barrera para sincronizacion

    pthread_mutex_lock(&mutex[informacion->id]);//bloquea esta hebra
    //print
    printf("\n");
    printf("llegue id: %d\n",informacion->id);
    if(informacion->id !=0){
        for(int j=0; j<columnas; j++){
             printf("%d ",filaAnterior[j]);
        }
        printf("\n");
    }
    for(i=0;i<filasPorHebra;i++){
        for(int j=0; j<columnas; j++){
            printf("%d ",salida[i][j]);
        }
        printf("\n");
    }
    if(informacion->id !=informacion->cantidadHebras - 1){
        for(int j=0; j<columnas; j++){
             printf("%d ",filaSig[j]);
        }
        printf("\n");
    }
    if(informacion->id !=informacion->cantidadHebras - 1){// desbloquea la siguiente hebra
        pthread_mutex_unlock(&mutex[informacion->id +1]);
    }
    else{// desbloquea la primera hebra
        pthread_mutex_unlock(&mutex[0]);
    }

}


int main(int argc, char* argv[]){
    //Revisar los datos de entrada

    int c;
    char* flag;
    char* filas;
    char* columnas;
    char* giro;
    char* factor;
    char* nombre_archivo_in;
    char* nombre_archivo_out;

    int tamanoBuffer;
    int numeroHebras;
    int cantidadFilas;
    int cantidadColumnas;



    //Se inicializa un dato tipo argumento
    //Flag comienza Apagada
    flag = "0";
    
    //Separaciones para la lectura de los argumentos por medio de getopt()
    while((c = getopt(argc, argv, "I:O:M:N:r:g:b"))!= -1){
        switch(c){
            case 'I':
                nombre_archivo_in = optarg;
                break;
            case 'O':
                nombre_archivo_out = optarg;
                break;
            case 'M':                
                if(atoi(optarg) < 1){
                    printf("El largo no es válido");
                    return 0;
                }
                filas = optarg;
                break;
            case 'N':
                if(atoi(optarg) < 1){
                    printf("El ancho no es válido");
                    return 0;
                }
                columnas = optarg;
                break;
            case 'r':
                if(atoi(optarg) < 1){
                    printf("El factor de zoom no es válido");
                    return 0;
                }
                factor = optarg;
                break;
            case 'g':
                if((atoi(optarg) % 90)!= 0){
                    printf("El factor de giro no es válido");
                    return 0;
                }
                giro = optarg;
                break;
            case 'b':
                flag = "1";
                break;
        }
    }
    

    numeroHebras = 4;

    tamanoBuffer = 70;

    pthread_t numeroHebras[numeroHebras];

    //Recordar castear y hacer atoi
    cantidadFilas = 46;
    cantidadColumnas = 68;


    int filasPorHebra;

    //Revisamos la cantidad de filas que van a existir por hebra
    if (cantidadFilas%numeroHebras == 0){
        filasPorHebra = cantidadFilas/numeroHebras;
    }
    else {
        filasPorHebra = cantidadFilas/numeroHebras + 1;
    }

    //Inicializamos el mutex que vamos a utilizar mas adelante
    pthread_mutex_t * mutex = (pthread_mutex_t*)malloc(sizeof(pthread_mutex_t)*numeroHebras);

    //Inicializamos buffers que vamos a utilizar
    buffer_t *buffer = bufferInit(cantidadFilas, cantidadColumnas, tamanoBuffer, nombre_archivo_in);
    ///////////////////////////
    //Posible ciclo para crear n buffers
    buffer_t *bufferAuxiliar = bufferInit(cantidadFilas, cantidadColumnas, numeroHebras-1, nombre_archivo_out);
    //////////////////////////////

    consumer_t * informacion = (consumer_t*)malloc(sizeof(consumer_t)*numeroHebras);
    pthread_mutex_t * mutex=(pthread_mutex_t*)malloc(sizeof(pthread_mutex_t)*numeroHebras);

    //Inicializamos las barreras que seran utilizadas a futuro
    pthread_barrier_t barrera;
    pthread_barrier_init(&barrera,NULL,numeroHebras);

    //Inicializamos las n hebras
    for(int i = 0; i < numeroHebras; i++){
        pthread_mutex_init(&mutex[i],NULL);
        if(i != numeroHebras - 1){
            //Bloqueamos todas las hebras menos la ultima
            pthread_mutex_lock(&mutex[i]);
        }        
        informacion[i].id = i;
        informacion[i].cantidadHebras = numeroHebras;
        
        informacion[i].columnas = columnas;
        informacion[i].factor = factor;
        informacion[i].nombreArchivoSalida = nombre_archivo_out;
        informacion[i].mutex = mutex;
        informacion[i].buffer = buffer;
        informacion[i].bufferAux = bufferAuxiliar;
        informacion[i].barrera = &barrera;
    }

    //Cambiar algoritmo
    int j=0;
    for(int i=0;i<filas;i++){
        informacion[j].filasPorHebra++;
        j++;
        if(j==numeroHebras){
            j=0;
        }
    }
    

    pthread_t pro, * con;
    con=(pthread_t *)malloc(numeroHebras*sizeof(pthread_t));
    pthread_mutex_init(&buffer->mutex, NULL);
    pthread_cond_init(&buffer->notFull, NULL);
    pthread_cond_init(&buffer->notEmpty, NULL);
    pthread_create(&pro, NULL, producer, (void *) buffer);
    for(int i=0;i<numeroHebras;i++){
        pthread_create(&con[i], NULL, consumer, (void *) &informacion[i]);
    }
    pthread_join(pro, NULL);
    for(int i=0;i<numeroHebras;i++){
        pthread_join(con[i], NULL);
    }

    free(buffer);
    free(informacion);
    free(mutex);
    free(con); 


    return 0;
 

}