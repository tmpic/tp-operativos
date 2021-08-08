#include "../include/planificador.h"

void iniciar_planificador(){

    iniciar_planificador_largo_plazo();

    iniciar_planificador_block();

    iniciar_clock();

    iniciar_planificador_corto_plazo();

}

void iniciar_clock(){
    pthread_t thread;
    pthread_create(&thread,NULL,(void*)clock_cpu, NULL);
	pthread_detach(thread);
}

void clock_cpu(){

	sleep(config_discordiador->retardo_ciclo_cpu);

    while(true){
    	sem_post(sem_planificar);
    	sem_post(sem_desuscribir);

        printf("-----------CLOCK-------------\n");

        for (IteratorList il = beginlist(suscriptores_cpu); il != NULL; il = nextlist(il)){
            sem_t* suscriptor = (sem_t*) dataiterlist(il);
            sem_post(suscriptor);
        }

        sleep(config_discordiador->retardo_ciclo_cpu);

        sem_wait(sem_planificar);
        sem_wait(sem_desuscribir);
    }
}

void iniciar_planificador_largo_plazo(){
    pthread_t thread;
    pthread_create(&thread,NULL,(void*)planificar_largo_plazo, NULL);
	pthread_detach(thread);
}

void planificar_largo_plazo(){
	while(true){
		sem_wait(sem_pcb_new);
		sem_wait(sem_lock_new);
		sem_wait(sem_planificador);

		IteratorList it = NULL;

		List listaAux;
		initlist(&listaAux);

		while(sizelist(pcb_new) != 0){

			t_patota* patota = popfrontlist(&pcb_new);

			char* respuesta = enviar_mensaje_iniciar_patota(modulo_miramhq, patota);

			if(!strcmp(respuesta, "NO")){
				printf("La patota no entra en memoria\n");
				for(IteratorList it = beginlist(*patota->tripulantes); it != NULL; it = nextlist(it)){

					t_tripulante* tripulante = dataiterlist(it);

					free_tripulante(tripulante);

				}
				emptylist(patota->tripulantes);
				liberarPatota(patota);
				free(respuesta);
				continue;
			}

			for(it = beginlist(*patota->tripulantes); it != NULL; it = nextlist(it)){
				t_tripulante* tripulante = dataiterlist(it);

				char* patota = string_new();
				char* aux = &contadorPatota;
				string_append(&patota, aux);

				tripulante->pid = patota;

				tripulante->estado = READY;
				//char* param1 = string_itoa(tripulante->tid);
				//enviar_mensaje_cambio_estado(modulo_miramhq, tripulante->pid, param1, tripulante->estado);
				enviar_mensaje_nuevo_tripulante(modulo_imongo, tripulante->pid, tripulante->tid);

				pthread_t thread;
				pthread_create(&thread,NULL,(void*)ejecutar_tripulante, tripulante);
				pthread_detach(thread);

				pushbacklist(&tripulantes_totales, tripulante);
				pushbacklist(&listaAux, tripulante);
				pushbacklist(&pcb_ready, tripulante);

			}
			contadorPatota += 1;
			free(respuesta);
			liberarPatota(patota);
		}
		while(sizelist(listaAux) != 0){
			t_tripulante* tripulante = popfrontlist(&listaAux);

			sem_post(tripulante->new_a_ready);
			sem_post(sem_pcb_ready);
		}


		sem_post(sem_planificador);
		sem_post(sem_lock_new);
	}

}

void iniciar_planificador_corto_plazo(){
    if (!strcmp(config_discordiador->algoritmo, "FIFO")){
        planificar_corto_plazo_FIFO();
    }
    else if (!strcmp(config_discordiador->algoritmo, "RR")){
    	//pushfrontlist(&suscriptores_cpu, sem_quantum);
        //pthread_t thread;
        //pthread_create(&thread,NULL,(void*)aumentar_quantum, NULL);
	    //pthread_detach(thread);
        planificar_corto_plazo_RR();
    }
}


void planificar_corto_plazo_FIFO(){
    while (true){
    	sem_wait(sem_pcb_ready);
        sem_wait(sem_grado_multiprocesamiento);
        sem_wait(sem_planificador);

        t_tripulante* tripulante = popfrontlist(&pcb_ready);

        printf("TRIPULANTE %s%d PASA A EXEC\n", tripulante->pid,tripulante->tid);

        tripulante->estado = EXEC;
        char* param1 = string_itoa(tripulante->tid);
        enviar_mensaje_cambio_estado(modulo_miramhq, tripulante->pid, param1, tripulante->estado);

        if(tripulante->primeraVez){
			sem_post(tripulante->ready_a_exec);
		}

        pushbacklist(&pcb_exec, tripulante);
        pushbacklist(&suscriptores_cpu, tripulante->ciclo_cpu);

        sem_post(sem_planificador);

    }
}

void iniciar_planificador_block(){
	pthread_t thread;
	pthread_create(&thread,NULL,(void*)planificar_block_io, NULL);
	pthread_detach(thread);
}

