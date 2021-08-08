#include "../include/discordiador.h"


t_config* configuracion;

int main (void) {

flag_first_init = 1;
	contadorPatota = 'A';

	sabotaje = malloc(sizeof(t_sabotaje));

	configuracion=leer_config("./cfg/discordiador.config");
	config_discordiador = iniciar_configuracion(configuracion, &logger);

	if(!strcmp(config_discordiador->algoritmo, "FIFO")){
		config_discordiador->quantum = -1;
	}

	inicializar_parametros();

	iniciar_servidor_desacoplado();
	sleep(1);

	leer_consola();

}

void iniciar_servidor_desacoplado(){

	pthread_t thread;

	pthread_create(&thread,NULL,(void*)iniciar_servidor_discordiador, NULL);
	pthread_detach(thread);
}

void iniciar_servidor_discordiador(){

	iniciar_servidor(config_discordiador->ip_discordiador, config_discordiador->puerto_discordiador, handler_client);

}

void handler_client(t_result* result){
	if(!strcmp(result->mensajes->mensajes[0], "POSICION_SABOTAJE")){
		sem_wait(sem_sabotaje);
		pausar_planificacion();
		char* respuesta = result->mensajes->mensajes[1];
		char** split = string_split(respuesta, "|");
		char* var1 = split[0];
		char* var2 = split[1];
		sabotaje->posicion.posx = atoi(var1);
		sabotaje->posicion.posy = atoi(var2);
		sabotaje->duracion = config_discordiador->duracion_sabotaje;
		planificar_sabotaje();
		iniciar_planificacion();
		sleep(2);
		sem_post(sem_sabotaje);
	}
}

void leer_consola() {

	while (1){
		pthread_t thread_mensaje;

		char* leido = readline(">");


		while(strncmp(leido, "", 1) != 0) {
			char* mensaje = string_new();
			string_append(&mensaje, leido);
			pthread_create(&thread_mensaje,NULL,(void*)interpretar_mensaje, mensaje);
			pthread_detach(thread_mensaje);
			free(leido);
			leido = readline(">");
			free(mensaje);
		}
	}
}

void interpretar_mensaje(char* mensaje){

	int size_msj = 0;

    char** string_prueba = NULL;
    string_prueba = string_split(mensaje, " ");

    for(int i = 0; string_prueba[i] != NULL; i++){
    	size_msj += 1;
    }

    //string_prueba = separar_por_comillas(string_prueba);

    if (string_prueba == NULL){
        return;
    }

    int numero_mensaje = obtener_numero_mensaje(string_prueba[0]);

    switch(numero_mensaje){

        case 1 : iniciar_patota(string_prueba, size_msj);
        break;
        case 2 : listar_tripulantes();
        free_string_split(string_prueba);
        break;
        case 3 : expulsar_tripulante(string_prueba[1]);
        free_string_split(string_prueba);
        break;
        case 4 : iniciar_planificacion();
        free_string_split(string_prueba);
        break;
        case 5 : pausar_planificacion();
        free_string_split(string_prueba);
		break;
        case 6 : obtener_bitacora(string_prueba[1]);
        free_string_split(string_prueba);
		break;
        default : printf("No es un mensaje valido \n");
        free_string_split(string_prueba);

    }

}

void iniciar_planificacion(){

	if(flag_first_init == 1){
		flag_first_init = 0;
		sem_post(sem_lock_new);
		pthread_t thread;
		pthread_create(&thread,NULL,(void*)iniciar_planificador, NULL);
		pthread_detach(thread);
	} else {
		sem_post(sem_planificar);
		sem_post(sem_planificador);
	}

}

void pausar_planificacion(){

	sem_wait(sem_planificar);
	sem_wait(sem_planificador);

}

void obtener_bitacora(char* patotaTid){

	char* numAux = string_substring_from(patotaTid, 1);

	int idTripulante = atoi(numAux);
	char* aux = string_substring_until(patotaTid, 1);

	char* respuesta = enviar_mensaje_obtener_bitacora(modulo_imongo, aux, idTripulante);

	printf("BITACORA: %s\n \t%s\n", patotaTid, respuesta);

	free(numAux);
	free(aux);

}

void expulsar_tripulante(char* patotaTid){

	//char* respuesta = enviar_mensaje_expulsar_tripulante(modulo_miramhq, patotaTid);

	IteratorList it = NULL;

	for(it = beginlist(tripulantes_totales); it != NULL; it = nextlist(it)){

		t_tripulante* tripulante = (t_tripulante*) dataiterlist(it);

		char* param1 = string_itoa(tripulante->tid);

		char* result = unir_pid_tid(tripulante->pid, param1);

		if(!strcmp(result, patotaTid)){
			//popiterlist(&tripulantes_totales, it);
			pasar_a_expulsar_tripulante(tripulante);
			free(param1);
			free(result);
			return;
		}

		free(param1);
		free(result);

	}

	printf("El tripulante no esta en la nave\n");
	return;

}

void listar_tripulantes(){

	IteratorList it = NULL;

	printf("\n");

	for(it = beginlist(tripulantes_totales); it != NULL; it = nextlist(it)){

		t_tripulante* tripulante = (t_tripulante*) dataiterlist(it);

		char* estatus = obtener_status_string(tripulante->estado);

		printf("Tripulante: %d\t Patota: %s\t Status: %s\n", tripulante->tid, tripulante->pid, estatus);

		free(estatus);

		//TODO: HACER LOG

	}

}

