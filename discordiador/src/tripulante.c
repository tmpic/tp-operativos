#include "../include/tripulante.h"



void ejecutar_tripulante(t_tripulante* tripulante){
	sem_wait(tripulante->new_a_ready);
	//tripulante->tarea = solicitar_siguiente_tarea(tripulante);
	while (1) {
        laburar_en_nave(tripulante);
        if(tripulante->exit){
        	break;
        }
    }
}

void laburar_en_nave(t_tripulante* tripulante){

    ir_hacia_tarea(tripulante);

    realizar_tarea(tripulante);

    tripulante->tarea = solicitar_siguiente_tarea(tripulante);

    if(tripulante->exit){
    	pasar_a_exit(tripulante);
    	return;
    }

    if(tripulante->tarea == NULL){
    	desuscribirse_clock(tripulante->ciclo_cpu);
    	pasar_a_exit(tripulante);
		habilitar_exec(tripulante);
		tripulante->exit = 1;
		return;
	}
    if(tripulante->estado == BLOCK_IO){
    	tripulante->quantum = 0;
    	pasar_a_ready(tripulante);
    }
    if(cumplio_quantum(tripulante)){
    	desuscribirse_clock(tripulante->ciclo_cpu);
    	habilitar_exec(tripulante);
    	pasar_a_ready(tripulante);
    }


}

t_tarea* solicitar_siguiente_tarea(t_tripulante* tripulante){

	if(tripulante->exit){
		return NULL;
	}
	char* param1 = string_itoa(tripulante->tid);
	char* tarea = enviar_mensaje_proxima_tarea(modulo_miramhq, tripulante->pid, param1);

	if(tripulante->tarea != NULL){
		free(tripulante->tarea->nombre_tarea);
		free(tripulante->tarea);
	}

	if(!strcmp(tarea, "NO")){
		return NULL;
	}

	char** tareas_spliteadas;

	char** tarea_spliteada = string_split(tarea, " ");

	t_tarea* tareaReturn = malloc(sizeof(t_tarea));

	if(tarea_spliteada[1] != NULL){
		tareas_spliteadas = string_split(tarea_spliteada[1], ";");
		tareaReturn->cantidad = atoi(tareas_spliteadas[0]);
		free(tareas_spliteadas[0]);
		tareas_spliteadas[0] = string_duplicate(tarea_spliteada[0]);
		tareaReturn->compuesta = 1;
	} else {
		tareas_spliteadas = string_split(tarea, ";");
		tareaReturn->cantidad = 0;
		tareaReturn->compuesta = 0;
	}

	t_posicion posicion;

	posicion.posx = atoi(tareas_spliteadas[1]);
	posicion.posy = atoi(tareas_spliteadas[2]);

	tareaReturn->nombre_tarea = string_new();

	string_append(&tareaReturn->nombre_tarea, tareas_spliteadas[0]);
	tareaReturn->posicion = posicion;
	tareaReturn->duracion = atoi(tareas_spliteadas[3]);

	free_string_split(tareas_spliteadas);
	free_string_split(tarea_spliteada);
	free(tarea);

	return tareaReturn;

}

void ir_hacia_tarea(t_tripulante* tripulante){

	if(tripulante->primeraVez){
		sem_wait(tripulante->ready_a_exec);
		tripulante->tarea = solicitar_siguiente_tarea(tripulante);
		tripulante->primeraVez = 0;
	}
    t_tarea* tarea = tripulante->tarea;

    while(!misma_posicion(tripulante->posicion, tarea->posicion)){

        sem_wait(tripulante->ciclo_cpu);
        tripulante->quantum += 1;

        if(tripulante->exit){
        	return;
        }

        avanzar_hacia(tripulante, tarea->posicion);

        printf("TRIPULANTE %s%d SE MUEVE, NUEVAS COORDENADAS - X: %d - Y: %d\n", tripulante->pid, tripulante->tid, tripulante->posicion.posx, tripulante->posicion.posy);

        char* param1 = string_itoa(tripulante->tid);
        char* param2 = string_itoa(tripulante->posicion.posx);
        char* param3 = string_itoa(tripulante->posicion.posy);

        enviar_mensaje_mover_tripulante(modulo_miramhq, tripulante->pid, param1, param2, param3);

        char* respuesta = string_new();

        string_append_with_format(&respuesta, "Se movio a %s - %s\n", param2, param3);

        enviar_mensaje_notificar_fileSystem(modulo_imongo, respuesta, tripulante->pid, tripulante->tid);

        free(param1);
        free(param2);
        free(param3);
        free(respuesta);



        if(cumplio_quantum(tripulante)){
			desuscribirse_clock(tripulante->ciclo_cpu);
			habilitar_exec(tripulante);
			pasar_a_ready(tripulante);
		}
    }
}

