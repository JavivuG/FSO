//SEGUNDO ENTREGABLE FUNDAMENTOS DE SISTEMAS OPERATIVOS
//GRUPO DE LABORATORIO: X2
//INTEGRANTES DEL GRUPO:
//GARCIA GONZALEZ, JAVIER
//WORTMAN RUEDA, DIEGO STEFANO

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <sys/wait.h>
#include <string.h>
#include <unistd.h>
#include <ctype.h>
#include <semaphore.h>
#include <stdbool.h>

#define NPAR 5

//ESTRUCTURA PARA EL BUFFER

typedef struct leer{
	int fil;
	int col;
	int caracter;
}ESTR;

//ESTRUCTURA PARA LA LISTA ENLAZADA

typedef struct item{
	int fila;
	int columna;
	char c;
}Item;

typedef struct dato{
	Item dt;
	struct dato *sig;
}DATO;

//ESTRUCTURAS PARA EL PASO DE PARAMETROS A LOS HILOS

typedef struct parametrosLector{
	int filas;
	int columnas;
	int tbuffer;
	FILE *fin;
	FILE *fdatos;
}PARALEC;

typedef struct parametrosDecodificador{
	int id;
	int filas;
	int columnas;
	int nhilos;
	int tbuffer;
	FILE *fdatos;
	int col_ini;
	int col_fin;
}PARADECO;

typedef struct parametrosDibujante{
	int filas;
	int columnas;
	char* fdecodificado;
}PARADIBU;

//RECURSOS COMPARTIDOS
ESTR *buffer; // Buffer circular
DATO *cab; // Lista enlazada
DATO *act;
int sig_vaciar=0;//Variable global para los decodificadores

// SEMAFOROS
sem_t hay_espacio;
sem_t hay_dato;
sem_t despierta_dibu;
sem_t mutex_pos_vaciar;
sem_t mutex_lista;
sem_t escritor;

//PROTOTIPOS DE LAS FUNCIONES
void *funcion_lector(void *arg);
void *funcion_decodificador(void *arg);
void *funcion_dibujante(void *arg);
DATO *crearDato(Item x);
void insertar(Item x);
void free_list(DATO* cabeza);

