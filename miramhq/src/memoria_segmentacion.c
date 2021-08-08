#include "../include/memoria_segmentacion.h"

void eliminar_proceso_lista(List* lista, l_proceso_segmento* proceso){
	IteratorList iterador = NULL;

	for(iterador = beginlist(*lista);iterador!=NULL;iterador = nextlist(iterador)){
		l_proceso_segmento* procesoAux = dataiterlist(iterador);

		if(proceso->idProceso == procesoAux->idProceso){
			popiterlist(lista, iterador);
			break;
		}
	}
}

int esBorrableElProcesoSegmentacion(l_proceso_segmento* proceso){
	List* tablaSegmentos = proceso->punteroTablaSegmentos;
	IteratorList iteradorDeSegmentos = NULL;
	l_segmento* segmento = NULL;

	if(beginlist(*tablaSegmentos)==NULL){
			return -1;
	}

	for(iteradorDeSegmentos = beginlist(*tablaSegmentos);iteradorDeSegmentos!=NULL;iteradorDeSegmentos = nextlist(iteradorDeSegmentos)){
		segmento = dataiterlist(iteradorDeSegmentos);

		if(segmento->tipoSegmento == tipo_tcb){
			return 0;
		}
	}
	return 1;
}

void iniciarMemoriaSegmentacion(){

	l_segmento* segmento = crearSegmento(NULL, libre, NULL, miramhq_config->tamanio_memoria, NULL);

	liberarSegmento(segmento);

}

void liberarSegmento(l_segmento* segmento){
	segmento->ocupado = 0;
}


l_segmento* crearSegmento(l_proceso_segmento* proceso, t_tipo_segmento tipoSegmento, char* idSegmento, int tamanio_segmento, void* estructura){//Cargo pcb o tcb o tareas(indicando length)

	l_segmento *segmento = malloc(sizeof(l_segmento));

	if(proceso != NULL){
		segmento->idSegmento = string_new();
		string_append(&segmento->idSegmento, idSegmento);
		segmento->tipoSegmento = tipoSegmento;
	}
	segmento->tamanio_segmento = tamanio_segmento;

	int movimientoMistico = hueco_en_memoria(tamanio_segmento, &l_particiones);

	if(movimientoMistico == -1){
		free(segmento->idSegmento);
		free(segmento);
		return NULL;
	}

	segmento->byteInicio = movimientoMistico;
	segmento->direccion_inicio = puntero_memoria_principal + segmento->byteInicio;
	segmento->ocupado = 1;

	if(proceso != NULL){
		pushbacklist(proceso->punteroTablaSegmentos, segmento);
	}

	pushearEnPosicion(segmento, &l_particiones);

	if(estructura != NULL){
		cargar_segmento_memoria(proceso, segmento, estructura);
	}

	return segmento;//Hace falta devolver el segmento?

}

l_proceso_segmento *crearProceso(int idProceso){

    l_proceso_segmento* proceso = malloc(sizeof(l_proceso_segmento));
	proceso->idProceso = idProceso;

    List *tablaSegmentos = malloc(sizeof(List));
    initlist(tablaSegmentos);

    proceso->punteroTablaSegmentos = tablaSegmentos;

    return proceso;
}

l_segmento *find_segmento_lista(char* idSegmento, List *segmentos){

    l_segmento *seg = NULL;

    IteratorList iterador = NULL;

    for(iterador = beginlist(*segmentos);iterador!=NULL;iterador = nextlist(iterador)){
        seg = dataiterlist(iterador);

        if(!strcmp(idSegmento, seg->idSegmento)){
            return seg;
        }
    }
    return NULL;

}

l_proceso_segmento *find_proceso_lista(char* idProceso, List *procesos){

    l_proceso_segmento *proceso = NULL;

    IteratorList iterador = NULL;

    for(iterador = beginlist(*procesos);iterador!=NULL;iterador = nextlist(iterador)){
        proceso = dataiterlist(iterador);

        if(atoi(idProceso) == proceso->idProceso){
            return proceso;
        }
    }
    return NULL;

}

void cargar_segmento_memoria(l_proceso_segmento* proceso, l_segmento* segmento, void* structACargarEnMemoria){

	memcpy(segmento->direccion_inicio, structACargarEnMemoria, segmento->tamanio_segmento);//Aca va structACargarEnMemoria

	segmento->ocupado = 1;

}

