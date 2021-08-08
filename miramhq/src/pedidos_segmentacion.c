#include "../include/pedidos_segmentacion.h"

void dibujar(){
	nivel_gui_dibujar(nivel);
}

void handle_iniciar_patota(t_result* result){

	sem_wait(sem_mutex_inicio_patota);

	int i=0;

	char** mensajites = malloc(sizeof(char*)*2);

	int cantidad_tripulantes = atoi(result->mensajes->mensajes[1]);

	int tamanioTareas = strlen(result->mensajes->mensajes[2])+1;

	int tamanioPatota = tamanioTareas + sizeof(t_pcb) + (cantidad_tripulantes*sizeof(t_tcb));

	if(tamanioPatota > espacioLibreMemoria()){
		ejecutarCompactacion();
		sem_wait(sem_mutex_continuar_inicio_patota);
		mensajites[0] = "NO";
		send_messages_socket(result->socket, mensajites, 1);
		free(mensajites);
		sem_post(sem_mutex_inicio_patota);
		return;

	}

 	void* aux = (void*)result->mensajes->mensajes[2];

	void* tareas = malloc(sizeof(char)*tamanioTareas);

	memcpy(tareas, aux, tamanioTareas);

	l_proceso_segmento* proceso = crearProceso(contadorProcesosGlobal);

	l_procesoAux* proceso2 = malloc(sizeof(l_procesoAux));
	proceso2->cantTripulantes = cantidad_tripulantes;
	proceso2->idProceso = contadorProcesosGlobal;
	pushbacklist(&tablaProcesosAux, proceso2);

	pushbacklist(&tablaProcesos, proceso);

	t_pcb* pcb = (t_pcb*)crearPcb((uint32_t)contadorProcesosGlobal, 0);
	//MUTEX
	sem_wait(sem_mutex_crear_segmento);
	contadorProcesosGlobal++;
	sem_post(sem_mutex_crear_segmento);

	char* paraTirarFree2 = string_itoa(i);
	l_segmento* segmentoPcb = crearSegmento(proceso, tipo_pcb, paraTirarFree2, sizeof(t_pcb), pcb);
	i++;

	free(paraTirarFree2);
	free(pcb);


	int tripulantesEn00 = cantidad_tripulantes + 3 - (int)*result->mensajes->size;
	int tripulantesConPosicion = cantidad_tripulantes - tripulantesEn00;


	for(int w=0;w<tripulantesConPosicion;w++){

		char** posiciones_spliteadas = string_split(result->mensajes->mensajes[w+3], "|");
		uint32_t pos_x = (uint32_t)atoi(posiciones_spliteadas[0]);
		uint32_t pos_y = (uint32_t)atoi(posiciones_spliteadas[1]);

		void* tcb = crearTcb((uint32_t)contadorTripulantesGlobal, 'R', pos_x, pos_y, 0, (uint32_t)segmentoPcb->byteInicio);
		contadorTripulantesGlobal++;

		sem_wait(sem_mutex_mapa);
		personaje_crear(nivel, traductorTripAChar(spawnerDeTripulantes), pos_x, pos_y);
		spawnerDeTripulantes++;
		dibujar();
		sem_post(sem_mutex_mapa);

		char* paraTirarFreeAcaAdentro = string_itoa(i);
		crearSegmento(proceso, tipo_tcb, paraTirarFreeAcaAdentro, sizeof(t_tcb), tcb);

		free(paraTirarFreeAcaAdentro);
		free(tcb);

		i++;
		free_string_split(posiciones_spliteadas);
	}

	for(int j=*(result->mensajes->size);j<cantidad_tripulantes+3;j++){

		void* tcb = crearTcb((uint32_t)contadorTripulantesGlobal, 'R', 0, 0, 0, (uint32_t)segmentoPcb->byteInicio);
		contadorTripulantesGlobal++;

		sem_wait(sem_mutex_mapa);
		personaje_crear(nivel, traductorTripAChar(spawnerDeTripulantes), 0, 0);
		spawnerDeTripulantes++;
		dibujar();
		sem_post(sem_mutex_mapa);

		char* paraTirarFreeAcaAdentro = string_itoa(i);
		crearSegmento(proceso, tipo_tcb, paraTirarFreeAcaAdentro, sizeof(t_tcb), tcb);

		free(paraTirarFreeAcaAdentro);
		free(tcb);
		i++;
	}

	char* paraTirarFree = string_itoa(i);
	l_segmento* segmentoTareas = crearSegmento(proceso, tipo_tareas, paraTirarFree, tamanioTareas, tareas);
	i++;

	free(paraTirarFree);
	free(tareas);

	t_pcb* pcbAux = segmentoPcb->direccion_inicio;
	pcbAux->direccionInicioTareas = segmentoTareas->byteInicio;


	//AGREGUE ESTO PARA MANDAR LA TAREA EN UN MENSAJE
	char** tareas_spliteadas = string_split(result->mensajes->mensajes[2], "|");
	char* primerTarea = tareas_spliteadas[0];

	mensajites[0] = primerTarea;
	send_messages_socket(result->socket, mensajites, 1);
	resolverTarea(primerTarea);

	free_string_split(tareas_spliteadas);
	free(mensajites);

	sem_post(sem_mutex_inicio_patota);

}

