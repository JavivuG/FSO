//ENTREGABLE 1 de FUNDAMENTOS DE SISTEMAS OPERATIVOS, GRUPO DE LABORATORIO: X2, ULTIMA MODIFICACIÓN: 16:27 18/11/2022
//INTEGRANTES DEL GRUPO
//GARCÍA GONZALEZ, JAVIER
//WORTMAN RUEDA, DIEGO STEFANO
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <stdbool.h>

#define PROMPT "minishell>> "
#define TAMCOMANDO 100

int main(int argc, char *arg[]){
	int i, pid, estado, contador;
	bool sigue=true;
	char comando[TAMCOMANDO], *token;
	char **argv;
	
	while(sigue==true){
		//Declaramos las variables que utilizaremos durante la ejecucion del minishell.
		i=0;
		contador=0;
		
		//Escribimos el prompt por pantalla.
		write(1, PROMPT, strlen(PROMPT));

		//Leemos los comandos escritos por pantalla.
		read(0,&comando[i],1);

		while(comando[i] != '\n'){
			read(0, &comando[++i], 1);
		}

		//Ponemos \0 para terminar de generar la cadena de comandos.
		comando[i]='\0';

		//Obtenemos la longitud de la cadena, para saber posteriormento que ejecutar en eñl proceso hijo.
		//Creamos una copia de este porque una la utilizaremos para separar la cadena, por lo que modificamos su valor.

		//Reserva dinamica de memoria para almacenar los comandos introducidos por el usuario.
		if (((argv=(char **) malloc(3*sizeof(char *))))==NULL){
			fprintf(stderr,"Error al reservar memoria\n");
			return -1;
		}

		for(int i=0; i<3; i++){
			if((argv[i]=(char *) malloc(TAMCOMANDO*sizeof(char)))==NULL){
				fprintf(stderr,"Error al reservar memoria\n");
				return -2;
			}
		}

		//Separamos la cadena segun un espacio introducido por usuario y almacenamos los tres primeros introducidos
		//ya que los 2 primeros son los que se ejecutan, y solo necesitamos conocer el tercero para generar el Warning
		//y no necesitamos conocer a partir del tercer parametro.

		token=strtok(comando, " ");

		while (token!=NULL){
			if(contador<3){
				strcpy(argv[contador], token);
				token=strtok(NULL," ");
				contador++;
			}
			else{
				token=strtok(NULL," ");
			}
		}

		//Comprobamos si el parametro introduzido por el usuario es saluir, ya que si es asi, debemos finalizar ejecución al instante.
		if (strcmp(argv[0],"salir")==0){
			sigue=false;
        }

		if (sigue==true){
			if (contador==3){
				if (strcmp(argv[0], "ls")==0 || strcmp(argv[0], "cat")==0){
                			printf("Warning los comandos con mas de un parametro se ignoraran. A partir del parametro ''%s'' sera ignorados\n", argv[2]);
				}
            }
			//Genero un proceso pesado, en el que el proceso hijo se encargará de realizar las ejecuciones.
			pid = fork();

			//Compruebo si se ha generado bien el proceso pesado se ha generado bien
			if (pid == -1 ){
				fprintf(stderr,"Error al crear el proceso hijo, intentelo de nuevo.\n");
			}
			else if(pid == 0){
				/*PROCESO HIJO*/
				/*Comprueba que parametro debo ejecutar, ls o cat, sino es ninguno, error*/	
				if(strcmp(argv[0],"ls") == 0){		
					if(contador==1){
                        argv[1]=NULL;
						fprintf(stdout,"Ejecutando %s sin parámetros ...\n",argv[0]);
						if((estado=execvp(argv[0], argv ))==-1){
							fprintf(stderr,"El proceso no ha terminado correctamente\n");
                            exit(-1);
						}
					}
					else{
                        argv[2]=NULL;
						fprintf(stdout,"Ejecutando %s con parámetro %s ...\n",argv[0],argv[1]);
						if((estado=execvp(argv[0], argv ))==-1){
							fprintf(stderr, "El proceso no ha terminado correctamente\n");
                            exit(-1);
						}
					}
				}
				if(strcmp(argv[0],"cat") == 0){
					if (contador==1){
						fprintf(stderr,"El comando cat necesita un parámetro: cat parámetro/s\n");
					}
					else{
                        argv[2]=NULL;
						fprintf(stdout,"Ejecutando %s con parametro %s ...\n",argv[0],argv[1]);
						
						
						if((estado=execvp(argv[0], argv ))==-1){
							fprintf(stderr,"El proceso no ha terminado correctamente\n");
                            exit(-1);
						}
					}
				}
				else{
					fprintf(stderr,"Error. El comando ‘‘%s’’ no es un comando valido. Use ls o cat.\n",argv[0]);
				}
				//Volvemos al proceso padre para liberar la memoria dinámica.
				exit(0);
			}
			else{ /*PROCESO PADRE*/
				//Esperamos a que se ejecute el proceso hijo.
				wait(NULL);
				/*LIBERAMOS MEMORIA DINAMICA*/
				for(int i=0;i<3;i++){
					free(argv[i]);
				}
				free(argv);
			}
		}
	}
	return 0;
}
