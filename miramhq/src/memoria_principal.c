#include "../include/memoria_principal.h"

void iniciarMemoria(){
	if(!strcmp(miramhq_config->esquema_memoria, "PAGINACION")){
		iniciarMemoriaPaginacion();
		iniciarMemoriaSwap();
	} else if(!strcmp(miramhq_config->esquema_memoria, "SEGMENTACION")){
		iniciarMemoriaSegmentacion();
	}
}

void iniciarMemoriaPaginacion(){

	void* puntero = NULL;
	numeroPaginaGlobal = 0;
	inicioClock = 1;

	puntero = puntero_memoria_principal;

	for(double i=0;  tamanioBitMapPrincipal> i; i++){

		bitarray_clean_bit(bitMap, i);

	}

    while(puntero + tamanioMemoria > puntero_memoria_principal){

		pushbacklist(&tablaFrames, puntero_memoria_principal);

		puntero_memoria_principal += miramhq_config->tamanio_pagina;

	}

	puntero_memoria_principal = puntero;

}

l_pagina* crear_pagina(l_proceso* proceso){
    l_pagina *pagina = malloc(sizeof(l_pagina));

	sem_wait(sem_mutex_num_pagina);
	pagina->numPagina = numeroPaginaGlobal;
	numeroPaginaGlobal += 1;
	sem_post(sem_mutex_num_pagina);

	pagina->bitUso = 0;
    pagina->bitPresencia = 0;
	pagina->bitModificado = 0;

	pushbacklist(proceso->punteroTablaPaginas, pagina);

	return pagina;

}

void cargar_bytes_pagina(void* bytesACargar, int tamanio_bytes, l_pagina* pagina){

	pagina->swap = NULL;
	pagina->frame = NULL;

	modificarPagina(pagina);

	memcpy(pagina->frame, bytesACargar, tamanio_bytes);
/*
	if(pagina->frame == NULL){
		pagina->swap = frameLibreSwap();
		if(pagina->swap == NULL){
			exit(-1);
		}
		memcpy(pagina->swap, bytesACargar, tamanio_bytes);
		return;
	}
	memcpy(pagina->frame, bytesACargar, tamanio_bytes);
	pagina->bitPresencia = 1;
	pagina->bitUso = 1;
	ocuparFrame(pagina->frame, bitMap, tablaFrames);
	pushbacklist(&pilaPaginasAlgoritmos, pagina);*/

}

l_proceso* crearProcesoPaginacion(int idProceso){

	l_proceso* proceso = malloc(sizeof(l_proceso));
	proceso->idProceso = idProceso;

	List *tablaPaginas = malloc(sizeof(List));
	initlist(tablaPaginas);

	proceso->punteroTablaPaginas = tablaPaginas;

	pushbacklist(&tablaProcesos, proceso);

	return proceso;

}

void pasarPaginasAPrincipal(l_proceso *proceso){

	l_pagina *pagina = NULL;
	IteratorList iterador = NULL;

	if(isemptylist(*proceso->punteroTablaPaginas)){
		return;
	}

	for(iterador = beginlist(*proceso->punteroTablaPaginas);iterador!=NULL;iterador = nextlist(iterador)){

		pagina = dataiterlist(iterador);

		modificarPagina(pagina);

	}

}

void *frameLibre(){

    for(int i=0; tamanioBitMapPrincipal > i; i++){

		int bitOcupado = bitarray_test_bit(bitMap, i);
		void *punteroFrame = atlist(tablaFrames, i);

		if(!bitOcupado){
			//printf("PUNTERO FRAMELIBRE: %p\n", punteroFrame);
			return punteroFrame;
		}

	}

	return NULL;

}

void ocuparFrame(void* frame, t_bitarray *bitMapp, List lista){
	sem_wait(sem_ocupar_frame);
	if(bitMapp == bitMapSwap){
		for(int i=0; tamanioBitMapSwap > i; i++){

			void *punteroFrame = atlist(lista, i);

			if(punteroFrame == frame){
				bitarray_set_bit(bitMapSwap, i);
				sem_post(sem_ocupar_frame);
				return;
			}

		}
	} else {
		for(int i=0; tamanioBitMapPrincipal > i; i++){

			void *punteroFrame = atlist(lista, i);

			if(punteroFrame == frame){
				bitarray_set_bit(bitMap, i);
				sem_post(sem_ocupar_frame);
				return;
			}

		}
	}
}

void desocuparFrame(void* frame, t_bitarray *bitMapp, List lista){
	sem_wait(sem_ocupar_frame);
	if(bitMapp == bitMapSwap){
		for(int i=0; tamanioBitMapSwap > i; i++){

			void *punteroFrame = atlist(lista, i);

			if(punteroFrame == frame){
				bitarray_clean_bit(bitMapSwap, i);
				sem_post(sem_ocupar_frame);
				return;
			}

		}
	} else {
		for(int i=0; tamanioBitMapPrincipal > i; i++){

			void *punteroFrame = atlist(lista, i);

			if(punteroFrame == frame){
				bitarray_clean_bit(bitMap, i);
				sem_post(sem_ocupar_frame);
				return;
			}

		}
	}
}

void *frameLibreSwap(){
	sem_wait(sem_mutex_swap_libre);
	for(int i=0; i < tamanioBitMapSwap; i++){

		int bitOcupado = bitarray_test_bit(bitMapSwap, i);
		void *punteroSwap = atlist(tablaSwap, i);

		if(!bitOcupado){
			bitarray_set_bit(bitMapSwap, i);
			sem_post(sem_mutex_swap_libre);
			return punteroSwap;
		}

	}

	printf("\n\n\nSWAP LLENO\n\n\n");
	return NULL;
}