int espacioLibreMemoria(){

	int valorReturn = 0;
	IteratorList it = NULL;

	for(it = beginlist(l_particiones); it != NULL; it = nextlist(it)){

		l_segmento* segmento = dataiterlist(it);

		if(!segmento->ocupado){
			valorReturn += segmento->tamanio_segmento;
		}

	}

	return valorReturn;

}

void* crearPcb(uint32_t idPatota, uint32_t direccionInicioTareas){
	t_pcb* pcb= malloc(sizeof(t_pcb));

	pcb->idPatota = idPatota;
	pcb->direccionInicioTareas = direccionInicioTareas;

	return pcb;
}

void* crearTcb(uint32_t idTripulante, char estadoTripulante, uint32_t pos_x, uint32_t pos_y, uint32_t proxima_instruccion_a_ejecutar, uint32_t puntero_PCB){

	t_tcb* tcb = malloc(sizeof(t_tcb));

	tcb->idTripulante = idTripulante;
	tcb->estadoTripulante = estadoTripulante;
	tcb->pos_x = pos_x;
	tcb->pos_y = pos_y;
	tcb->proxima_instruccion_a_ejecutar = proxima_instruccion_a_ejecutar;
	tcb->puntero_PCB = puntero_PCB;


	return tcb;
}

void handle_recibir_ubicacion_tripulante(t_result* result){

	char* mensaje = result->mensajes->mensajes[1];
	int procesoTripulante = traductorCharAInt((char)*mensaje);
	int idTripulante = atoi(mensaje+1);

	char** posiciones_spliteadas = string_split(result->mensajes->mensajes[2], "|");
	uint32_t pos_x = (uint32_t)atoi(posiciones_spliteadas[0]);
	uint32_t pos_y = (uint32_t)atoi(posiciones_spliteadas[1]);

	char* procesoTripulanteChar = string_itoa(procesoTripulante);

	char* aux = string_itoa(idTripulante+1);

	l_proceso_segmento* proceso = find_proceso_lista(procesoTripulanteChar, &tablaProcesos);

	l_segmento* segmentoTcb = find_segmento_lista(aux, proceso->punteroTablaSegmentos);//El id tripulante es global.

	t_tcb* tcb = (t_tcb*)segmentoTcb->direccion_inicio;

	tcb->pos_x = pos_x;
	tcb->pos_y = pos_y;

	int resultado = calcularNumTripulante(proceso, idTripulante);

	if(resultado == -1){
		char string_log[200];
		sprintf(string_log, "No se encontro el tripulante global.");
		log_info(logger, string_log);
	}

	sem_wait(sem_mutex_mapa);
	item_mover(nivel, traductorTripAChar(resultado), pos_x, pos_y);
	dibujar();
	sem_post(sem_mutex_mapa);

	char* mensajites[1] = {"OK"};
	send_messages_socket(result->socket, mensajites, 1);

	free_string_split(posiciones_spliteadas);
	free(procesoTripulanteChar);
	free(aux);
}