void iniciar_patota(char** string_prueba, int size_msj){

	int contadorTripulante = 0;
	int coordenadasRecibidas = size_msj - 3;
	int coordenadas00 = atoi(string_prueba[1]) - coordenadasRecibidas;

	t_patota* patota = malloc(sizeof(t_patota));

	List* listaAux = malloc(sizeof(List));
	initlist(listaAux);

	List* listaAux2 = malloc(sizeof(List));
	initlist(listaAux2);

	patota->tripulantes = listaAux;
	patota->posiciones = listaAux2;


	if(coordenadasRecibidas > atoi(string_prueba[1])){
		printf("Mensaje con demasiados parametros\n");
		return;
	}

	char* tareas = obtener_tarea_archivo(string_prueba[2]);

	if(tareas == NULL){
		printf("Path de tareas invalido\n");
		return;
	}

	patota->tarea = tareas;

	for(int i = 1; i <= coordenadasRecibidas; i++){
		char** posiciones_spliteadas = string_split(string_prueba[2+i], "|");
		int pos_x = (int)atoi(posiciones_spliteadas[0]);

		if(posiciones_spliteadas[1] == NULL){
			printf("Coordenadas mal escritas\n");
			return;
		}
		int pos_y = (int)atoi(posiciones_spliteadas[1]);

		pushbacklist(patota->posiciones, string_prueba[2+i]);

		t_tripulante* tripulante = crear_tripulante(contadorTripulante, pos_x, pos_y);

		pushbacklist(patota->tripulantes, tripulante);

		free(posiciones_spliteadas);

		contadorTripulante += 1;

	}
	for(int j = 0;j < coordenadas00;j++){

		t_tripulante* tripulante = crear_tripulante(contadorTripulante, 0, 0);

		pushbacklist(patota->tripulantes, tripulante);

		contadorTripulante += 1;
	}

	pushbacklist(&pcb_new, patota);
	sem_post(sem_pcb_new);

}

t_tripulante* crear_tripulante(int tid, int posx, int posy){

	t_tripulante* tripulante = malloc(sizeof(t_tripulante));

	t_posicion posicion;
	posicion.posx = posx;
	posicion.posy = posy;

	sem_t* sem_ciclo_cpu = malloc(sizeof(sem_t));
	sem_init(sem_ciclo_cpu, 0, 0);

	sem_t* block_io = malloc(sizeof(sem_t));
	sem_init(block_io, 0, 0);

	sem_t* new_a_ready = malloc(sizeof(sem_t));
	sem_init(new_a_ready, 0, 0);

	sem_t* ready_a_exec = malloc(sizeof(sem_t));
	sem_init(ready_a_exec, 0, 0);

	tripulante->tid = tid;
	tripulante->estado = NEW;
	tripulante->posicion = posicion;
	tripulante->ciclo_cpu = sem_ciclo_cpu;
	tripulante->block_io = block_io;
	tripulante->new_a_ready = new_a_ready;
	tripulante->ready_a_exec = ready_a_exec;
	tripulante->primeraVez = 1;
	tripulante->tarea = NULL;
	tripulante->exit = 0;
	tripulante->quantum = 0;

	printf("NUEVO TRIPULANTE: PosX: %d - PosY: %d - TID: %d\n", tripulante->posicion.posx, tripulante->posicion.posy, tripulante->tid);

	return tripulante;

}

int filesize2(char *archivo)
{
   FILE *stream;
   int curpos, length;

   if((stream = fopen(archivo, "r+"))!=NULL)
   {
      curpos = ftell(stream);
      fseek(stream, 0L, SEEK_END);
      length = ftell(stream);
      fseek(stream, curpos, SEEK_SET);

      fclose(stream);

      return length;
   }

   return -1;
}

char* obtener_tarea_archivo(char* path){

	char* restoString = string_new();


	string_append(&restoString, "/home/utnso/");
	string_append(&restoString, path);

	FILE* tareas = fopen(restoString, "r+");

	if(tareas == NULL){
		printf("No se pudo leer el archivo\n");
		return NULL;
	}

	char* cadenaTareas = malloc(filesize2(restoString)+1);
	char* cadenaReturn = string_new();
	char* cadenaReturnPosta = string_new();

	do{
		fgets(cadenaTareas, filesize2(restoString), tareas);

		string_append(&cadenaReturn, cadenaTareas);

	}while(!feof(tareas));

	char** posiciones_spliteadas = string_split(cadenaReturn, "\n");

	if(posiciones_spliteadas[1] != NULL){
		for(int i = 0; posiciones_spliteadas[i] != NULL; i++){
			string_append(&cadenaReturnPosta, posiciones_spliteadas[i]);
			if(posiciones_spliteadas[i+1] != NULL){
				string_append(&cadenaReturnPosta, "|");
			}
		}
	} else {
		free_string_split(posiciones_spliteadas);
		free(cadenaReturnPosta);
		free(cadenaTareas);
		free(restoString);

		fclose(tareas);

		return cadenaReturn;
	}

	free_string_split(posiciones_spliteadas);
	free(cadenaReturn);
	free(cadenaTareas);
	free(restoString);

	fclose(tareas);

	return cadenaReturnPosta;

}

int obtener_numero_mensaje(char* mensaje){

	if(!strcmp(mensaje, "INICIAR_PATOTA")){
		return 1;
	} else if (!strcmp(mensaje, "LISTAR_TRIPULANTES")){
		return 2;
	} else if (!strcmp(mensaje, "EXPULSAR_TRIPULANTE")){
		return 3;
	} else if (!strcmp(mensaje, "INICIAR_PLANIFICACION")){
		return 4;
	} else if (!strcmp(mensaje, "PAUSAR_PLANIFICACION")){
		return 5;
	} else if (!strcmp(mensaje, "OBTENER_BITACORA")){
		return 6;
	}
	return -1;

}