void modificarPagina(l_pagina* paginaSwap){

	sem_wait(sem2);
	int resultadoDePagAPrincipal = pasarAPrincipal(paginaSwap);
	if(!resultadoDePagAPrincipal){
		exit(-3);
	}

	paginaSwap->bitUso = 1;

	quitarSiExiste(paginaSwap);

	pushbacklist(&pilaPaginasAlgoritmos, paginaSwap);

	sem_post(sem2);

}

void quitarSiExiste(l_pagina* paginaSwap){

	l_pagina* pagina = NULL;
	IteratorList iterador = NULL;

	for(iterador = beginlist(pilaPaginasAlgoritmos);iterador!=NULL;iterador = nextlist(iterador)){

		pagina = dataiterlist(iterador);

		if(paginaSwap->bitPresencia){
			if(paginaSwap->frame == pagina->frame){
				popiterlist(&pilaPaginasAlgoritmos, iterador);
				return;
			}
		} else {
			if(paginaSwap->swap == pagina->swap){
				popiterlist(&pilaPaginasAlgoritmos, iterador);
				return;
			}
		}

	}

}

int pasarAPrincipal(l_pagina* paginaSwap){

	if(paginaSwap->bitPresencia == 1){
		return 1;
	}
	sem_wait(sem_mutex_algoritmos);
	void* espacioLibre = ejecutarAlgoritmo();

	ocuparFrame(espacioLibre, bitMap, tablaFrames);
	paginaSwap->frame = espacioLibre;
	if(paginaSwap->swap != NULL){
		memcpy(espacioLibre, paginaSwap->swap, miramhq_config->tamanio_pagina);
		desocuparFrame(paginaSwap->swap, bitMapSwap, tablaSwap);
		paginaSwap->swap = NULL;
	}
	paginaSwap->bitPresencia = 1;
	imprimirDumpMemoriaPaginacion();
	imprimirDumpMemoriaPaginacion2();
	sem_post(sem_mutex_algoritmos);
	return 1;

}

void *ejecutarAlgoritmo(){

	void* framelibre = frameLibre();

	if(framelibre != NULL)
		return framelibre;

	char string_log[100];
    sprintf(string_log, "Ejecutando algoritmo: %s", algoritmo);
    log_info(logger, string_log);

	if(!strcmp(algoritmo, "LRU")){
		void* direccion = ejecutarLRU();
		return direccion;
	}
	if(!strcmp(algoritmo, "CLOCK")){
		void* direccion = ejecutarClock();
		return direccion;
	}

	printf("ALGORITMO INVALIDO\n");

	return NULL;
}

void* ejecutarLRU(){
	l_pagina* pagina = popfrontlist(&pilaPaginasAlgoritmos);
	//l_frame* frame = pagina->frame;
	char string_log[200];
    sprintf(string_log, "Victima seleccionada: %p | Direccion principal: %p | Direccion virtual: %p ", pagina->frame, pagina->frame, pagina->swap);
    log_info(logger, string_log);
	return desalojarDePrincipal(pagina);

}

void* ejecutarClock(){

	l_pagina* pagina = NULL;
	IteratorList iterador = NULL;

	if(inicioClock == 1){
		iteradorClock = beginlist(pilaPaginasAlgoritmos);
		inicioClock = 0;
	}

	iterador = iteradorClock;

	while(iterador != NULL){

		pagina = (l_pagina*)dataiterlist(iterador);

		if(!pagina->bitUso){
			if(nextlist(iterador) == NULL){
				iteradorClock = beginlist(pilaPaginasAlgoritmos);
			} else iteradorClock = nextlist(iterador);
			popiterlist(&pilaPaginasAlgoritmos, iterador);
			char string_log[200];
			sprintf(string_log, "Victima Seleccionada: Direccion principal: %p | Direccion virtual: %p ", pagina->frame, pagina->swap);
			log_info(logger, string_log);
			return desalojarDePrincipal(pagina);
		} else {
			pagina->bitUso = 0;
		}


		iterador = nextlist(iterador);

		if(iterador == NULL){
			iterador = beginlist(pilaPaginasAlgoritmos);
		}

	}
	printf("\nError ejecutando el algoritmo!!");
	return NULL;
}

void *desalojarDePrincipal(l_pagina* pagina){

	void* direccionPagina = NULL;

	if(pagina->bitPresencia != 1){
		exit(-2);
	}

	pagina->bitPresencia = 0;
	direccionPagina = pagina->frame;
	pagina->swap = frameLibreSwap();
	ocuparFrame(pagina->swap, bitMapSwap, tablaSwap);
	memcpy(pagina->swap, pagina->frame, miramhq_config->tamanio_pagina);
	pagina->frame = NULL;
	desocuparFrame(direccionPagina, bitMap, tablaFrames);

	return direccionPagina;

}

