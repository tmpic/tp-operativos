#include "../include/pedidos_mensajes.h"

void handle_client(t_result* result){
	sem_wait(sem4);
	char* tipo_mensaje = result->mensajes->mensajes[0];
	if (!strcmp(tipo_mensaje, "INICIAR_PATOTA")){ // CANTIDAD_TRIPULANTES PATH_TAREAS UBICACION_TRIPULANTES(Potencialmente = CANTIDAD_TRIPULANTES)
		if(!strcmp(miramhq_config->esquema_memoria, "SEGMENTACION")){

			handle_iniciar_patota(result);
			imprimirTodoSegmentacion();

		}else if(!strcmp(miramhq_config->esquema_memoria, "PAGINACION")){

			handle_iniciar_patota_paginacion2(result);

		}

	}else if(!strcmp(tipo_mensaje, "RECIBIR_UBICACION_TRIPULANTE")){

		if(!strcmp(miramhq_config->esquema_memoria, "SEGMENTACION")){

			handle_recibir_ubicacion_tripulante(result);

		}else if(!strcmp(miramhq_config->esquema_memoria, "PAGINACION")){

			handle_recibir_ubicacion_tripulante_paginacion(result);
		}

	}else if(!strcmp(tipo_mensaje, "ENVIAR_PROXIMA_TAREA")){
		if(!strcmp(miramhq_config->esquema_memoria, "SEGMENTACION")){

			handle_enviar_proxima_tarea(result);

		}else if(!strcmp(miramhq_config->esquema_memoria, "PAGINACION")){

			handle_enviar_proxima_tarea_paginacion(result);
		}

	}else if(!strcmp(tipo_mensaje, "ACTUALIZAR_ESTADO")){
		if(!strcmp(miramhq_config->esquema_memoria, "SEGMENTACION")){

			handle_actualizar_estado(result);

		}else if(!strcmp(miramhq_config->esquema_memoria, "PAGINACION")){

			handle_actualizar_estado_paginacion(result);
		}

	}else if(!strcmp(tipo_mensaje, "EXPULSAR_TRIPULANTE")){

		if(!strcmp(miramhq_config->esquema_memoria, "SEGMENTACION")){
			handle_expulsar_tripulante(result);

		}else if(!strcmp(miramhq_config->esquema_memoria, "PAGINACION")){

			handle_expulsar_tripulante_paginacion(result);
		}
	}
	sem_post(sem4);
}

//HANDLERS PAGINACION