void handle_enviar_proxima_tarea(t_result* result){


	char* mensaje = result->mensajes->mensajes[1];
	int procesoTripulante = traductorCharAInt((char)*mensaje);
	int idTripulante = atoi(mensaje+1);

	char* aux = string_itoa(idTripulante+1);

	char* procesoTripulanteChar = string_itoa(procesoTripulante);

	l_proceso_segmento* proceso = find_proceso_lista(procesoTripulanteChar, &tablaProcesos);

	l_segmento* segmentoTcb = find_segmento_lista(aux, proceso->punteroTablaSegmentos);//El id tripulante es global.

	t_tcb* tcb = (t_tcb*)segmentoTcb->direccion_inicio;

	uint32_t desplazamiento = tcb->puntero_PCB;

	t_pcb* pcb = malloc(sizeof(t_pcb));//(t_pcb*)puntero_memoria_principal+desplazamiento;// Si saco este malloc anda mal.
	memcpy(pcb, puntero_memoria_principal+desplazamiento, sizeof(t_pcb));

	l_segmento* segmento = backlist(*proceso->punteroTablaSegmentos);

	l_segmento* segmentoTareas = find_segmento_lista(segmento->idSegmento ,proceso->punteroTablaSegmentos);

	char* tareas = malloc(segmentoTareas->tamanio_segmento);
	memcpy(tareas, puntero_memoria_principal + pcb->direccionInicioTareas, segmentoTareas->tamanio_segmento);

	char string_log[200];
	sprintf(string_log, "Las tareas copiadas son: %s", tareas);
	log_info(logger, string_log);

	char** tareasSeparadas = string_split(tareas, "|");

	int cantidadTareas = 0;

	while(tareasSeparadas[cantidadTareas] != NULL){
		cantidadTareas++;
	}
	char* tareaActual = tareasSeparadas[tcb->proxima_instruccion_a_ejecutar];
	resolverTarea(tareaActual);

	char* mensajites[1];

	uint32_t valorMagico = tcb->proxima_instruccion_a_ejecutar;

	if(valorMagico < cantidadTareas){

		char* proximaTareaAEjecutar = tareasSeparadas[valorMagico];

		char string_log[200];
		sprintf(string_log, "La proxima tarea a ejecutar es: %s", proximaTareaAEjecutar);
		log_info(logger, string_log);

		tcb->proxima_instruccion_a_ejecutar = valorMagico+1;
		mensajites[0] = proximaTareaAEjecutar;

	}else{
		mensajites[0] = "NO";
		char string_log[200];
		sprintf(string_log,"Ooops: No existe proxima tarea a ejecutar");
		log_info(logger, string_log);
	}

	send_messages_socket(result->socket, mensajites, 1);

	free(pcb);
	free(tareas);
	free(procesoTripulanteChar);
	free(aux);
	free_string_split(tareasSeparadas);

}