void planificar_block_io(){
    while (true){
    	sem_wait(sem_io_libre);
        sem_wait(sem_pcb_block);

        t_tripulante* tripulante = popfrontlist(&pcb_block);

        sem_post(tripulante->block_io);

        pushbacklist(&suscriptores_cpu, tripulante->ciclo_cpu);

    }
}

void imprimirLista(List* lista, char* nombre){

	char string_log[200];
	sprintf(string_log, "-----------IMPRIMIENDO %s------------", nombre);
	log_info(logger, string_log);

	for(IteratorList it = beginlist(*lista); it != NULL; it = nextlist(it)){

		t_tripulante* tripulante = dataiterlist(it);

		char string_log[200];
		sprintf(string_log, "\tTRIP: PID: %s - TID: %d - QUANTUM: %d", tripulante->pid, tripulante->tid, tripulante->quantum);
		log_info(logger, string_log);

	}

}

void planificar_sabotaje(){

	char string_log[200];
	sprintf(string_log, "SE INICIA SABOTAJE");
	log_info(logger, string_log);

	IteratorList it = NULL;

	imprimirLista(&pcb_exec, "EXEC");

	List* listaOrdenadaExec = ordenarListaTid(pcb_exec);

	imprimirLista(listaOrdenadaExec, "EXEC ORDENADO");

	imprimirLista(&pcb_ready, "READY");

	List* listaOrdenadaReady = ordenarListaTid(pcb_ready);

	imprimirLista(listaOrdenadaReady, "READY ORDENADO");

	//emptylist(&pcb_exec);
	initlist(&pcb_exec);
	//emptylist(&pcb_ready);
	initlist(&pcb_ready);

	for(it = beginlist(*listaOrdenadaExec); it != NULL; it = nextlist(it)){
		t_tripulante* tripulante = dataiterlist(it);
		desuscribirse_clock(tripulante->ciclo_cpu);
		habilitar_exec(tripulante);
		tripulante->estado_anterior_sabotaje = tripulante->estado;
		tripulante->estado = BLOCK_EMERGENCY;
		char* param1 = string_itoa(tripulante->tid);
		enviar_mensaje_cambio_estado(modulo_miramhq, tripulante->pid, param1, tripulante->estado);
		pushbacklist(&tripulantes_emergencia, tripulante);
	}
	//emptylist(listaOrdenadaExec);
	free(listaOrdenadaExec);
	for(it = beginlist(*listaOrdenadaReady); it != NULL; it = nextlist(it)){
		t_tripulante* tripulante = dataiterlist(it);
		tripulante->estado_anterior_sabotaje = tripulante->estado;
		tripulante->estado = BLOCK_EMERGENCY;
		char* param1 = string_itoa(tripulante->tid);
		enviar_mensaje_cambio_estado(modulo_miramhq, tripulante->pid, param1, tripulante->estado);
		pushbacklist(&tripulantes_emergencia, tripulante);
	}
	//emptylist(listaOrdenadaReady);
	free(listaOrdenadaReady);

	t_tripulante* tripulante = obtener_tripulante_mas_cercano(sabotaje);

	sabotaje->tripulante = tripulante;

	char* respuesta = "Empezo a atender el sabotaje\n";

	enviar_mensaje_notificar_fileSystem(modulo_imongo, respuesta, sabotaje->tripulante->pid, sabotaje->tripulante->tid);

	while(!misma_posicion(tripulante->posicion, sabotaje->posicion)){
		sleep(config_discordiador->retardo_ciclo_cpu);
		avanzar_hacia(tripulante, sabotaje->posicion);
	}

	pushbacklist(&tripulantes_emergencia, tripulante);

	enviar_mensaje_fsck(modulo_imongo);

	for(int i = 0; i<sabotaje->duracion; i++){
		printf("Tripulante: %s%d resuelve el sabotaje\n", tripulante->pid, tripulante->tid);
		sleep(config_discordiador->retardo_ciclo_cpu);
	}

	respuesta = "Finalizo de atender el sabotaje\n";

	enviar_mensaje_notificar_fileSystem(modulo_imongo, respuesta, sabotaje->tripulante->pid, sabotaje->tripulante->tid);

	imprimirLista(&tripulantes_emergencia, "TRIP EMERGENCIA");
/*
	for(int i = 0; i < config_discordiador->grado_multitarea; i++){
		t_tripulante* tripulante = popfrontlist(&tripulantes_emergencia);
		tripulante->estado = EXEC;
		if(tripulante->estado_anterior_sabotaje != EXEC){
			pushbacklist(&suscriptores_cpu, tripulante->ciclo_cpu);
		}
		char* param1 = string_itoa(tripulante->tid);
		enviar_mensaje_cambio_estado(modulo_miramhq, tripulante->pid, param1, tripulante->estado);
		pushbacklist(&pcb_exec, tripulante);
	}
*/
	//imprimirLista(&pcb_exec, "EXEC TRAS SABOTAJE");

	int auxFor = sizelist(tripulantes_emergencia);

	for(int i = 0; i < auxFor; i++){
		t_tripulante* tripulante = popfrontlist(&tripulantes_emergencia);
		tripulante->estado = READY;
		char* param1 = string_itoa(tripulante->tid);
		enviar_mensaje_cambio_estado(modulo_miramhq, tripulante->pid, param1, tripulante->estado);
		if(tripulante->estado_anterior_sabotaje != READY){
			desuscribirse_clock(tripulante->ciclo_cpu);
			pasar_a_ready(tripulante);
			continue;
		}
		pushbacklist(&pcb_ready, tripulante);
	}

	imprimirLista(&pcb_ready, "READY TRAS SABOTAJE");

	char string_log2[200];
	sprintf(string_log2, "SE FINALIZA SABOTAJE");
	log_info(logger, string_log2);
}