void handle_iniciar_patota_paginacion2(t_result* result){

	int cantidad_tripulantes = atoi(result->mensajes->mensajes[1]);

	int tamanioTareas = strlen(result->mensajes->mensajes[2])+1;

	void* aux = malloc(tamanioTareas);

	memcpy(aux, (void*)result->mensajes->mensajes[2], tamanioTareas);

	int recorridoTotal = 0;

	l_proceso* proceso = crearProcesoPaginacion(contadorProcesosGlobal);
	proceso->tamanioTareas = tamanioTareas;
	proceso->cantTripulantes = cantidad_tripulantes;

	l_procesoAux* proceso2 = malloc(sizeof(l_procesoAux));
	proceso2->cantTripulantes = cantidad_tripulantes;
	proceso2->idProceso = contadorProcesosGlobal;
	pushbacklist(&tablaProcesosAux, proceso2);

	int tamanioTotal = (cantidad_tripulantes*sizeof(t_tcb) + sizeof(t_pcb) + tamanioTareas);

	List lista_paginas_parcial;
	initlist(&lista_paginas_parcial);

	int auxWhile = tamanioTotal;

	while(auxWhile >= miramhq_config->tamanio_pagina){

		l_pagina* pagina = crear_pagina(proceso);

		pushfrontlist(&lista_paginas_parcial, pagina);

		if(auxWhile - miramhq_config->tamanio_pagina < miramhq_config->tamanio_pagina){
			l_pagina* pagina = crear_pagina(proceso);
			pushfrontlist(&lista_paginas_parcial, pagina);
		}

		auxWhile -= miramhq_config->tamanio_pagina;
	}

	void* puntero_total = malloc(tamanioTotal);

	t_pcb* pcb = (t_pcb*)crearPcb((uint32_t)contadorProcesosGlobal, 0);
	contadorProcesosGlobal++;

	memcpy(puntero_total, pcb, sizeof(t_pcb));

	recorridoTotal += sizeof(t_pcb);

	free(pcb);

	//Cargue el PCB.
	int tripulantesEn00 = cantidad_tripulantes + 3 - (int)*result->mensajes->size;

	int tripulantesConPosicion = cantidad_tripulantes - tripulantesEn00;

	int contadorDeTcbs = 0;
	t_tcb* tcb = NULL;

	int auxPCB = recorridoTotal;

	while(contadorDeTcbs < cantidad_tripulantes){

		for(int tripulantesConPosicionCreados = 0; tripulantesConPosicionCreados < tripulantesConPosicion; tripulantesConPosicionCreados++){
			char** posiciones_spliteadas = string_split(result->mensajes->mensajes[contadorDeTcbs+3], "|");
			uint32_t pos_x = (uint32_t)atoi(posiciones_spliteadas[0]);
			uint32_t pos_y = (uint32_t)atoi(posiciones_spliteadas[1]);

			tcb = crearTcb((uint32_t)contadorTripulantesGlobal, 'R', pos_x, pos_y, 0, auxPCB-sizeof(t_pcb));
			memcpy(puntero_total+recorridoTotal, tcb, sizeof(t_tcb));
			contadorTripulantesGlobal++;
			contadorDeTcbs++;

			sem_wait(sem_mutex_mapa);
			personaje_crear(nivel, traductorTripAChar(spawnerDeTripulantes), pos_x, pos_y);
			spawnerDeTripulantes++;
			dibujar();
			sem_post(sem_mutex_mapa);

			free_string_split(posiciones_spliteadas);

			recorridoTotal += sizeof(t_tcb);

			free(tcb);

		}
		for(int tripulantesEn00Creados = 0;tripulantesEn00Creados < tripulantesEn00;tripulantesEn00Creados++){

			tcb = crearTcb((uint32_t)contadorTripulantesGlobal, 'R', 0, 0, 0, auxPCB-sizeof(t_pcb));
			memcpy(puntero_total+recorridoTotal, tcb, sizeof(t_tcb));
			contadorTripulantesGlobal++;
			contadorDeTcbs++;

			sem_wait(sem_mutex_mapa);
			personaje_crear(nivel, traductorTripAChar(spawnerDeTripulantes), 0, 0);
			spawnerDeTripulantes++;
			dibujar();
			sem_post(sem_mutex_mapa);

			recorridoTotal += sizeof(t_tcb);
			free(tcb);

		}

	}

	int min (int x, int y)
	{
	if (x>y) {
	return (y);
	}  else  {
	return (x);
	}
	}
	int variable = sizelist(lista_paginas_parcial);

	memcpy(puntero_total+recorridoTotal, aux, tamanioTareas);

	recorridoTotal += tamanioTareas;

	for(int i = 0; variable > i;i++){
		void* pagina = popbacklist(&lista_paginas_parcial);

		int bytesACargar = min(miramhq_config->tamanio_pagina, recorridoTotal);

		cargar_bytes_pagina(puntero_total+(i*miramhq_config->tamanio_pagina), bytesACargar, pagina);

		recorridoTotal -= miramhq_config->tamanio_pagina;

	}
	//AGREGUE ESTO PARA MANDAR LA TAREA EN UN MENSAJE
	char** tareas_spliteadas = string_split(result->mensajes->mensajes[2], "|");
	char* primerTarea = tareas_spliteadas[0];

	char* mensajites[1] = {primerTarea};
	send_messages_socket(result->socket, mensajites, 1);

	resolverTarea(primerTarea);

	free_string_split(tareas_spliteadas);
	free(aux);
	free(puntero_total);
	emptylist(&lista_paginas_parcial);
	numeroPaginaGlobal = 0;

}

void handle_recibir_ubicacion_tripulante_paginacion(t_result* result){

	char* mensaje = result->mensajes->mensajes[1];
	int procesoTripulante = traductorCharAInt((char)*mensaje);
	int idTripulante = atoi(mensaje+1);


	char** posiciones_spliteadas = string_split(result->mensajes->mensajes[2], "|");
	uint32_t pos_x = (uint32_t)atoi(posiciones_spliteadas[0]);
	uint32_t pos_y = (uint32_t)atoi(posiciones_spliteadas[1]);

	char* procesoTripulanteChar = string_itoa(procesoTripulante);

	l_proceso* proceso = find_proceso_lista_paginacion(procesoTripulanteChar, &tablaProcesos);

	modificarTripulanteDeLaEstructura(idTripulante, proceso, 5, &pos_x, sizeof(uint32_t));

	modificarTripulanteDeLaEstructura(idTripulante, proceso, 9, &pos_y, sizeof(uint32_t));

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

}

