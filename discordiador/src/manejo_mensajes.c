#include "../include/manejo_mensajes.h"

int enviar_mensaje_modulo(t_modulo* modulo, char** mensaje, int cantidadMensajes){

    int socketReturn;

    if(modulo->socket == 0){
    	sem_wait(sem_enviar_msj);
        socketReturn = send_messages_and_return_socket(modulo->nombre,modulo->ip, modulo->puerto, mensaje, cantidadMensajes);
        sem_post(sem_enviar_msj);
    } else {
        socketReturn = modulo->socket;
        send_message_socket(modulo->socket, modulo->nombre);
        send_messages_socket(modulo->socket, mensaje, cantidadMensajes);
    }

    return socketReturn;
}

char* enviar_mensaje_iniciar_patota(t_modulo* modulo, t_patota* patota){

	char** mensajes = malloc(sizeof(char*)*(3+sizelist(*patota->posiciones)));

	int i = 0;

	mensajes[i] = string_new();
	string_append(&mensajes[i], "INICIAR_PATOTA");
	i++;
	mensajes[i] = string_itoa(sizelist(*patota->tripulantes));
	i++;
	mensajes[i] = string_duplicate(patota->tarea);
	i++;
	for(int j = 0; j < sizelist(*patota->posiciones); j++){

		mensajes[i] = (char*)atlist(*patota->posiciones, j);
		i++;

	}

	int socket;

	socket = enviar_mensaje_modulo(modulo, mensajes, i);

	t_mensajes* respuesta = receive_simple_messages(socket);

	if(modulo->socket <= 0){
		liberar_conexion(socket);
	}

	free(mensajes[0]);
	free(mensajes[1]);
	free(mensajes[2]);
	free(mensajes);

	char* respuestaReturn = string_duplicate(respuesta->mensajes[0]);

	free(respuesta->mensajes[0]);
	free(respuesta->mensajes);
	free(respuesta->size);
	free(respuesta);

	return respuestaReturn;

}

char* enviar_mensaje_proxima_tarea(t_modulo* modulo, char* patota, char* tid ){

	char* tarea = "ENVIAR_PROXIMA_TAREA";

	char* patotaTid = unir_pid_tid(patota, tid);

	char* mensajes[2] ={tarea, patotaTid};

	int socket;

	socket = enviar_mensaje_modulo(modulo, mensajes, 2);

	t_mensajes* respuesta = receive_simple_messages(socket);

	if(modulo->socket <= 0){
		liberar_conexion(socket);
	}

	char* respuestaReturn = string_new();

	string_append(&respuestaReturn, respuesta->mensajes[0]);

	free(patotaTid);
	free(respuesta->mensajes[0]);
	free(respuesta->mensajes);
	free(respuesta->size);
	free(respuesta);
	free(tid);

	return respuestaReturn;

}

void enviar_mensaje_mover_tripulante(t_modulo* modulo, char* patota, char* tid, char* posx, char* posy){

	char* tarea = "RECIBIR_UBICACION_TRIPULANTE";

	char* patotaTid = unir_pid_tid(patota, tid);

	char* envio = string_new();

	string_append(&envio, posx);
	string_append(&envio, "|");
	string_append(&envio, posy);

	char* mensajes[3] ={tarea, patotaTid, envio};

	int socket;

	socket = enviar_mensaje_modulo(modulo, mensajes, 3);

	t_mensajes* respuesta = receive_simple_messages(socket);

	if(strcmp(respuesta->mensajes[0],"OK") != 0){
		printf("MOVER_TRIPULANTE NO ESTA OK\n");
	}

	if(modulo->socket <= 0){
		liberar_conexion(socket);
	}
	free(respuesta->mensajes[0]);
	free(respuesta->mensajes);
	free(respuesta->size);
	free(respuesta);
	free(envio);
	free(patotaTid);

}

char* enviar_mensaje_expulsar_tripulante(t_modulo* modulo, char* patotaTid){
	char* tarea = "EXPULSAR_TRIPULANTE";

	char* mensajes[2] ={tarea, patotaTid};

	int socket;

	socket = enviar_mensaje_modulo(modulo, mensajes, 2);

	t_mensajes* respuesta = receive_simple_messages(socket);

	if(modulo->socket <= 0){
		liberar_conexion(socket);
	}

	char* respuestaReturn = string_new();

	string_append(&respuestaReturn, respuesta->mensajes[0]);

	free(patotaTid);
	free(respuesta->mensajes[0]);
	free(respuesta->mensajes);
	free(respuesta->size);
	free(respuesta);

	return respuestaReturn;
}

void enviar_mensaje_cambio_estado(t_modulo* modulo, char* patota, char* tid, t_status estado){
	char* tarea = "ACTUALIZAR_ESTADO";

	char* patotaTid = unir_pid_tid(patota, tid);
	char* estadoString = obtener_status_char(estado);

	printf("---------------------------------------------------------------------ESTADO: %s\n", estadoString);

	char* mensajes[3] ={tarea, patotaTid, estadoString};

	int socket;

	socket = enviar_mensaje_modulo(modulo, mensajes, 3);

	t_mensajes* respuesta = receive_simple_messages(socket);

	if(strcmp(respuesta->mensajes[0],"OK") != 0){
		printf("CAMBIO_ESTADO NO ESTA OK\n");
	}

	if(modulo->socket <= 0){
		liberar_conexion(socket);
	}
	free(respuesta->mensajes[0]);
	free(respuesta->mensajes);
	free(respuesta->size);
	free(respuesta);
	free(patotaTid);
	free(estadoString);
	free(tid);
}