int main(int argc, char *argv[]){
	//Declaracion de variables
	FILE *f1, *fdatos;
	pthread_t lector, *decodificador, dibujante;
	int l, filas, columnas, reparto, sobrantes;
	char token[50];
	//Inicializacion de semáforos
	sem_init(&hay_dato,1,0);
	sem_init(&despierta_dibu,1,0);
	sem_init(&mutex_pos_vaciar,0,1);
	sem_init(&mutex_lista,0,1);
	sem_init(&escritor,0,1);

	//lista enlazada vacia(genero el estado inicial de la lista enlazada).
	cab = act = NULL;

	//Comprobamos los parámetros introducidos por el usuario
	if (argc != NPAR+1){
		fprintf(stderr,"Número de parámetros incorrecto.\n");
		return -1;
	}
	if ((f1=fopen(argv[1],"r"))==NULL || (fdatos=fopen(argv[5],"w"))==NULL){
                fprintf(stderr,"No se pudo abrir el fichero de lectura.\n");
                return -2;
	}else{
		//Obtenemos el numero de filas y columnas de la imagen.
		fgets(token,50,f1);
		strtok(token,",");

		filas = atoi(strtok(NULL,","));
		columnas = atoi(strtok(NULL,","));
	}
	if (atoi(argv[3])<=0 || atoi(argv[4])<=0){
		fprintf(stderr,"El tercer y cuarto parámetro deben ser numéricos positivos.\n");
		return -3;
	}else{
		//Inicializacion del semaforo de espacio del buffer
		sem_init(&hay_espacio,1,atoi(argv[3]));
		//Reserva de memoria dinamica para los hilos decodificadores
		if ((decodificador = (pthread_t *) malloc(sizeof(pthread_t)*atoi(argv[4]))) == NULL){
			fprintf(stderr,"Error al reservar memoria.");
			return -1;
		}
	}
	
	//Reserva de memoria para el buffer
	if ((buffer = (ESTR *) malloc(atoi(argv[3])*sizeof(ESTR)))==NULL){
	        fprintf(stderr,"Error al reservar memoria.\n");
		return -3;
	}
	//Creamos los hilos
	//HILO LECTOR
	PARALEC *plector;
	//Reserva de memoria para los parametros del hilo lector.
	if ((plector =(PARALEC *) malloc(sizeof(PARALEC)))==NULL){
		fprintf(stderr,"No se pudo reservar memoria.");
		return -3;
	}

	plector -> filas = filas;
	plector -> columnas = columnas;
	plector -> fin = f1;
	plector -> fdatos = fdatos;
	plector -> tbuffer = atoi(argv[3]);
	pthread_create(&lector,NULL,funcion_lector,(void *) plector);
	
	//Buscamos el numero de columnas asignadas para cada hilo decodificador.
	reparto = columnas/atoi(argv[4]);
	sobrantes = columnas - (reparto*atoi(argv[4]));


	//HILOS DECODIFICADORES
	PARADECO **pdeco;
	//Reserva de memoria para los parametros de los decodificadores.
	if ((pdeco =(PARADECO **) malloc(sizeof(PARADECO *)*atoi(argv[4])))==NULL){
		fprintf(stderr,"No se pudo reservar memoria.");
		return -3;
	}

	for (int i=0; i<atoi(argv[4]); i++){
		if ((pdeco[i] = (PARADECO *) malloc(sizeof(PARADECO)))==NULL){
			fprintf(stderr,"No se pudo reservar memoria.");
			return -3;
		}
		pdeco[i] -> nhilos = atoi(argv[4]);
		pdeco[i] -> fdatos = fdatos;
		pdeco[i] -> tbuffer = atoi(argv[3]);
		pdeco[i] -> filas = filas;
		pdeco[i] -> columnas = columnas;
	}
	
	for(int h=0;h<atoi(argv[4]); h++){
		pdeco[h]->id=h;
		//Si la division anterior no es entera
		if (sobrantes > 0){
			if (h == 0){
				pdeco[h] -> col_ini = 0 ;
				pdeco[h] -> col_fin = (pdeco[h] -> col_ini) + reparto+1;
			}else{
				pdeco[h] -> col_ini = (pdeco[h-1] -> col_fin) ;
				pdeco[h] -> col_fin = (pdeco[h] -> col_ini) + reparto+1;
			}
			sobrantes--;
		}else{
			if (h == 0){
			        pdeco[h] -> col_ini = 0 ;
				pdeco[h] -> col_fin = (pdeco[h] -> col_ini) + reparto;
			}else{
				pdeco[h] -> col_ini = (pdeco[h-1] -> col_fin);
				pdeco[h] -> col_fin = (pdeco[h] -> col_ini) + reparto;
			}
		}
		pthread_create(&decodificador[h],NULL,funcion_decodificador,(void *) pdeco[h]); 
	}
	
	//HILO DIBUJANTE
	PARADIBU *pdibu;
	//Reserva de memoria para los parametros del hilo dibujante.
	if ((pdibu =(PARADIBU *) malloc(sizeof(PARADIBU)))==NULL){
		fprintf(stderr,"No se pudo reservar memoria.");
		return -3;
	}

	pdibu -> fdecodificado = argv[2]; 
	pdibu -> filas = filas;
	pdibu -> columnas = columnas;
	pthread_create(&dibujante,NULL,funcion_dibujante,(void *) pdibu);

	//Esperamos a que terminen los hilos lanzados.
	pthread_join(lector,NULL);
	for(int h=0 ;h<atoi(argv[4]); h++){
		pthread_join(decodificador[h],NULL);
	}
	fclose(fdatos);
	pthread_join(dibujante,NULL);
	
	//Liberamos ya el buffer porque no nos hará falta, junto con el resto de recursos..
	free(buffer);
	free(decodificador);
	free_list(cab);

	//Destruimos los semaforors utilizados tras su uso.
	sem_destroy(&hay_espacio);
	sem_destroy(&hay_dato);
	sem_destroy(&despierta_dibu);
	sem_destroy(&mutex_pos_vaciar);
	sem_destroy(&mutex_lista);
	sem_destroy(&escritor);
}

//Funcion para crear un dato nuevo.
DATO *crearDato(Item x){
	DATO *z;
	if ((z = (DATO *) malloc(sizeof(DATO))) == NULL){
		fprintf(stderr, "Error al reservar memoria");
		exit(-1);
	}
	z -> dt = x;
	z -> sig = NULL;
	return z;
}

//Funcion para aniadir un nuevo dato a la lista.
void insertar(Item x){
        DATO *nuevo;
        nuevo = crearDato(x);
	if ( cab == NULL){
		cab = nuevo;
		act = nuevo;
	}else{
		act -> sig = nuevo;
		act = nuevo;
	}
}

//Metodo para liberar cada nodo de la lista enlazada.
void free_list(DATO* cabeza) {
	DATO* actual = cabeza;
	while (actual != NULL) {
		DATO* siguiente = actual -> sig;
		free(actual);
		actual = siguiente;
	}
}