void handle_enviar_proxima_tarea_paginacion(t_result* result){

    char* mensaje = result->mensajes->mensajes[1];
    int procesoTripulante = traductorCharAInt((char)*mensaje);
    int idTripulante = atoi(mensaje+1);

    char* procesoTripulanteChar = string_itoa(procesoTripulante);

    l_proceso* proceso = find_proceso_lista_paginacion(procesoTripulanteChar, &tablaProcesos);

    obtenerLoQueSea_result* resultReturn = obtenerLoQueSea(sizeof(t_pcb)+(idTripulante*(sizeof(t_tcb))), sizeof(t_tcb), proceso);

    void* direccionTCB = resultReturn->punteroALoRequisado;

    t_tcb* tcb = (t_tcb*)direccionTCB;

    char* tareasACopiar = obtenerTareas(proceso);

    char string_log[200];
    sprintf(string_log, "Las tareas copiadas son: %s", tareasACopiar);
    log_info(logger, string_log);

    char** tareasSeparadas = string_split(tareasACopiar, "|");

    int cantidadTareas = 0;

    while(tareasSeparadas[cantidadTareas] != NULL){
        cantidadTareas++;
    }

    char* tareaActual = tareasSeparadas[tcb->proxima_instruccion_a_ejecutar];
    char* tareaAnterior = tareasSeparadas[tcb->proxima_instruccion_a_ejecutar-1];

    int i = 0;
    int contador = 0;

    while(tareasSeparadas[contador] != tareaActual){
        i += string_length(tareasSeparadas[contador]);
        contador++;
    }

    if(tareaActual != NULL){
        obtenerTareasYActivarlas(proceso, i, string_length(tareaActual));
    }else{
    	obtenerTareasYActivarlas(proceso, i-string_length(tareaAnterior), string_length(tareaAnterior));
    }

    resolverTarea(tareaActual);

    char* mensajites[1];

    if(tcb->proxima_instruccion_a_ejecutar < cantidadTareas){
        char* proximaTareaAEjecutar = tareasSeparadas[(tcb->proxima_instruccion_a_ejecutar)];

        char string_log[200];
        sprintf(string_log, "La proxima tarea a ejecutar es: %s", proximaTareaAEjecutar);
        log_info(logger, string_log);

        int valorMagico = tcb->proxima_instruccion_a_ejecutar + 1;

        modificarTripulanteDeLaEstructura(idTripulante, proceso, 13, &valorMagico, sizeof(uint32_t));

        mensajites[0] = proximaTareaAEjecutar;



    }else{

        mensajites[0] = "NO";
        char string_log[200];
        sprintf(string_log,"Ooops: No existe proxima tarea a ejecutar\n");
        log_info(logger, string_log);
    }

    send_messages_socket(result->socket, mensajites, 1);


    free(resultReturn->punteroALoRequisado);
    free(resultReturn);
    free(tareasACopiar);
    free(procesoTripulanteChar);
    free_string_split(tareasSeparadas);
}

void handle_actualizar_estado_paginacion(t_result* result){

	char* mensaje = result->mensajes->mensajes[1];
	int procesoTripulante = traductorCharAInt((char)*mensaje);
	int idTripulante = atoi(mensaje+1);
	char estadoAActualizar = *result->mensajes->mensajes[2];

	char* procesoTripulanteChar = string_itoa(procesoTripulante);

	l_proceso* proceso = find_proceso_lista_paginacion(procesoTripulanteChar, &tablaProcesos);

	modificarTripulanteDeLaEstructura(idTripulante, proceso, 4, &estadoAActualizar, sizeof(char));

	char* mensajites[1] = {"OK"};
	send_messages_socket(result->socket, mensajites, 1);

	free(procesoTripulanteChar);
}

void handle_expulsar_tripulante_paginacion(t_result* result){

	char* mensaje = result->mensajes->mensajes[1];
	int procesoTripulante = traductorCharAInt((char)*mensaje);
	int idTripulanteAExpulsar = atoi(mensaje+1);

	char* procesoTripulanteChar = string_itoa(procesoTripulante);

	l_proceso* proceso = find_proceso_lista_paginacion(procesoTripulanteChar, &tablaProcesos);

	int resultado = calcularNumTripulante(proceso, idTripulanteAExpulsar);

	eliminarTripulante(idTripulanteAExpulsar,  proceso);

	if(resultado == -1){
		char string_log[200];
		sprintf(string_log, "No se encontro el tripulante global.");
		log_info(logger, string_log);
	}

	sem_wait(sem_mutex_mapa);
	item_borrar(nivel, traductorTripAChar(resultado));
	dibujar();
	sem_post(sem_mutex_mapa);

	char* mensajites[1] = {"OK"};
	send_messages_socket(result->socket, mensajites, 1);

	free(procesoTripulanteChar);
}