void realizar_tarea(t_tripulante* tripulante){

	if(tripulante->exit){
		return;
	}

    if(!tripulante->tarea->compuesta){

    	for (int i = 0; i < tripulante->tarea->duracion; i++){
			sem_wait(tripulante->ciclo_cpu);
			tripulante->quantum += 1;
			if(tripulante->exit){
				return;
			}
			printf("TRIPULANTE %s%d REALIZA TAREA - %s\n", tripulante->pid,tripulante->tid, tripulante->tarea->nombre_tarea);
			if(cumplio_quantum(tripulante)){
				desuscribirse_clock(tripulante->ciclo_cpu);
				if(i+1 >= tripulante->tarea->duracion){
					tripulante->quantum = config_discordiador->quantum;
					break;
				}
				habilitar_exec(tripulante);
				pasar_a_ready(tripulante);
			}
		}

    	return;

    }

    char* respuesta = string_new();

    string_append_with_format(&respuesta, "Inicio la tarea: %s\n", tripulante->tarea->nombre_tarea);

    enviar_mensaje_notificar_fileSystem(modulo_imongo, respuesta, tripulante->pid, tripulante->tid);

    sem_wait(tripulante->ciclo_cpu);

    if(tripulante->tarea == NULL){
		return;
	}

    desuscribirse_clock(tripulante->ciclo_cpu);

    habilitar_exec(tripulante);
    pasar_a_block(tripulante);

    sem_wait(tripulante->block_io);

    for (int i = 0; i < tripulante->tarea->duracion; i++){
		sem_wait(tripulante->ciclo_cpu);
		tripulante->quantum += 1;
		if(tripulante->exit){
			return;
		}
	}

    enviar_mensaje_tarea(modulo_imongo, tripulante->tarea);

    string_append_with_format(&respuesta, "Finalizo la tarea: %s\n", tripulante->tarea->nombre_tarea);

    enviar_mensaje_notificar_fileSystem(modulo_imongo, respuesta, tripulante->pid, tripulante->tid);

    free(respuesta);

    desuscribirse_clock(tripulante->ciclo_cpu);

    sem_post(sem_io_libre);

}

bool misma_posicion(t_posicion posicion1, t_posicion posicion2){
    return posicion1.posx == posicion2.posx && posicion1.posy == posicion2.posy;
}

void avanzar_hacia(t_tripulante* tripulante, t_posicion destino){


    if (tripulante->posicion.posx > destino.posx){
        mover_hacia_izquierda(tripulante);
    } else if(tripulante->posicion.posx < destino.posx) {
        mover_hacia_derecha(tripulante);
    } else if (tripulante->posicion.posy > destino.posy) {
        mover_hacia_abajo(tripulante);
    } else {
        mover_hacia_arriba(tripulante);
    }

}

void mover_hacia_izquierda(t_tripulante* tripulante){
	tripulante->posicion.posx -= 1;
}

void mover_hacia_derecha(t_tripulante* tripulante){
	tripulante->posicion.posx += 1;
}

void mover_hacia_abajo(t_tripulante* tripulante){
	tripulante->posicion.posy -= 1;
}

void mover_hacia_arriba(t_tripulante* tripulante){
	tripulante->posicion.posy += 1;
}

void pasar_a_ready(t_tripulante* tripulante){
	sem_wait(sem_ready);
	printf("TRIPULANTE %s%d PASA A READY\n",tripulante->pid, tripulante->tid);
	quitar_de_exec(tripulante);
	tripulante->estado = READY;
	pushbacklist(&pcb_ready, tripulante);
	sem_post(sem_pcb_ready);
	char* param1 = string_itoa(tripulante->tid);
	enviar_mensaje_cambio_estado(modulo_miramhq, tripulante->pid, param1, tripulante->estado);
	sem_post(sem_ready);
}

void pasar_a_block(t_tripulante* tripulante){
	printf("TRIPULANTE %s%d PASA A BLOCKED\n", tripulante->pid, tripulante->tid);
	quitar_de_exec(tripulante);
	tripulante->estado = BLOCK_IO;
	pushbacklist(&pcb_block, tripulante);
	sem_post(sem_pcb_block);
	char* param1 = string_itoa(tripulante->tid);
	enviar_mensaje_cambio_estado(modulo_miramhq, tripulante->pid, param1, tripulante->estado);
}