void *funcion_lector(void *arg){

	char token[50];
	int p1, p2, p3, i=0;
	int lprocesadas=0, lvalidas=0, lerroneas=0;
	PARALEC *parametro = (PARALEC *) arg;
	//Leemos las lineas del fichero codificado
	while((fgets(token,50,parametro->fin)!=NULL)){

		p1 = atoi(strtok(token,","));
		p2 = atoi(strtok(NULL,","));
		p3 = atoi(strtok(NULL,","));
		//Comprobamos si es valido con respecto a las filas o a las columnas.
		if(p1 < (parametro->filas) && p2 < (parametro->columnas)){
			sem_wait(&hay_espacio);
			buffer[i].fil = p1;
			buffer[i].col = p2;
			buffer[i].caracter = p3;
			i=(i+1)%(parametro->tbuffer);
			lvalidas++;
			sem_post(&hay_dato);
			
		}else{
			lerroneas++;
		}
		lprocesadas++;
	}
	//Ponemos un -1 en el ultimo dato para saber que ya no hay mas lineas en el fichero
	sem_wait(&hay_espacio);
	buffer[i].fil = -1;
	sem_post(&hay_dato);
	//Escribimos los datos invalidos detectados por el hilo lector, semaforo escritor por si esta escribiendo un hilo lector al mismo tiempo.
	sem_wait(&escritor);
	fprintf(parametro->fdatos,"Hilo Lector\nLíneas procesadas: %i\nLineas correctas: %i\nLineas no validas: %i\n",lprocesadas,lvalidas,lerroneas);
	sem_post(&escritor);
	fclose(parametro->fin);
	pthread_exit(NULL);

} 
void *funcion_decodificador(void *arg){
	int total, valido, invalido, i, fil, col, caracter;
	PARADECO *parametro=(PARADECO *) arg;
	total=0;
        invalido=0;
	valido=0;
	bool seguir = true;
	//recorremos el buffer circular
	while(seguir){
		//esperamos a que hayan datos
		sem_wait(&hay_dato);
		sem_wait(&mutex_pos_vaciar);
		i=sig_vaciar;
		fil = buffer[i].fil;
		//Indicativo de fin de fichero de entrada.
		if (fil == -1){
			sem_post(&hay_dato);
			sem_post(&mutex_pos_vaciar);
			seguir = false;
		}
		if (seguir){
			col = buffer[i].col;
			//Comprobamos si pertenece al rango de columnas del hilo. 
			if (col >= (parametro -> col_ini) && col < (parametro -> col_fin)){
				sig_vaciar=(sig_vaciar+1)%(parametro->tbuffer);
				caracter = buffer[i].caracter;
				sem_post(&hay_espacio);
				total++;
				//Comprobamos si es valido el caracter.
				if (caracter >= 32 && caracter <= 126 ){
					valido++;
					Item x;
					x.fila = fil;
					x.columna = col;
					x.c = (char) caracter;
					sem_wait(&mutex_lista);
					insertar(x);
					sem_post(&despierta_dibu);
					sem_post(&mutex_lista);				
				}
				else{
					invalido++;
				}
				
			}else{
				sem_post(&hay_dato);
			}
			sem_post(&mutex_pos_vaciar);
		}
	}

	//Escribir datos de los invalidos en fdatos, semaforo escritor por si esta escribiendo el hilo lector en el mismo instante.
	sem_wait(&escritor);
	fprintf(parametro->fdatos,"ID Hilo Decodificador: %i\nColumnas asignadas: %i-%i\nCaracteres totales:%i\nCaracteres validos: %i\nCaracteres invalidos: %i\n",parametro->id,(parametro -> col_ini),(parametro -> col_fin),total,valido,invalido);
	sem_post(&escritor);
	//Damos señal para que acceda otro hilo
	pthread_exit(NULL);
}

void *funcion_dibujante(void *arg){
	
	int i=0, j=0;
	char **dibujo;
	DATO *q;
	FILE *fdes;
	PARADIBU *parametro = (PARADIBU *) arg;
	int filas=parametro->filas;
	int columnas=parametro->columnas;
	
	//Comprobamos si se abre el fichero de salida
	if ((fdes = fopen(parametro->fdecodificado,"w"))==NULL){
		fprintf(stderr,"Error al abrir el fichero.\n");
		exit(-1);
	}
	
	//Reserva de memoria dinamica para almacenar los caracteres 
	if ((dibujo=(char **) malloc(filas*sizeof(char *)))==NULL){
		fprintf(stderr,"Error al reservar memoria.\n");
		exit(-1);
	}
	
	for(int i=0;i<filas;i++){
		if((dibujo[i]=(char *) malloc(columnas*sizeof(char)))==NULL){
			fprintf(stderr,"Error al reservar memoria.\n");
			exit(-1);
		}
	}
	
	
	//Recorrido de la lista enlazada
	for (int i = 0; i< (filas*columnas); i++){
		sem_wait(&despierta_dibu);
		if (i==0){
			q = cab;
		}else{
			q = q -> sig;
		}
		dibujo[q -> dt.fila][q -> dt.columna]= q -> dt.c;
	}

	//Escritura de los caracteres por pantalla y en el fichero de salida
	for(int i=0;i<filas;i++){
		for(int j=0;j<columnas;j++){
			if(j==columnas-1){
				printf("%c\n",dibujo[i][j]);
				fprintf(fdes,"%c\n",dibujo[i][j]);
			}else{
				printf("%c",dibujo[i][j]);
				fprintf(fdes,"%c",dibujo[i][j]);
			}
		}
	}
	//cierre del fichero de salida
	fclose(fdes);
	//Liberacion de la memoria reservada
	for(int j=0;j<filas;j++){
		free(dibujo[j]);
	}	
	free(dibujo);
	
	pthread_exit(NULL);
	
}