void handle_actualizar_estado(t_result* result){

	char* mensaje = result->mensajes->mensajes[1];
	int procesoTripulante = traductorCharAInt((char)*mensaje);
	int idTripulanteAActualizar = atoi(mensaje+1);
	char nuevoEstado = (char)*result->mensajes->mensajes[2];

	char* procesoTripulanteChar = string_itoa(procesoTripulante);
	char* aux = string_itoa(idTripulanteAActualizar+1);

	l_proceso_segmento* proceso = find_proceso_lista(procesoTripulanteChar, &tablaProcesos);

	l_segmento* segmentoTcb = find_segmento_lista(aux, proceso->punteroTablaSegmentos);//El id tripulante es global.

	t_tcb* tcb = (t_tcb*)segmentoTcb->direccion_inicio;

	char viejoEstado = tcb->estadoTripulante;
	tcb->estadoTripulante = nuevoEstado;

	char string_log[200];
	sprintf(string_log, "El tripulante %d ha cambiado su estado de '%c' a '%c'", tcb->idTripulante, viejoEstado, nuevoEstado);
	log_info(logger, string_log);

	char* mensajites[1] = {"OK"};
	send_messages_socket(result->socket, mensajites, 1);

	free(procesoTripulanteChar);
	free(aux);
}

void handle_expulsar_tripulante(t_result* result){

	sem_wait(sem_mutex_inicio_patota);

	char* mensaje = result->mensajes->mensajes[1];
	int procesoTripulante = traductorCharAInt((char)*mensaje);
	int idTripulanteAExpulsar = atoi(mensaje+1);

	char* procesoTripulanteChar = string_itoa(procesoTripulante);
	char* aux = string_itoa(idTripulanteAExpulsar+1);

	l_proceso_segmento* proceso = find_proceso_lista(procesoTripulanteChar, &tablaProcesos);

	l_segmento* segmentoTcb = find_segmento_lista(aux, proceso->punteroTablaSegmentos);//El id tripulante es global.

	if(segmentoTcb == NULL){
		char string_log2[200];
		sprintf(string_log2, "El tripulante no existe");
		log_info(logger, string_log2);
	}

	l_segmento* segmento = (l_segmento*)eliminar_segmento_lista(aux, proceso->punteroTablaSegmentos);//Tiene +2 porque se supone que la tabla de segmentos tiene tareas, pcb y todos los tripulantes.
	//l_segmento* segmento2 = (l_segmento*)eliminar_segmento_lista_global(segmentoTcb->byteInicio, &l_particiones);

	if(segmento == NULL){
		printf("El segmento que desea eliminar no existe.\n");
		sem_post(sem_mutex_inicio_patota);
		return;
	}

	segmento->ocupado = 0;

	char string_log[200];
	sprintf(string_log, "Se elimino el segmento de la tabla de segmentos local: \n Segmento: %s | Inicio: %p | Tam: %db", segmento->idSegmento, segmento->direccion_inicio, segmento->tamanio_segmento);
	log_info(logger, string_log);
	//sprintf(string_log, "Se elimino el segmento de la tabla de segmentos global: \n Segmento: %s | Inicio: %p | Tam: %db", segmento2->idSegmento, segmento2->direccion_inicio, segmento2->tamanio_segmento);
	//log_info(logger, string_log);

	int resultado = calcularNumTripulante(proceso, idTripulanteAExpulsar);

	sem_wait(sem_mutex_mapa);
	item_borrar(nivel, traductorTripAChar(resultado));
	dibujar();
	sem_post(sem_mutex_mapa);

	free(segmentoTcb->idSegmento);
	//free(segmentoTcb);

	if(esBorrableElProcesoSegmentacion(proceso)){
		l_segmento* segmentoAux = backlist(*proceso->punteroTablaSegmentos);
		segmento = (l_segmento*)eliminar_segmento_lista(segmentoAux->idSegmento, proceso->punteroTablaSegmentos);
		//eliminar_segmento_lista_global(segmento->byteInicio, &l_particiones);
		free(segmento->idSegmento);
		segmento->ocupado = 0;
		//free(segmento);
		segmento = (l_segmento*)eliminar_segmento_lista("0", proceso->punteroTablaSegmentos);
		//eliminar_segmento_lista_global(segmento->byteInicio, &l_particiones);
		free(segmento->idSegmento);
		segmento->ocupado = 0;
		//free(segmento);
		emptylist(proceso->punteroTablaSegmentos);
		eliminar_proceso_lista(&tablaProcesos, proceso);
		free(proceso->punteroTablaSegmentos);
		free(proceso);
	}

	char* mensajites[1] = {"OK"};
	send_messages_socket(result->socket, mensajites, 1);

	free(procesoTripulanteChar);
	free(aux);

	sem_post(sem_mutex_inicio_patota);
}