t_tripulante* obtener_tripulante_mas_cercano(t_sabotaje* sabotaje2){

	t_tripulante* tripulanteAux = NULL;
    int calculoAux;
    IteratorList iterAux;
    //LLENAR TAREA CON LA EMERGENCIA Y SU UBICACION


    for(IteratorList iter = beginlist(tripulantes_emergencia); iter != NULL; iter = nextlist(iter)){

        t_tripulante* tripulante = (t_tripulante*) iter->data;

        int posix = abs(tripulante->posicion.posx - sabotaje2->posicion.posx);

        int posiy = abs(tripulante->posicion.posy - sabotaje2->posicion.posy);

        int calculo = sqrt(pow(posix, 2) + pow(posiy, 2));

        if(tripulanteAux == NULL){
        	tripulanteAux = tripulante;
            calculoAux = calculo;
            iterAux = iter;
        }
        if(calculo < calculoAux){
        	tripulanteAux = tripulante;
            calculoAux = calculo;
            iterAux = iter;
        }
        if(nextlist(iter) == NULL){
        	tripulanteAux = popiterlist(&tripulantes_emergencia, iterAux);
        }
    }

    return tripulanteAux;
}

void planificar_corto_plazo_RR(){
    while(1){
        sem_wait(sem_pcb_ready);
        sem_wait(sem_grado_multiprocesamiento);
        sem_wait(sem_planificador);

        t_tripulante* tripulante = popfrontlist(&pcb_ready);

        printf("TRIPULANTE %s%d PASA A EXEC\n", tripulante->pid,tripulante->tid);

        tripulante->estado = EXEC;
        char* param1 = string_itoa(tripulante->tid);
        enviar_mensaje_cambio_estado(modulo_miramhq, tripulante->pid, param1, tripulante->estado);

        if(tripulante->primeraVez){
        	sem_post(tripulante->ready_a_exec);
        }

        pushbacklist(&pcb_exec, tripulante);
        pushbacklist(&suscriptores_cpu, tripulante->ciclo_cpu);

        sem_post(sem_planificador);

    }
}
/*
void aumentar_quantum(){

    while(true){
        sem_wait(sem_quantum);
        for(IteratorList iter = beginlist(pcb_exec); iter != NULL; iter = nextlist(iter)){
            t_tripulante* tripulante = (t_tripulante*) dataiterlist(iter);

            tripulante->quantum += 1;
        }
        sem_post(sem_continuar_clock);    }
}
*/
List* ordenarListaTid(List lista){
	List* listaReturn = malloc(sizeof(List));
	initlist(listaReturn);

	List listaAux;
	initlist(&listaAux);

	IteratorList it = NULL;

	for(it = beginlist(lista); it != NULL; it = nextlist(it)){
		void* data = dataiterlist(it);
		pushbacklist(&listaAux, data);
	}

	int sizelista = sizelist(lista);

	int aux2 = 99;
	int valorPop;
	int sizeValorReturn = 0;

	t_tripulante* tripAux;

	while(sizeValorReturn != sizelista){

		aux2 = 99;

		for(int i = 0; i < sizelista-sizeValorReturn; i++){

			t_tripulante* tripulante = (t_tripulante*) atlist(listaAux, i);
			char auxPid = (char)*tripulante->pid;
			int aux = auxPid;

			if(aux < aux2){
				aux2 = aux;
				tripAux = tripulante;
				valorPop = i;
			} else if (aux == aux2){
				if(tripulante->tid < tripAux->tid){
					tripAux = tripulante;
					valorPop = i;
				}
			}
			if(atlist(listaAux, i+1) == NULL){
				popatlist(&listaAux, valorPop);
			}
		}

	pushbacklist(listaReturn, tripAux);
	sizeValorReturn += 1;

	}

	return listaReturn;
}

void quitarDeTripGlobales(t_tripulante* tripulante){

	for(IteratorList it = beginlist(tripulantes_totales); it != NULL; it = nextlist(it)){

		t_tripulante* tripulanteAux = dataiterlist(it);

		if(!strcmp(tripulante->pid, tripulanteAux->pid) && tripulante->tid == tripulanteAux->tid){
			popiterlist(&tripulantes_totales, it);
			return;
		}

	}

}

void liberarPatota(t_patota* patota){

	free(patota->tripulantes);
	free(patota->posiciones);
	free(patota->tarea);

	free(patota);


}