void imprimirMemoriaPaginacion(){
	sem_wait(sem);
	l_proceso* proceso = NULL;
	obtenerLoQueSea_result* resultReturn;

	//l_frame* frame = NULL;

	IteratorList iterador = NULL;

	char string_log[200];
    sprintf(string_log, "----------------------------------------------------------------------------------------------");
    log_info(logger, string_log);

    for(iterador = beginlist(tablaProcesos);iterador!=NULL;iterador = nextlist(iterador)){
        proceso = dataiterlist(iterador);

		char string_log[200];
    	sprintf(string_log, "ID proceso: %d", proceso->idProceso);
    	log_info(logger, string_log);

    	resultReturn = obtenerLoQueSea(0, sizeof(t_pcb), proceso);

    	void* direccionPCB = resultReturn->punteroALoRequisado;
    	t_pcb* pcb = (t_pcb*) direccionPCB;

    	char string_log2[200];
		sprintf(string_log2, "\t-----TAREAS-----");
		log_info(logger, string_log2);

		char* tareas = obtenerTareas(proceso);

		char string_log3[200];
		sprintf(string_log3, "\tTAREA: %s", tareas);
		log_info(logger, string_log3);

		free(tareas);

    	char string_log4[300];
		sprintf(string_log4, "\t-----PCB-------");
		log_info(logger, string_log4);

    	char string_log5[300];
		sprintf(string_log5, "\tID: %d | puntero_tarea: %d", pcb->idPatota, pcb->direccionInicioTareas);
		log_info(logger, string_log5);

    	free(direccionPCB);
    	free(resultReturn);

		char string_log6[300];
		sprintf(string_log6, "\t-----TCB-------");
		log_info(logger, string_log6);


    	for(int i = 0; proceso->cantTripulantes > i; i++){
    		//int sumaTCB = i*sizeof(t_tcb);
    		resultReturn = obtenerLoQueSea(sizeof(t_pcb)+(i*(sizeof(t_tcb))), sizeof(t_tcb), proceso);

    		if(resultReturn != NULL){
    			void* direccionTCB = resultReturn->punteroALoRequisado;

				t_tcb* tcb = (t_tcb*)direccionTCB;

				char string_log7[1000];
				sprintf(string_log7, "\tID: %d | Estado: %c | pos_x: %d | pos_y: %d | proxima_instruccion_a_ejecutar: %d | puntero_PCB: %d", tcb->idTripulante, tcb->estadoTripulante, tcb->pos_x, tcb->pos_y, tcb->proxima_instruccion_a_ejecutar, tcb->puntero_PCB);
				log_info(logger, string_log7);

				free(tcb);
				free(resultReturn);
    		}

    	}

	}

	log_info(logger, string_log);
	sem_post(sem);
}

obtenerLoQueSea_result* obtenerLoQueSea(int paraSumar, int paraBuscar, l_proceso* proceso){

	sem_wait(sem_mutex_obtener_tareas);

	obtenerLoQueSea_result* resultReturn = malloc(sizeof(obtenerLoQueSea_result));

	int auxiliarDivisionTarea = (sizelist(*proceso->punteroTablaPaginas) - (proceso->tamanioTareas / miramhq_config->tamanio_pagina)) -1;

	int divisionTarea = auxiliarDivisionTarea;
	int restoTarea = proceso->tamanioTareas % miramhq_config->tamanio_pagina;

	int auxiliarRestoTarea = (miramhq_config->tamanio_pagina - restoTarea);

	int paginas = 0;

	l_pagina* pagina = NULL;

	void* voidReturn = malloc(paraBuscar);

	IteratorList jterador = NULL;

	int cuentaWhile = paraSumar;

	while(cuentaWhile >= miramhq_config->tamanio_pagina){
		cuentaWhile -= miramhq_config->tamanio_pagina;
		paginas += 1;
	}
//()(TAREA-PCB-TCB1)
	for(jterador = beginlist(*proceso->punteroTablaPaginas);jterador!=NULL;jterador = nextlist(jterador)){
		pagina = dataiterlist(jterador);
		l_pagina* paginaSiguiente = dataiterlist(nextlist(jterador));

		int cuentaAcaArriba = miramhq_config->tamanio_pagina - cuentaWhile;

		if(auxiliarRestoTarea < paraSumar && paginaSiguiente->numPagina == divisionTarea-1){
			free(resultReturn);
			free(voidReturn);
			sem_post(sem_mutex_obtener_tareas);
			return NULL;
		}

		if(pagina->numPagina == paginas && cuentaAcaArriba < paraBuscar){
			if(paginaSiguiente->numPagina >= divisionTarea || paginaSiguiente->numPagina != pagina->numPagina +1){
				free(resultReturn);
				free(voidReturn);
				sem_post(sem_mutex_obtener_tareas);
				return NULL;
			}
			if(pagina->bitPresencia == 1){
				memcpy(voidReturn, pagina->frame + cuentaWhile, cuentaAcaArriba);
			} else {
				memcpy(voidReturn, pagina->swap + cuentaWhile, cuentaAcaArriba);
			}
			if(paginaSiguiente->bitPresencia == 1){
				memcpy(voidReturn + cuentaAcaArriba, paginaSiguiente->frame, paraBuscar - cuentaAcaArriba);
			} else {
				memcpy(voidReturn + cuentaAcaArriba, paginaSiguiente->swap, paraBuscar - cuentaAcaArriba);
			}

			resultReturn->paginaUtilizada = pagina->numPagina;
			resultReturn->paginaUtilizada2 = paginaSiguiente->numPagina;
			resultReturn->punteroALoRequisado = voidReturn;
			sem_post(sem_mutex_obtener_tareas);
			return resultReturn;
		} else if (pagina->numPagina == paginas && cuentaAcaArriba >= paraBuscar){
			if(pagina->bitPresencia == 1){
				memcpy(voidReturn, pagina->frame + cuentaWhile, paraBuscar);
			} else {
				memcpy(voidReturn, pagina->swap + cuentaWhile, paraBuscar);
			}
			resultReturn->paginaUtilizada = pagina->numPagina;
			resultReturn->paginaUtilizada2 = -1;
			resultReturn->punteroALoRequisado = voidReturn;
			sem_post(sem_mutex_obtener_tareas);
			return resultReturn;
		}

	}
	sem_post(sem_mutex_obtener_tareas);
	return NULL;

}