//---------------UTILITARIAS---------------

int calcularNumTripulante(void* procesoABuscar, int numTripulante){

	l_procesoAux* proceso = NULL;

	IteratorList iterador = NULL;

	int sumatoria = 0;

	for(iterador = beginlist(tablaProcesosAux);iterador!=NULL;iterador = nextlist(iterador)){
		proceso = dataiterlist(iterador);

		uint32_t aux;
		memcpy(&aux, procesoABuscar, sizeof(uint32_t));

		if(proceso->idProceso == aux){
			return sumatoria + numTripulante;
		}

		sumatoria = sumatoria + proceso->cantTripulantes;

	}

	return -1;

}

void free_string_split(char** string){

	int i = 0;
	while(string[i] != NULL){
		free(string[i]);
		i++;
	}
	free(string);
}

void resolverTarea(char* tarea){
	/*char** tareas_spliteadas;
	char** tarea_spliteada = string_split(tarea, " ");

	if(tarea_spliteada[1] != NULL){
		tareas_spliteadas = string_split(tarea_spliteada[1], ";");
	} else{
		tareas_spliteadas = NULL;//string_split(tarea, ";");//Me interesa hacer esto? si la tarea no es printeable en el mapa no tengo que hacer nada.
	}
	if(tareas_spliteadas != NULL){
		char* nombre_tarea = tarea_spliteada[0];
		uint32_t cantidadRecursos = atoi(tareas_spliteadas[0]);
		uint32_t pos_x = atoi(tareas_spliteadas[1]);
		uint32_t pos_y = atoi(tareas_spliteadas[2]);

		//handle_tarea(nombre_tarea, cantidadRecursos, pos_x, pos_y);
		free_string_split(tareas_spliteadas);
	}
	free_string_split(tarea_spliteada);
	*/
}

int validarItem(char id, uint32_t pos_x, uint32_t pos_y, uint32_t cantidadRecursos){
	err = caja_crear(nivel, id, pos_x, pos_y, cantidadRecursos);
	char* error = nivel_gui_string_error(err);

	if (!strcmp(error, "El item ya existe.")){
		return 1;
	}
	return 0;
}

void handle_tarea(char* tarea, uint32_t cantidadRecursos, uint32_t pos_x, uint32_t pos_y){

	int booleano;
	if(!strcmp(tarea, "GENERAR_OXIGENO")){
		booleano = validarItem('@', pos_x, pos_y, cantidadRecursos);
		if(booleano){
			agregar_n_recursos('@', cantidadRecursos);
		}
		return;

	}else if(!strcmp(tarea, "CONSUMIR_OXIGENO")){
		booleano = validarItem('@', pos_x, pos_y, cantidadRecursos);
		if(booleano){
			quitar_n_recursos('@', cantidadRecursos);
		}
		return;

	}else if(!strcmp(tarea, "GENERAR_COMIDA")){
		booleano = validarItem('$', pos_x, pos_y, cantidadRecursos);
		if(booleano){
			agregar_n_recursos('$', cantidadRecursos);
		}
		return;

	}else if(!strcmp(tarea, "CONSUMIR_COMIDA")){
		booleano = validarItem('$', pos_x, pos_y, cantidadRecursos);
		if(booleano){
			quitar_n_recursos('$', cantidadRecursos);
		}
		return;

	}else if(!strcmp(tarea, "GENERAR_BASURA")){
		booleano = validarItem('#', pos_x, pos_y, cantidadRecursos);
		if(booleano){
			agregar_n_recursos('#', cantidadRecursos);
		}
		return;

	}else if(!strcmp(tarea, "DESCARTAR_BASURA")){
		booleano = validarItem('#', pos_x, pos_y, cantidadRecursos);
		if(booleano){
			quitar_n_recursos('#', cantidadRecursos);
		}
		return;

	}else{
		return;
	}
}