void* eliminar_segmento_lista(char* idSegmento, List *segmentos){

    l_segmento *seg = NULL;

    IteratorList iterador = NULL;

    for(iterador = beginlist(*segmentos);iterador!=NULL;iterador = nextlist(iterador)){
        seg = dataiterlist(iterador);

        if(!strcmp(idSegmento, seg->idSegmento)){
            return popiterlist(segmentos, iterador);
        }
    }

    return NULL;
}

l_segmento* eliminar_segmento_lista_global(int byteInicio, List* segmentos){

	l_segmento *seg = NULL;

	IteratorList iterador = NULL;

	for(iterador = beginlist(*segmentos);iterador!=NULL;iterador = nextlist(iterador)){
		seg = dataiterlist(iterador);

		if(byteInicio == seg->byteInicio){
			return popiterlist(segmentos, iterador);
		}
	}
	return NULL;
}

void realizarParticion(int byteInicio, int size, List* lista){

	IteratorList iteradorAux = NULL;

	for(iteradorAux = beginlist(*lista);iteradorAux!=NULL;iteradorAux = nextlist(iteradorAux)){
		l_segmento* segmento = dataiterlist(iteradorAux);

		if(segmento->byteInicio == byteInicio){
			if(segmento->tamanio_segmento == size){
				segmento = popiterlist(lista, iteradorAux);
				free(segmento);
				break;
			}
			segmento->tamanio_segmento -= size;
			segmento->byteInicio += size;
		}
	}
}

int hay_hueco(int size, List* lista){
	l_segmento* segmento = NULL;
	IteratorList iteradorDeSegmentos = NULL;
	List* tablaSegmentos = lista;
	int mejorHuecoSize = -1;
	l_segmento* segmentoAux;

	if(beginlist(l_particiones) == NULL){
		return 0;
	}

	for(iteradorDeSegmentos = beginlist(*tablaSegmentos);iteradorDeSegmentos!=NULL;iteradorDeSegmentos = nextlist(iteradorDeSegmentos)){

		segmento = dataiterlist(iteradorDeSegmentos);

		if(segmento->ocupado){
			continue;
		}

		if(strcmp("FF", miramhq_config->criterio_seleccion) == 0){
			if(segmento->tamanio_segmento >= size){
				char string_log[200];
				sprintf(string_log, "Encontre un hueco en la direccion: %d", segmento->byteInicio);
				log_info(logger, string_log);
				mejorHuecoSize = segmento->byteInicio;
				realizarParticion(mejorHuecoSize, size, lista);

				return mejorHuecoSize;

			}
		}else if(strcmp("BF", miramhq_config->criterio_seleccion) == 0){
			int huecoActualSize = segmento->tamanio_segmento;

			if(huecoActualSize >= size){
				if(mejorHuecoSize < 0){
					mejorHuecoSize = segmento->byteInicio;
					segmentoAux = segmento;
				}
				if(huecoActualSize == size){
					mejorHuecoSize = segmento->byteInicio;
					realizarParticion(mejorHuecoSize, size, lista);
					return mejorHuecoSize;
				}
				if(segmentoAux->tamanio_segmento > huecoActualSize){
					mejorHuecoSize = segmento->byteInicio;
					segmentoAux = segmento;
				}
			}
			if(nextlist(iteradorDeSegmentos) == NULL && mejorHuecoSize != -1){
				realizarParticion(mejorHuecoSize, size, lista);
			}

		}
	}

	return mejorHuecoSize;
}

int hueco_en_memoria(int size, List* lista){

	int flagAux = 1;

	int returnValue = hay_hueco(size, lista);

	if(returnValue == -1){
		char string_log[200];
		sprintf(string_log, "No Encontre ningun hueco de %d bytes", size);
		log_info(logger, string_log);

		if(flagAux == 1){
			ejecutarCompactacion();
			flagAux = 0;

			returnValue = hay_hueco(size, lista);

		}
	}

	return returnValue;

}

void pushearEnPosicion(l_segmento* segmentoAsignar, List* lista){

	IteratorList iteradorDeParticiones = NULL;
	IteratorList iteradorAux = NULL;
	unsigned long contador = 0;
	unsigned long aux=0;

	for(iteradorDeParticiones = beginlist(*lista);iteradorDeParticiones!=NULL;iteradorDeParticiones = nextlist(iteradorDeParticiones)){

		l_segmento* segmento = (l_segmento*)dataiterlist(iteradorDeParticiones);

		iteradorAux = nextlist(iteradorDeParticiones);
		l_segmento* segmentoAux = (l_segmento*)dataiterlist(iteradorAux);

		if(segmento->byteInicio > segmentoAsignar->byteInicio){
			segmentoAux = segmento;
			aux = contador ;
			break;
		}

		if(segmentoAux == NULL){
			aux = contador + 1;
			break;
		}

		contador++;
	}

	pushatlist(lista, aux, segmentoAsignar);

}