void modificarTripulanteDeLaEstructura(int numeroTripulante, l_proceso* proceso, int byteAModificar, void* valor, int pesoAAgregar){

	int divisionTarea = 0;

	l_pagina* pagina = NULL;

	IteratorList jterador = NULL;

	void* i = malloc(pesoAAgregar);
	memcpy(i, valor, pesoAAgregar);

	int cuentaWhile = sizeof(t_pcb) + ((numeroTripulante) * sizeof(t_tcb)) + byteAModificar;

	while(cuentaWhile >= miramhq_config->tamanio_pagina){
		cuentaWhile -= miramhq_config->tamanio_pagina;
		divisionTarea += 1;
	}

	for(jterador = beginlist(*proceso->punteroTablaPaginas);jterador!=NULL;jterador = nextlist(jterador)){
		pagina = dataiterlist(jterador);
		l_pagina* paginaSiguiente = dataiterlist(nextlist(jterador));

		int cuentaAcaArriba = miramhq_config->tamanio_pagina - cuentaWhile;

		if(pagina->numPagina == divisionTarea && cuentaAcaArriba < pesoAAgregar){
			modificarPagina(pagina);
			memcpy(pagina->frame + cuentaWhile, i, cuentaAcaArriba);
			modificarPagina(paginaSiguiente);
			memcpy(paginaSiguiente->frame, i+cuentaAcaArriba, pesoAAgregar - cuentaAcaArriba);
			break;
		} else if (pagina->numPagina == divisionTarea && cuentaAcaArriba >= pesoAAgregar){
			modificarPagina(pagina);
			memcpy(pagina->frame + cuentaWhile, i, pesoAAgregar);
			break;

		}

	}

	free(i);

}

void eliminarTripulante(int numeroTripulante, l_proceso* proceso){

	int calculoAuxiliar = sizeof(t_pcb)+((numeroTripulante)*sizeof(t_tcb));

	obtenerLoQueSea_result* resultReturn = obtenerLoQueSea(calculoAuxiliar, sizeof(t_tcb), proceso);

	if(resultReturn == NULL){
		return;
	}

	uint32_t algo = -1;

	void* acaArriba = malloc(sizeof(uint32_t));
	memcpy(acaArriba, &algo, sizeof(uint32_t));

	switch(estanLasPaginasAlrededorVacias(resultReturn, proceso)){

		case 0:
			modificarTripulanteDeLaEstructura(numeroTripulante, proceso, 0, acaArriba, sizeof(uint32_t));
			break;
		case 1:
			liberarPagina(resultReturn->paginaUtilizada, proceso);
			break;
		case 2:
			liberarPagina(resultReturn->paginaUtilizada2, proceso);
			break;
		case 3:
			liberarPagina(resultReturn->paginaUtilizada, proceso);
			liberarPagina(resultReturn->paginaUtilizada2, proceso);
			break;
	}

	if(esBorrableElProcesoPaginacion(proceso)){
		liberarPatota(proceso);
	}

	free(resultReturn->punteroALoRequisado);
	free(resultReturn);
	free(acaArriba);
}

void desalojarDeEstructuraAdministrativa(int numeroPagina, l_proceso* proceso){

	l_pagina* pagina = NULL;

	IteratorList iterador = NULL;

	for(iterador = beginlist(*proceso->punteroTablaPaginas);iterador!=NULL;iterador = nextlist(iterador)){
		pagina = dataiterlist(iterador);

		if(pagina->numPagina == numeroPagina){
			popiterlist(proceso->punteroTablaPaginas, iterador);
			break;
		}
	}

}

void liberarPagina(int numeroPagina, l_proceso* proceso){

	sem_wait(sem2);
	IteratorList jterador = NULL;

	for(jterador = beginlist(*proceso->punteroTablaPaginas);jterador!=NULL;jterador = nextlist(jterador)){
		l_pagina* pagina = (l_pagina*)dataiterlist(jterador);

		if(pagina->numPagina == numeroPagina){
			quitarSiExiste(pagina);
			if(pagina->bitPresencia == 1){
				pagina->bitPresencia = 0;
				desocuparFrame(pagina->frame, bitMap, tablaFrames);
				pagina->frame = NULL;
			} else {
				desocuparFrame(pagina->swap, bitMapSwap, tablaSwap);
			}

			desalojarDeEstructuraAdministrativa(numeroPagina, proceso);

			popiterlist(proceso->punteroTablaPaginas, jterador);

			free(pagina);

			sem_post(sem2);

			return;
		}

	}
	sem_post(sem2);

}

void liberarPatota(l_proceso* proceso){

	int sizelistAux = sizelist(*proceso->punteroTablaPaginas);

	for(int i=0;i<sizelistAux;i++){
		liberarPagina(i, proceso);
	}
	//emptylist(proceso->punteroTablaPaginas);
	free(proceso->punteroTablaPaginas);

	IteratorList jterador = NULL;

	for(jterador = beginlist(tablaProcesos);jterador!=NULL;jterador = nextlist(jterador)){
		l_proceso* procesoAux = (l_proceso*)dataiterlist(jterador);

		if(proceso->idProceso == procesoAux->idProceso){

			popiterlist(&tablaProcesos, jterador);

			free(proceso);

			return;
		}

	}


}

int estanLasPaginasAlrededorVacias(obtenerLoQueSea_result* resultReturn, l_proceso* proceso){

	if(estaPcbEnPagina(resultReturn, proceso)){
		if(resultReturn->paginaUtilizada2 == -1){
			return 0;
		} else {
			return chequearPaginasAlrededor(resultReturn, proceso);
		}
	} else {
		return chequearPaginasAlrededor(resultReturn, proceso);
	}

}