void agregar_n_recursos(char id, int n){
	for(int i=0;i<n;i++){
		caja_agregar_recurso(nivel, id);
	}
}

void quitar_n_recursos(char id, int n){
	for(int i=0;i<n;i++){
		caja_quitar_recurso(nivel, id);
	}
}

void imprimirPuntero(void* puntero, int tamanioTareas, int cantidadTripulantes){

	char* direccionTareas = malloc(tamanioTareas);
	memcpy(direccionTareas, puntero, tamanioTareas);

	char string_log3[200];
	sprintf(string_log3, "------------------------");
	log_info(logger, string_log3);

	char string_log4[200];
	sprintf(string_log4, "\tTAREA: %s", direccionTareas);
	log_info(logger, string_log4);

	t_pcb* pcb = malloc(8);
	memcpy(pcb, puntero+tamanioTareas, 8);

	char string_log5[300];
	sprintf(string_log5, "\tID: %d | puntero_tarea: %d", pcb->idPatota, pcb->direccionInicioTareas);
	log_info(logger, string_log5);

	free(pcb);

	for(int i = 0; i < cantidadTripulantes; i++){

		t_tcb* tcb = malloc(21);
		memcpy(tcb, puntero+tamanioTareas+sizeof(t_pcb)+(i*21), 21);

		char string_log7[1000];
		sprintf(string_log7, "\tID: %d | Estado: %c | pos_x: %d | pos_y: %d | proxima_instruccion_a_ejecutar: %d | puntero_PCB: %d", tcb->idTripulante, tcb->estadoTripulante, tcb->pos_x, tcb->pos_y, tcb->proxima_instruccion_a_ejecutar, tcb->puntero_PCB);
		log_info(logger, string_log7);

		free(tcb);
	}

}

int traductorCharAInt(char charmander){

	if(charmander == 'A'){
			return 1;
		}
	if(charmander == 'B'){
			return 2;
		}
	if(charmander == 'C'){
			return 3;
		}
	if(charmander == 'D'){
			return 4;
		}
	if(charmander == 'E'){
			return 5;
		}
	if(charmander == 'F'){
			return 6;
		}
	if(charmander == 'G'){
			return 7;
		}
	if(charmander == 'H'){
			return 8;
		}
	if(charmander == 'I'){
			return 9;
		}
	if(charmander == 'J'){
			return 10;
		}
	if(charmander == 'K'){
			return 11;
		}
	if(charmander == 'L'){
			return 12;
		}
	if(charmander == 'M'){
			return 13;
		}
	if(charmander == 'N'){
			return 14;
		}
	if(charmander == 'O'){
			return 15;
		}
	if(charmander == 'P'){
			return 16;
		}
	if(charmander == 'Q'){
			return 17;
		}
	if(charmander == 'R'){
			return 18;
		}
	if(charmander == 'S'){
			return 19;
		}
	if(charmander == 'T'){
			return 20;
		}
	if(charmander == 'U'){
			return 21;
		}
	if(charmander == 'V'){
			return 22;
		}
	if(charmander == 'W'){
			return 23;
		}
	if(charmander == 'X'){
			return 24;
		}
	if(charmander == 'Y'){
			return 25;
		}
	if(charmander == 'Z'){
			return 26;
		}
	return -1;

	/*
		int charmanderAux =  (int)charmander;

		return charmanderAux-16;*/
}


char traductorTripAChar(int tripulante){

	char aux = 'A';

	return aux + tripulante;
}