void algoritmoBoliviano(){

	IteratorList iteradorAux = NULL;

	int datoImportanteTarea;
	int datoImportantePcb;

	for(iteradorAux = beginlist(tablaProcesos);iteradorAux!=NULL;iteradorAux = nextlist(iteradorAux)){
		l_proceso_segmento* proceso = dataiterlist(iteradorAux);

		l_segmento* segmentoAux = backlist(*proceso->punteroTablaSegmentos);

		datoImportanteTarea = segmentoAux->byteInicio;

		l_segmento* segmento = frontlist(*proceso->punteroTablaSegmentos);

		datoImportantePcb = segmento->byteInicio;

		t_pcb* pcb = segmento->direccion_inicio;

		pcb->direccionInicioTareas = datoImportanteTarea;

		for(int i = sizelist(*proceso->punteroTablaSegmentos)-2; i > 0; i--){

			l_segmento* segmento = atlist(*proceso->punteroTablaSegmentos, i);

			t_tcb* tcb = segmento->direccion_inicio;

			tcb->puntero_PCB = datoImportantePcb;

		}

	}

}

void juntarParticionesOcupadas(List* lista){

	IteratorList iteradorDeParticiones = NULL;
	IteratorList iteradorAux = NULL;
	int distancia;

	for(iteradorDeParticiones = beginlist(*lista);iteradorDeParticiones!=NULL;iteradorDeParticiones = nextlist(iteradorDeParticiones)){

		l_segmento* segmento = (l_segmento*)dataiterlist(iteradorDeParticiones);

		iteradorAux = nextlist(iteradorDeParticiones);

		if(iteradorAux == NULL){
			break;
		}

		l_segmento* segmentoSiguiente = (l_segmento*)dataiterlist(iteradorAux);

		distancia = segmentoSiguiente->byteInicio - (segmento->byteInicio + segmento->tamanio_segmento);

		if(distancia > 0){
			segmentoSiguiente->byteInicio = segmento->byteInicio + segmento->tamanio_segmento;
			void* aux = malloc(segmentoSiguiente->tamanio_segmento);
			memcpy(aux, segmentoSiguiente->direccion_inicio, segmentoSiguiente->tamanio_segmento);
			memcpy(puntero_memoria_principal + segmentoSiguiente->byteInicio, aux, segmentoSiguiente->tamanio_segmento);
			segmentoSiguiente->direccion_inicio = puntero_memoria_principal + segmentoSiguiente->byteInicio;
			free(aux);

		}
	}

	algoritmoBoliviano();

}

int ejecutarCompactacion(){

	IteratorList iteradorDeParticiones = NULL;
	IteratorList iteradorDeParticionesAux = malloc(sizeof(ListNode));
	int libre = 0;

	for(iteradorDeParticiones = beginlist(l_particiones);iteradorDeParticiones!=NULL;iteradorDeParticiones = nextlist(iteradorDeParticionesAux)){

		l_segmento* segmento = (l_segmento*)dataiterlist(iteradorDeParticiones);
		memcpy(iteradorDeParticionesAux, iteradorDeParticiones, sizeof(ListNode));

		if(!segmento->ocupado){
			l_segmento* segmentoAux = popiterlist(&l_particiones, iteradorDeParticiones);
			free(segmentoAux);
			libre++;
		}
	}

	free(iteradorDeParticionesAux);

	if(libre == 0){
		return 0;
	}

	juntarParticionesOcupadas(&l_particiones);

	l_segmento* segmento = backlist(l_particiones);

	if(segmento == NULL){
		l_segmento *segmentoAux = malloc(sizeof(l_segmento));

		segmentoAux->tamanio_segmento = miramhq_config->tamanio_memoria;
		segmentoAux->byteInicio = 0;
		segmentoAux->direccion_inicio = puntero_memoria_principal;
		segmentoAux->ocupado = 0;

		pushearEnPosicion(segmentoAux, &l_particiones);

	}else {
		l_segmento *segmentoAux = malloc(sizeof(l_segmento));
		segmentoAux->tamanio_segmento = miramhq_config->tamanio_memoria - (segmento->byteInicio + segmento->tamanio_segmento);

		int movimientoMistico = segmento->byteInicio + segmento->tamanio_segmento;

		segmentoAux->byteInicio = movimientoMistico;
		segmentoAux->direccion_inicio = puntero_memoria_principal + segmento->byteInicio;
		segmentoAux->ocupado = 0;

		pushearEnPosicion(segmentoAux, &l_particiones);
	}
	return 0;
}