void habilitar_exec(t_tripulante* tripulante){
	sem_post(sem_grado_multiprocesamiento);
}

void desuscribirse_clock(sem_t* ciclo_cpu){
	sem_wait(sem_desuscribir);
	for (IteratorList it = beginlist(suscriptores_cpu); it != NULL ; it = nextlist(it)){
        if ((sem_t*)dataiterlist(it) == ciclo_cpu){
            popiterlist(&suscriptores_cpu, it);
            break;
        }
    }
	sem_post(sem_desuscribir);
}

int cumplio_quantum(t_tripulante* tripulante){

	if(tripulante->quantum == config_discordiador->quantum){
		tripulante->quantum = 0;
		return 1;
	}
	return 0;

}

void quitar_de_exec(t_tripulante* tripulante){
	for (IteratorList it = beginlist(pcb_exec); it != NULL ; it = nextlist(it)){
		t_tripulante* tripuAux = (t_tripulante*)dataiterlist(it);
		if (tripuAux->pid == tripulante->pid && tripuAux->tid == tripulante->tid){
			popiterlist(&pcb_exec, it);
			break;
		}
	}
}

void quitar_de_block_io(t_tripulante* tripulante){
	for (IteratorList it = beginlist(pcb_block); it != NULL ; it = nextlist(it)){
		t_tripulante* tripuAux = (t_tripulante*)dataiterlist(it);
		if (tripuAux->pid == tripulante->pid && tripuAux->tid == tripulante->tid){
			popiterlist(&pcb_block, it);
			break;
		}
	}
}

void quitar_de_ready(t_tripulante* tripulante){
	for (IteratorList it = beginlist(pcb_ready); it != NULL ; it = nextlist(it)){
		t_tripulante* tripuAux = (t_tripulante*)dataiterlist(it);
		if (tripuAux->pid == tripulante->pid && tripuAux->tid == tripulante->tid){
			popiterlist(&pcb_ready, it);
			break;
		}
	}
}

void quitar_de_new(t_tripulante* tripulante){
	for (IteratorList it = beginlist(pcb_new); it != NULL ; it = nextlist(it)){
		t_tripulante* tripuAux = (t_tripulante*)dataiterlist(it);
		if (tripuAux->pid == tripulante->pid && tripuAux->tid == tripulante->tid){
			popiterlist(&pcb_new, it);
			break;
		}
	}
}

void quitar_de_block_emergency(t_tripulante* tripulante){
	for (IteratorList it = beginlist(tripulantes_emergencia); it != NULL ; it = nextlist(it)){
		t_tripulante* tripuAux = (t_tripulante*)dataiterlist(it);
		if (tripuAux->pid == tripulante->pid && tripuAux->tid == tripulante->tid){
			popiterlist(&tripulantes_emergencia, it);
			break;
		}
	}
}

void free_tripulante(t_tripulante* tripulante){

	free(tripulante->new_a_ready);
	free(tripulante->block_io);
	free(tripulante->ciclo_cpu);
	//free(tripulante->pid);
	//sleep(1);
	//free(tripulante);

}

void pasar_a_exit(t_tripulante* tripulante){

	quitar_de_exec(tripulante);
	tripulante->estado = EXIT;
	char* param1 = string_itoa(tripulante->tid);
	char* string = unir_pid_tid(tripulante->pid, param1);
	char* respuesta = enviar_mensaje_expulsar_tripulante(modulo_miramhq, string);
	free(param1);
	free(respuesta);

}

void pasar_a_expulsar_tripulante(t_tripulante* tripulante){

	switch(tripulante->estado){

		case NEW: quitar_de_new(tripulante);
		sem_post(tripulante->new_a_ready);
		break;
		case READY: quitar_de_ready(tripulante);
		sem_wait(sem_pcb_ready);
		break;
		case EXEC: quitar_de_exec(tripulante);
		desuscribirse_clock(tripulante->ciclo_cpu);
		habilitar_exec(tripulante);
		break;
		case BLOCK_IO: quitar_de_block_io(tripulante);
		desuscribirse_clock(tripulante->ciclo_cpu);
		break;
		case BLOCK_EMERGENCY: quitar_de_block_emergency(tripulante);
		break;
		default: break;
	}

	if(tripulante->tarea != NULL){
		free(tripulante->tarea->nombre_tarea);
		free(tripulante->tarea);
		tripulante->tarea = NULL;
		tripulante->exit = 1;
	}

	while(tripulante->estado != EXIT){
		sem_post(tripulante->ciclo_cpu);
	}

	free_tripulante(tripulante);
}