int estaPcbEnPagina(obtenerLoQueSea_result* resultReturn, l_proceso* proceso){

	obtenerLoQueSea_result* resultPcb = obtenerLoQueSea(0, sizeof(t_pcb), proceso);

	if(resultPcb->paginaUtilizada == resultReturn->paginaUtilizada || resultPcb->paginaUtilizada2 == resultReturn->paginaUtilizada){
		free(resultPcb->punteroALoRequisado);
		free(resultPcb);
		return 1;
	}
	free(resultPcb->punteroALoRequisado);
	free(resultPcb);
	return 0;

}

int chequearPaginasAlrededor(obtenerLoQueSea_result* numeroPagina, l_proceso* proceso){

	int valorReturn = 0;

	List* lista_paginas_adelante;

	List* lista_paginas_atras;

	IteratorList jterador = NULL;

	int* i = malloc(sizeof(uint32_t));

	lista_paginas_adelante = obtenerTcbsAdelante(numeroPagina, proceso);

	if(isemptylist(*lista_paginas_adelante) && numeroPagina->paginaUtilizada2 != -1){
		valorReturn += 2;
	}

	for(jterador = beginlist(*lista_paginas_adelante);jterador!=NULL;jterador = nextlist(jterador)){

		t_tcb* tcb = dataiterlist(jterador);

		memcpy(i, tcb, sizeof(uint32_t));

		if(*i != -1){
			free(tcb);
			break;
		}
		if(nextlist(jterador) == NULL){
			valorReturn += 2;
		}

		free(tcb);


	}

	if(estaPcbEnPagina(numeroPagina, proceso)){
		free(lista_paginas_adelante);
		free(i);
		return valorReturn;
	}

	lista_paginas_atras = obtenerTcbsAtras(numeroPagina, proceso);

	if(isemptylist(*lista_paginas_atras)){
		valorReturn += 1;
	}

	for(jterador = beginlist(*lista_paginas_atras);jterador!=NULL;jterador = nextlist(jterador)){

		t_tcb* tcb = dataiterlist(jterador);

		memcpy(i, tcb, sizeof(uint32_t));

		if(*i != -1){
			free(tcb);
			break;
		}
		if(nextlist(jterador) == NULL){
			valorReturn += 1;
		}

		free(tcb);

	}

	free(lista_paginas_adelante);
	free(lista_paginas_atras);
	free(i);

	return valorReturn;

}

int esBorrableElProcesoPaginacion(l_proceso* proceso){

	for(int i = 0; proceso->cantTripulantes > i; i++){

		obtenerLoQueSea_result* result = obtenerLoQueSea(sizeof(t_pcb)+i*sizeof(t_tcb), sizeof(t_tcb), proceso);

		if(result != NULL){
			int* comparadorNum = malloc(sizeof(uint32_t));
			memcpy(comparadorNum, result->punteroALoRequisado, sizeof(uint32_t));

			if(*comparadorNum != -1){
				free(comparadorNum);
				free(result->punteroALoRequisado);
				free(result);
				return 0;
			}

			free(comparadorNum);

			free(result->punteroALoRequisado);
			free(result);
		}

	}
	return 1;
}

List* obtenerTcbsAdelante(obtenerLoQueSea_result* numeroPagina, l_proceso* proceso){

	//IteratorList jterador = NULL;

	List* listaReturn = malloc(sizeof(List));
	initlist(listaReturn);

	int* numTcbABuscar = malloc(sizeof(uint32_t));
	memcpy(numTcbABuscar, numeroPagina->punteroALoRequisado, sizeof(uint32_t));

	for(int i = 0; proceso->cantTripulantes > i; i++){

		obtenerLoQueSea_result* result = obtenerLoQueSea(sizeof(t_pcb)+i*sizeof(t_tcb), sizeof(t_tcb), proceso);

		if(result!=NULL){

			int* comparadorNum = malloc(sizeof(uint32_t));
			memcpy(comparadorNum, result->punteroALoRequisado, sizeof(uint32_t));

			t_tcb* tcb = malloc(sizeof(t_tcb));
			memcpy(tcb, result->punteroALoRequisado, sizeof(t_tcb));

			if(*comparadorNum != *numTcbABuscar){
				if (result->paginaUtilizada == numeroPagina->paginaUtilizada2 || result->paginaUtilizada2 == numeroPagina->paginaUtilizada2){
					pushbacklist(listaReturn, tcb);
				}
			}

			free(comparadorNum);

			free(result->punteroALoRequisado);
			free(result);


		}

	}

	free(numTcbABuscar);

	return listaReturn;

}

List* obtenerTcbsAtras(obtenerLoQueSea_result* numeroPagina , l_proceso* proceso){

	List* listaReturn = malloc(sizeof(List));
	initlist(listaReturn);

	int* numTcbABuscar = malloc(sizeof(uint32_t));
	memcpy(numTcbABuscar, numeroPagina->punteroALoRequisado, sizeof(uint32_t));

	for(int i = 0; proceso->cantTripulantes > i; i++){

		obtenerLoQueSea_result* result = obtenerLoQueSea(sizeof(t_pcb)+i*sizeof(t_tcb), sizeof(t_tcb), proceso);

		if(result!=NULL){

			int* comparadorNum = malloc(sizeof(uint32_t));
			memcpy(comparadorNum, result->punteroALoRequisado, sizeof(uint32_t));

			t_tcb* tcb = malloc(sizeof(t_tcb));
			memcpy(tcb, result->punteroALoRequisado, sizeof(t_tcb));

			if(*comparadorNum != *numTcbABuscar){
				if(result->paginaUtilizada == numeroPagina->paginaUtilizada || result->paginaUtilizada2 == numeroPagina->paginaUtilizada){
					pushbacklist(listaReturn, tcb);
				}
			}
			free(comparadorNum);

			free(result->punteroALoRequisado);
			free(result);

		}


	}

	free(numTcbABuscar);

	return listaReturn;

}