void enviar_mensaje_tarea(t_modulo* modulo, t_tarea* tarea){

	char* cantidad = string_itoa(tarea->cantidad);

	char string_log[200];
	sprintf(string_log, "ENVIAR_MENSAJE_TAREA: %s - %d", tarea->nombre_tarea, tarea->cantidad);
	log_info(logger, string_log);

	char* mensajes[2] ={tarea->nombre_tarea, cantidad};

	int socket;

	socket = enviar_mensaje_modulo(modulo, mensajes, 2);

	//t_mensajes* respuesta = receive_simple_messages(socket);

	if(modulo->socket <= 0){
		liberar_conexion(socket);
	}
	//free(respuesta->mensajes[0]);
	//free(respuesta->mensajes);
	//free(respuesta->size);
	//free(respuesta);
	free(cantidad);
}

void enviar_mensaje_nuevo_tripulante(t_modulo* modulo, char* pid, int tid){
	char* tarea = "NUEVO_TRIPULANTE";

	char* idTripulante = string_itoa(calcularNumTripulanteGlobal(pid, tid));

	char* mensajes[2] ={tarea, idTripulante};

	int socket;

	socket = enviar_mensaje_modulo(modulo, mensajes, 2);

	char* respuesta = receive_simple_message(socket);

	if(strcmp(respuesta,"Tripulante cargado OK") != 0){
		printf("NUEVO_TRIPULANTE FILESYSTEM NO ESTA OK\n");
	}

	if(modulo->socket <= 0){
		liberar_conexion(socket);
	}

	free(respuesta);
	free(idTripulante);
}

char* enviar_mensaje_obtener_bitacora(t_modulo* modulo, char* pid, int tid){
	char* tarea = "OBTENER_BITACORA";

	char* idTripulante = string_itoa(calcularNumTripulanteGlobal(pid, tid));

	char* mensajes[2] ={tarea, idTripulante};

	int socket;

	socket = enviar_mensaje_modulo(modulo, mensajes, 2);

	t_mensajes* respuesta = receive_simple_messages(socket);

	if(respuesta->mensajes[0] == NULL){
		printf("ERROR RARO DE LA BITACORA POR VALGRIND\n");
		return "ERROR RARO DE LA BITACORA POR VALGRIND\n";
	}

	if(modulo->socket <= 0){
		liberar_conexion(socket);
	}

	char* respuestaReturn = string_new();

	string_append(&respuestaReturn, respuesta->mensajes[0]);

	free(respuesta->mensajes[0]);
	free(respuesta->mensajes);
	free(respuesta->size);
	free(respuesta);
	free(idTripulante);

	return respuestaReturn;
}

void enviar_mensaje_fsck(t_modulo* modulo){
	char* tarea = "FSCK";

	char* mensajes[1] ={tarea};

	int socket;

	socket = enviar_mensaje_modulo(modulo, mensajes, 1);

	//t_mensajes* respuesta = receive_simple_messages(socket);

	if(modulo->socket <= 0){
		liberar_conexion(socket);
	}

	//char* respuestaReturn = string_new();

	//string_append(&respuestaReturn, respuesta->mensajes[0]);

	//free(respuesta->mensajes[0]);
	//free(respuesta->mensajes);
	//free(respuesta->size);
	//free(respuesta);
}

void enviar_mensaje_notificar_fileSystem(t_modulo* modulo, char* mensaje, char* pid, int tid){
	char* tarea = "MENSAJE_TRIPULANTE";

	char* idTripulante = string_itoa(calcularNumTripulanteGlobal(pid, tid));

	char* mensajes[3] ={tarea, idTripulante, mensaje};

	int socket;

	socket = enviar_mensaje_modulo(modulo, mensajes, 3);

	//t_mensajes* respuesta = receive_simple_messages(socket);

	if(modulo->socket <= 0){
		liberar_conexion(socket);
	}

	//char* respuestaReturn = string_new();

	//string_append(&respuestaReturn, respuesta->mensajes[0]);

	//free(respuesta->mensajes[0]);
	//free(respuesta->mensajes);
	//free(respuesta->size);
	//free(respuesta);
	free(idTripulante);

}

int calcularNumTripulanteGlobal(char* pid, int tid){

	IteratorList it = NULL;
	int sumatoria = 0;

	for(it = beginlist(tripulantes_totales); it != NULL; it = nextlist(it)){

		t_tripulante* tripulante = (t_tripulante*) dataiterlist(it);

		if(!(strcmp(tripulante->pid, pid)) && tripulante->tid == tid){
			return sumatoria;
		}

		sumatoria += 1;

	}

	return sumatoria;
}

char* unir_pid_tid(char* patota, char* tid){
	char* patotaTid = string_new();
	string_append(&patotaTid, patota);
	string_append(&patotaTid, tid);

	return patotaTid;
}
