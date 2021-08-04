#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

typedef struct {
    int filas;
    int columnas;

    int **buffer;
    int in, out;
    int full, empty;
    pthread_mutex_t mutex;
    pthread_cond_t notFull, notEmpty;
} buffer_t;

typedef struct {
    int id;
    int numHebras;
    int filasPorHebra;
    int columnas;
    pthread_mutex_t * mutex;
    buffer_t * buffer;
    buffer_t * pasoMensajes;
    pthread_barrier_t * barrera;
} consumer_t;


int produce(int i){
    return i*2;
}

buffer_t * bufferInit(int filas,int columnas){
    buffer_t * buf=(buffer_t*)malloc(sizeof(buffer_t));
    buf->in=0;
    buf->out=0;
    buf->full=0;
    buf->empty=1;
    buf->filas=filas;
    buf->columnas=columnas;
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
    int numeroElementos=filas*columnas;
    int imagen[numeroElementos];
    int p=0;
    for(int i=0;i<numeroElementos;i++){  // aca va un buffer de lectura por ejemplo, un arreglo grande que representa una matriz pero con sus datos en linea en un arreglo
        imagen[i]=p;
        p++;
        
    }
    int * v;
    v=(int *)malloc(sizeof(int)*columnas);
    int i=0;
    p=0;
    while (i<filas) {

        for(int j=0;j<columnas;j++){ //produce()
            v[j]=imagen[p];
            p++;
        }

        pthread_mutex_lock (&buffer->mutex);
        while (buffer->full) {
            pthread_cond_wait (&buffer->notFull, &buffer->mutex);
        }
        put_in_buffer(buffer, v,columnas);
        pthread_cond_signal(&buffer->notEmpty);
        pthread_mutex_unlock(&buffer->mutex);
        i++;
    }
}


void *consumer (void *arg){

    consumer_t * datos;
    datos=(consumer_t *) arg;

    buffer_t *buffer;
    buffer = datos->buffer;

    pthread_barrier_t * barrera;
    barrera=datos->barrera;

    pthread_mutex_t * mutex=datos->mutex;

    int filasPorHebra=datos->filasPorHebra;
    int columnas=datos->columnas;

    int ** salida=(int**)malloc(sizeof(int*)*filasPorHebra);
    pthread_mutex_lock(&mutex[datos->id]); //bloquea esta hebra
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

    if(datos->id !=datos->numHebras-1){ // desbloquea la siguiente hebra
        pthread_mutex_unlock(&mutex[datos->id +1]);
    }
    else{
        pthread_mutex_unlock(&mutex[0]); //o la primera si esta era la ultima hebra
    }

    pthread_barrier_wait(barrera);  // barrera para sincronizacion

    pthread_mutex_lock(&mutex[datos->id]);//bloquea esta hebra
    //print
    printf("llegue id: %d\n",datos->id);
    for(i=0;i<filasPorHebra;i++){
        for(int j=0; j<columnas; j++){
            printf("%d ",salida[i][j]);
        }
        printf("\n");
    }
    
    if(datos->id !=datos->numHebras-1){// desbloquea la siguiente hebra
        pthread_mutex_unlock(&mutex[datos->id +1]);
    }
    else{// desbloquea la primera hebra
        pthread_mutex_unlock(&mutex[0]);
    }


    ////////////////////////////filas anteriores
    pthread_barrier_wait(barrera);  // barrera para sincronizacion

    pthread_mutex_lock(&mutex[datos->id]);//bloquea esta hebra
    // se envia al buffer de mensajes la ultima fila de la hebra, exepto la ultima hebra.
    if(datos->id !=datos->numHebras-1){
        put_in_buffer(datos->pasoMensajes, salida[filasPorHebra-1],columnas);
    }
    if(datos->id !=datos->numHebras-1){// desbloquea la siguiente hebra
        pthread_mutex_unlock(&mutex[datos->id +1]);
    }
    else{// desbloquea la primera hebra
        pthread_mutex_unlock(&mutex[0]);
    }


    pthread_barrier_wait(barrera);  // barrera para sincronizacion
    int * filaAnterior;
    pthread_mutex_lock(&mutex[datos->id]);//bloquea esta hebra
    // se lee del buffer la fila anterior excepto la primera hebra.
    if(datos->id !=0){
        filaAnterior=take_from_buffer(datos->pasoMensajes, columnas);
    }
    if(datos->id !=datos->numHebras-1){// desbloquea la siguiente hebra
        pthread_mutex_unlock(&mutex[datos->id +1]);
    }
    else{// desbloquea la primera hebra
        pthread_mutex_unlock(&mutex[0]);
    }
    ////////////////////////////////////////////////////////////////////////////////////////////////////////

    //////////////////////////////////////Filas siguientes
    pthread_barrier_wait(barrera);  // barrera para sincronizacion

    pthread_mutex_lock(&mutex[datos->id]);//bloquea esta hebra
    // se envia al buffer de mensajes la ultima fila de la hebra, exepto la ultima hebra.
    if(datos->id !=0){
        put_in_buffer(datos->pasoMensajes, salida[0],columnas);
    }
    if(datos->id !=datos->numHebras-1){// desbloquea la siguiente hebra
        pthread_mutex_unlock(&mutex[datos->id +1]);
    }
    else{// desbloquea la primera hebra
        pthread_mutex_unlock(&mutex[0]);
    }
    pthread_barrier_wait(barrera);  // barrera para sincronizacion
    int * filaSig;
    pthread_mutex_lock(&mutex[datos->id]);//bloquea esta hebra
    // se lee del buffer la fila siguiente excepto la ultima hebra.
    if(datos->id !=datos->numHebras-1){
        filaSig=take_from_buffer(datos->pasoMensajes, columnas);
    }
    if(datos->id !=datos->numHebras-1){// desbloquea la siguiente hebra
        pthread_mutex_unlock(&mutex[datos->id +1]);
    }
    else{// desbloquea la primera hebra
        pthread_mutex_unlock(&mutex[0]);
    }
    //////////////////////////////////////////////////////////////////////////////////////////////




    pthread_barrier_wait(barrera);  // barrera para sincronizacion

    pthread_mutex_lock(&mutex[datos->id]);//bloquea esta hebra
    //print
    printf("\n");
    printf("llegue id: %d\n",datos->id);
    if(datos->id !=0){
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
    if(datos->id !=datos->numHebras-1){
        for(int j=0; j<columnas; j++){
             printf("%d ",filaSig[j]);
        }
        printf("\n");
    }
    if(datos->id !=datos->numHebras-1){// desbloquea la siguiente hebra
        pthread_mutex_unlock(&mutex[datos->id +1]);
    }
    else{// desbloquea la primera hebra
        pthread_mutex_unlock(&mutex[0]);
    }

}


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