char* obtenerTareas(l_proceso* proceso){

	sem_wait(sem_mutex_obtener_tareas);
	l_pagina* pagina = NULL;
	//l_frame* frame = NULL;

	IteratorList jterador = NULL;

	int divisionTarea = 0;
	//int restoTarea = proceso->tamanioTareas % miramhq_config->tamanio_pagina;

	int aCopiar = proceso->tamanioTareas;

	char* voidReturn = malloc(proceso->tamanioTareas);

	int cuentaWhile = sizeof(t_pcb) + (proceso->cantTripulantes * sizeof(t_tcb));

	int sumoAlgo = 0;

	while(cuentaWhile >= miramhq_config->tamanio_pagina){
		cuentaWhile -= miramhq_config->tamanio_pagina;
		divisionTarea += 1;
	}
//()(TAREA-PCB-TCB1)
	for(jterador = beginlist(*proceso->punteroTablaPaginas);jterador!=NULL;jterador = nextlist(jterador)){
		pagina = dataiterlist(jterador);

		int cuentaAcaArriba = miramhq_config->tamanio_pagina - cuentaWhile;

		if(pagina->numPagina == divisionTarea && cuentaAcaArriba <= aCopiar){
			if(pagina->bitPresencia == 1){
				memcpy(voidReturn + sumoAlgo, pagina->frame + cuentaWhile, cuentaAcaArriba);
			} else {
				memcpy(voidReturn + sumoAlgo, pagina->swap + cuentaWhile, cuentaAcaArriba);
			}
			cuentaWhile = 0;
			aCopiar -= cuentaAcaArriba;
			sumoAlgo += cuentaAcaArriba;
			divisionTarea++;
		} else if (pagina->numPagina == divisionTarea && cuentaAcaArriba > aCopiar){
			if(pagina->bitPresencia == 1){
				memcpy(voidReturn + sumoAlgo, pagina->frame + cuentaWhile, aCopiar);
			} else {
				memcpy(voidReturn + sumoAlgo, pagina->swap + cuentaWhile, aCopiar);
			}
		}

	}

	sem_post(sem_mutex_obtener_tareas);
	return voidReturn;
}

void obtenerTareasYActivarlas(l_proceso* proceso, int bytesSaltear, int tamanioTarea){

    sem_wait(sem_mutex_obtener_tareas);
    l_pagina* pagina = NULL;
    //l_frame* frame = NULL;

    IteratorList jterador = NULL;

    int divisionTarea = 0;
    //int restoTarea = proceso->tamanioTareas % miramhq_config->tamanio_pagina;

    int aCopiar = bytesSaltear + tamanioTarea;

    char* voidReturn = malloc(bytesSaltear + tamanioTarea);

    int cuentaWhile = sizeof(t_pcb) + (proceso->cantTripulantes * sizeof(t_tcb));

    int sumoAlgo = 0;

    while(cuentaWhile >= miramhq_config->tamanio_pagina){
        cuentaWhile -= miramhq_config->tamanio_pagina;
        divisionTarea += 1;
    }
//()(TAREA-PCB-TCB1)
    for(jterador = beginlist(*proceso->punteroTablaPaginas);jterador!=NULL;jterador = nextlist(jterador)){
        pagina = dataiterlist(jterador);

        int cuentaAcaArriba = miramhq_config->tamanio_pagina - cuentaWhile;

        if(pagina->numPagina == divisionTarea && cuentaAcaArriba <= aCopiar){
            modificarPagina(pagina);
            memcpy(voidReturn + sumoAlgo, pagina->frame + cuentaWhile, cuentaAcaArriba);
            cuentaWhile = 0;
            aCopiar -= cuentaAcaArriba;
            sumoAlgo += cuentaAcaArriba;
            divisionTarea++;
        } else if (pagina->numPagina == divisionTarea && cuentaAcaArriba > aCopiar){
            modificarPagina(pagina);
            memcpy(voidReturn + sumoAlgo, pagina->frame + cuentaWhile, aCopiar);
            //return voidReturn;
        }

    }

    free(voidReturn);

    sem_post(sem_mutex_obtener_tareas);
}

void imprimirDumpMemoriaPaginacion(){

	sem_wait(sem);

	List listaAux;

	initlist(&listaAux);

	l_proceso* proceso = NULL;
	l_pagina* pagina = NULL;

	IteratorList iterador = NULL;
	IteratorList jterador = NULL;

	char string_log[1000];
	sprintf(string_log, "----------------------------------------------------------------------------------------------\n");
	log_info(logger, string_log);

	char hora[500];
	char* temporal = temporal_get_string_time("%d/%m/%y %H:%M:%S");
	strncpy(hora, temporal, strlen(temporal)+1);

	sprintf(string_log, "Dump de la memoria: ");

	log_info(logger, string_log);
	log_info(logger, hora);


	for(iterador = beginlist(tablaProcesos);iterador!=NULL;iterador = nextlist(iterador)){
		proceso = dataiterlist(iterador);


		for(jterador = beginlist(*proceso->punteroTablaPaginas);jterador!=NULL;jterador = nextlist(jterador)){
			pagina = dataiterlist(jterador);

			if(pagina->bitPresencia){
				l_paginaMagicaParaTablaGlobal *paginaMagica = malloc(sizeof(l_paginaMagicaParaTablaGlobal));
				paginaMagica->idProceso = proceso->idProceso;
				paginaMagica->pagina = pagina;
				pushbacklist(&listaAux, paginaMagica);
			}

		}
	}


	sprintf(string_log, "-------------MEMORIA PRINCIPAL-------------");
	log_info(logger, string_log);

	for(int i=0; tamanioBitMapPrincipal > i; i++){

		int bitOcupado = bitarray_test_bit(bitMap, i);
		//void* frame = atlist(tablaFrames, i);


		if(bitOcupado){
			l_paginaMagicaParaTablaGlobal* paginaMagica = find_pagina_lista(i, &listaAux);
			l_pagina* paginaActual = paginaMagica->pagina;

			char string_log[200];
			sprintf(string_log, "Marco: %d     Estado: Ocupado     Proceso: %d     Pagina: %d", (paginaActual->frame - puntero_memoria_principal)/miramhq_config->tamanio_pagina, paginaMagica->idProceso, paginaActual->numPagina);
			log_info(logger, string_log);

		}else if(bitOcupado == 0){
			char string_log[200];
			sprintf(string_log, "Marco: %d     Estado: Libre     Proceso: -     Pagina: -", i);
			log_info(logger, string_log);
		}

	}

	free(temporal);
	//borrarLista(&listaAux);
	emptylist(&listaAux);
	sem_post(sem);
}