l_proceso_segmento* buscarSegmentoEntreProcesos(int byteInicio){

	IteratorList iterador = NULL;
	IteratorList jterador = NULL;

	for(iterador = beginlist(tablaProcesos);iterador!=NULL;iterador = nextlist(iterador)){
		l_proceso_segmento* proceso = dataiterlist(iterador);

		for(jterador = beginlist(*proceso->punteroTablaSegmentos);jterador!=NULL;jterador = nextlist(jterador)){

			l_segmento* segmento = dataiterlist(jterador);

			if(segmento->byteInicio == byteInicio){
				return proceso;
			}

		}
	}

	return NULL;

}

void imprimirTodoSegmentacionArchivo(FILE* archivo){

	sem_wait(sem);

	l_segmento* segmento = NULL;

	IteratorList iterador = NULL;

	for(iterador = beginlist(l_particiones);iterador!=NULL;iterador = nextlist(iterador)){
		segmento = dataiterlist(iterador);

		if(!segmento->ocupado){
			char* linea = string_from_format("   LIBRE   |    LIBRE    | Inicio: %p | Tam: %db | ByteInicio: %d\n", segmento->direccion_inicio, segmento->tamanio_segmento, segmento->byteInicio);
			txt_write_in_file(archivo, linea);
			free(linea);
			continue;
		}

		l_proceso_segmento* proceso = buscarSegmentoEntreProcesos(segmento->byteInicio);

		char* linea = string_from_format("Proceso: %d | Segmento: %s | Inicio: %p | Tam: %db | ByteInicio: %d\n", proceso->idProceso, segmento->idSegmento, segmento->direccion_inicio, segmento->tamanio_segmento, segmento->byteInicio);
		txt_write_in_file(archivo, linea);
		free(linea);

	}

	sem_post(sem);
}

void imprimirTodoSegmentacion(){

	sem_wait(sem);
	l_segmento* segmento = NULL;

	IteratorList iterador = NULL;
	//IteratorList jterador = NULL;

	char string_log[1000];
	sprintf(string_log, "----------------------------------------------------------------------------------------------\n");
	log_info(logger, string_log);


	char hora[1000];
	char* temporal = temporal_get_string_time("%d/%m/%y %H:%M:%S");
	strncpy(hora, temporal, strlen(temporal)+1);

	sprintf(string_log, "Dump de la memoria: ");

	log_info(logger, string_log);
	log_info(logger, hora);

	for(iterador = beginlist(l_particiones);iterador!=NULL;iterador = nextlist(iterador)){
		segmento = dataiterlist(iterador);

		if(!segmento->ocupado){
			char string_log[200];
			sprintf(string_log, "   LIBRE   |    LIBRE    | Inicio: %p | Tam: %db | ByteInicio: %d", segmento->direccion_inicio, segmento->tamanio_segmento, segmento->byteInicio);
			log_info(logger, string_log);
			continue;
		}

		l_proceso_segmento* proceso = buscarSegmentoEntreProcesos(segmento->byteInicio);

		char string_log[200];
		sprintf(string_log, "Proceso: %d | Segmento: %s | Inicio: %p | Tam: %db | ByteInicio: %d", proceso->idProceso, segmento->idSegmento, segmento->direccion_inicio, segmento->tamanio_segmento, segmento->byteInicio);
		log_info(logger, string_log);

	}

	sprintf(string_log, "----------------------------------------------------------------------------------------------\n");
	log_info(logger, string_log);

	free(temporal);
	sem_post(sem);
}

char* concat_two_strings(char* string1, char* string2) {
	char* new_string = string_new();
	string_append(&new_string, string1);
	string_append(&new_string, string2);
	return new_string;
}

void borrarLista(List* listaAEliminar){
    void* punteroMagico = NULL;
    IteratorList iterador;

    for(iterador = beginlist(*listaAEliminar);iterador!=NULL;iterador = nextlist(iterador)){
        punteroMagico = dataiterlist(iterador);
        free(punteroMagico);
    }

}