void imprimirDumpMemoriaPaginacion2(){

	sem_wait(sem);

	List listaAux;

	initlist(&listaAux);

	l_proceso* proceso = NULL;
	l_pagina* pagina = NULL;

	IteratorList iterador = NULL;
	IteratorList jterador = NULL;

	char string_log[1000];
	sprintf(string_log, "----------------------------------------------------------------------------------------------\n");
	log_info(logger, string_log);

	char hora[500];
	char* temporal = temporal_get_string_time("%d/%m/%y %H:%M:%S");
	strncpy(hora, temporal, strlen(temporal)+1);

	sprintf(string_log, "Dump de la memoria: ");

	log_info(logger, string_log);
	log_info(logger, hora);


	for(iterador = beginlist(tablaProcesos);iterador!=NULL;iterador = nextlist(iterador)){
		proceso = dataiterlist(iterador);


		for(jterador = beginlist(*proceso->punteroTablaPaginas);jterador!=NULL;jterador = nextlist(jterador)){
			pagina = dataiterlist(jterador);

			if(!pagina->bitPresencia){
				l_paginaMagicaParaTablaGlobal *paginaMagica = malloc(sizeof(l_paginaMagicaParaTablaGlobal));
				paginaMagica->idProceso = proceso->idProceso;
				paginaMagica->pagina = pagina;
				pushbacklist(&listaAux, paginaMagica);
			}

		}
	}


	sprintf(string_log, "-------------MEMORIA SWAP-------------");
	log_info(logger, string_log);

	for(int i=0; tamanioBitMapSwap > i; i++){

		int bitOcupado = bitarray_test_bit(bitMapSwap, i);
		//void* frame = atlist(tablaFrames, i);


		if(bitOcupado){
			l_paginaMagicaParaTablaGlobal* paginaMagica = find_pagina_lista_swap(i, &listaAux);
			l_pagina* paginaActual = paginaMagica->pagina;

			char string_log[200];
			sprintf(string_log, "Marco: %d     Estado: Ocupado     Proceso: %d     Pagina: %d", (paginaActual->swap - puntero_memoria_swap)/miramhq_config->tamanio_pagina, paginaMagica->idProceso, paginaActual->numPagina);
			log_info(logger, string_log);

		}else if(bitOcupado == 0){
			char string_log[200];
			sprintf(string_log, "Marco: %d     Estado: Libre     Proceso: -     Pagina: -", i);
			log_info(logger, string_log);
		}

	}

	free(temporal);
	//borrarLista(&listaAux);
	emptylist(&listaAux);
	sem_post(sem);
}

void tripulanteVolveAMemoria(l_proceso* proceso){


	obtenerLoQueSea_result* resultReturn = obtenerLoQueSea(0, sizeof(t_pcb), proceso);

	int numeroPagina = resultReturn->paginaUtilizada;
	int numeroPagina2 = resultReturn->paginaUtilizada2;

	IteratorList iterador = NULL;

	for(iterador = beginlist(*proceso->punteroTablaPaginas);iterador!=NULL;iterador = nextlist(iterador)){
		l_pagina* pagina = dataiterlist(iterador);

		if(pagina->numPagina == numeroPagina){
			modificarPagina(pagina);
		}
		if(numeroPagina2 != -1){
			if(pagina->numPagina == numeroPagina2){
				modificarPagina(pagina);
			}
		}
	}

}

l_paginaMagicaParaTablaGlobal* find_pagina_lista(int idFrame, List *tablaGlobalPaginas){

	l_paginaMagicaParaTablaGlobal* paginaMagica = NULL;
	l_pagina* pagina = NULL;

    IteratorList iterador = NULL;

    for(iterador = beginlist(*tablaGlobalPaginas);iterador!=NULL;iterador = nextlist(iterador)){
        paginaMagica = dataiterlist(iterador);
        pagina = paginaMagica->pagina;

        if(idFrame == (pagina->frame - puntero_memoria_principal)/miramhq_config->tamanio_pagina){
            return paginaMagica;
        }
    }
    return NULL;

}

l_paginaMagicaParaTablaGlobal* find_pagina_lista_swap(int idFrame, List *tablaGlobalPaginas){

	l_paginaMagicaParaTablaGlobal* paginaMagica = NULL;
	l_pagina* pagina = NULL;

    IteratorList iterador = NULL;

    for(iterador = beginlist(*tablaGlobalPaginas);iterador!=NULL;iterador = nextlist(iterador)){
        paginaMagica = dataiterlist(iterador);
        pagina = paginaMagica->pagina;

        if(idFrame == (pagina->swap - puntero_memoria_swap)/miramhq_config->tamanio_pagina){
            return paginaMagica;
        }
    }
    return NULL;

}

l_proceso *find_proceso_lista_paginacion(char* idProceso, List *procesos){

    l_proceso *proceso = NULL;

    IteratorList iterador = NULL;

    for(iterador = beginlist(*procesos);iterador!=NULL;iterador = nextlist(iterador)){
        proceso = dataiterlist(iterador);

        if(atoi(idProceso) == proceso->idProceso){
            return proceso;
        }
    }
    return NULL;

}

//--------------------------------MEMORIA SWAP---------------------------------------

void iniciarMemoriaSwap(){

    archivoSwap = open(miramhq_config->path_swap, O_RDWR, S_IRUSR | S_IWUSR);
    char* string = string_new();
    string_append(&string, "cat /dev/null > ");
    string_append(&string, miramhq_config->path_swap);

    system(string);

    void* puntero = NULL;

    lseek(archivoSwap, tamanioSwap - 1, SEEK_SET);

    write(archivoSwap, "", 1);

    puntero_memoria_swap = mmap(NULL, tamanioSwap, PROT_READ | PROT_WRITE, MAP_SHARED, archivoSwap, 0);

	puntero = puntero_memoria_swap;

    for(int i=0; tamanioBitMapSwap > i; i++){

		bitarray_clean_bit(bitMapSwap, i);

	}

    while(puntero + tamanioSwap > puntero_memoria_swap){

		pushbacklist(&tablaSwap, puntero_memoria_swap);

        puntero_memoria_swap += miramhq_config->tamanio_pagina;

	}

    puntero_memoria_swap = puntero;

    free(string);
}

int max (int x, int y){
	if (x<y) {
	return (y);

	}else  {
	return (x);
	}
}

//------------------------------FIN MEMORIA SWAP-------------------------------------

void imprimirDump(FILE* archivo){

	if(!strcmp(miramhq_config->esquema_memoria, "SEGMENTACION")){
		imprimirTodoSegmentacionArchivo(archivo);
	}else if(!strcmp(miramhq_config->esquema_memoria, "PAGINACION")){
		imprimirDumpMemoriaPaginacionArchivo(archivo);
	}else{
		char string_log[200];
		sprintf(string_log, "El -ESQUEMA_MEMORIA- seleccionado no es PAGINACION ni SEGMENTACION");
		log_info(logger, string_log);
	}

}

void dump(){
	char* timestamp_file = temporal_get_string_time("%d%m%y-%H%M%S");
	char* timestamp_cabecera = temporal_get_string_time("%d/%m/%y %H:%M:%S");
	char* cadena_guion = "--------------------------------------------------------------------------\n";
	char* cabecera = string_from_format("Dump: %s\n", timestamp_cabecera);
	char* path_archivo = string_from_format("/home/utnso/Dump_%s.dmp", timestamp_file);
	FILE* archivo_dump = (FILE*)txt_open_for_append(path_archivo);

	char string_log[200];
	sprintf(string_log, "[DUMP] Se inicia del dump de la memoria - Timestamp: %s", timestamp_file);
	log_info(logger, string_log);

	txt_write_in_file(archivo_dump, cadena_guion);
	txt_write_in_file(archivo_dump, cabecera);

	imprimirDump(archivo_dump);

	txt_write_in_file(archivo_dump, cadena_guion);

	char string_log2[200];
	sprintf(string_log2, "[DUMP] Se finaliza el dump de la memoria");
	log_info(logger, string_log2);

	txt_close_file(archivo_dump);
	free(timestamp_file);
	free(timestamp_cabecera);
	free(cabecera);
	free(path_archivo);
}

void imprimirDumpMemoriaPaginacionArchivo(FILE* archivo){

	List listaAux;

	initlist(&listaAux);

	l_proceso* proceso = NULL;
	l_pagina* pagina = NULL;

	IteratorList iterador = NULL;
	IteratorList jterador = NULL;

	for(iterador = beginlist(tablaProcesos);iterador!=NULL;iterador = nextlist(iterador)){
		proceso = dataiterlist(iterador);


		for(jterador = beginlist(*proceso->punteroTablaPaginas);jterador!=NULL;jterador = nextlist(jterador)){
			pagina = dataiterlist(jterador);

			if(pagina->bitPresencia){
				l_paginaMagicaParaTablaGlobal *paginaMagica = malloc(sizeof(l_paginaMagicaParaTablaGlobal));
				paginaMagica->idProceso = proceso->idProceso;
				paginaMagica->pagina = pagina;
				pushbacklist(&listaAux, paginaMagica);
			}

		}
	}

	for(int i=0; tamanioBitMapPrincipal > i; i++){

		int bitOcupado = bitarray_test_bit(bitMap, i);


		if(bitOcupado){
			l_paginaMagicaParaTablaGlobal* paginaMagica = find_pagina_lista(i, &listaAux);
			l_pagina* paginaActual = paginaMagica->pagina;

			char* linea = string_from_format("Marco: %d     Estado: Ocupado     Proceso: %d     Pagina: %d\n", (paginaActual->frame - puntero_memoria_principal)/miramhq_config->tamanio_pagina, paginaMagica->idProceso, paginaActual->numPagina);
			txt_write_in_file(archivo, linea);
			free(linea);

		}else if(bitOcupado == 0){

			char* linea = string_from_format("Marco: %d     Estado: Libre     Proceso: -     Pagina: -\n", i);
			txt_write_in_file(archivo, linea);
			free(linea);
		}

	}

	borrarLista(&listaAux);
	//emptylist(&listaAux);
}

