#include "../include/miramhq.h"

#define ASSERT_CREATE(nivel, id, err)                                                   \
    if(err) {                                                                           \
        nivel_destruir(nivel);                                                          \
        nivel_gui_terminar();                                                           \
        fprintf(stderr, "Error al crear '%c': %s\n", id, nivel_gui_string_error(err));  \
        return EXIT_FAILURE;                                                            \
    }


int main (int argc, char *argv[]){


	//ARRANCA MIRAM

	//signal(SIGINT,terminar_miramhq);
	signal(SIGUSR2,(void*)ejecutarCompactacion);
	signal(SIGUSR1, dump);

	contadorProcesosGlobal = 1;
	contadorTripulantesGlobal = 0;
	spawnerDeTripulantes = 0;
	miramhq_init(&miramhq_config, &logger);

	//char* ip = "127.0.0.1";
	//char* puerto = "15001";
	//iniciar_servidor(ip, puerto, handle_client);

	//PAGINACION
	initlist(&l_particiones);
	initlist(&tablaProcesos);
	initlist(&tablaProcesosAux);
	initlist(&tablaFrames);
	initlist(&tablaSwap);
	initlist(&pilaPaginasAlgoritmos);

	double tamanioMemoriaPrincipal = miramhq_config->tamanio_memoria;
	double tamanioMemoriaSwap = miramhq_config->tamanio_swap;

	tamanioBitMapPrincipal = (tamanioMemoriaPrincipal/256)*8;
	tamanioBitMapSwap = (tamanioMemoriaSwap/256)*8;

	punteroBitMapSwap = malloc(max(1, miramhq_config->tamanio_swap/256));
	punteroBitMap = malloc(max(1, miramhq_config->tamanio_memoria/256));

	bitMap = bitarray_create_with_mode(punteroBitMap, max(1, (miramhq_config->tamanio_memoria/256)), MSB_FIRST);
	bitMapSwap = bitarray_create_with_mode(punteroBitMapSwap, max(1, (miramhq_config->tamanio_swap/256)), MSB_FIRST);

	puntero_memoria_principal = malloc(tamanioMemoriaPrincipal);

	algoritmo = miramhq_config->algoritmo_reemplazo;
	tamanioMemoria = miramhq_config->tamanio_memoria;
	tamanioSwap = miramhq_config->tamanio_swap;

	semaforo_contador = malloc(sizeof(sem_t));
	sem_init(semaforo_contador, 0, 0);

	sem_mutex_algoritmos = malloc(sizeof(sem_t));
	sem_init(sem_mutex_algoritmos, 0, 1);

	sem_ocupar_frame = malloc(sizeof(sem_t));
	sem_init(sem_ocupar_frame, 0, 1);

	sem_mutex_swap_libre = malloc(sizeof(sem_t));
	sem_init(sem_mutex_swap_libre, 0, 1);

	sem_mutex_crear_segmento = malloc(sizeof(sem_t));
	sem_init(sem_mutex_crear_segmento, 0, 1);

	sem_mutex_eliminar_segmento = malloc(sizeof(sem_t));
	sem_init(sem_mutex_eliminar_segmento, 0, 1);

	sem_mutex_num_pagina = malloc(sizeof(sem_t));
	sem_init(sem_mutex_num_pagina, 0, 1);

	sem_mutex_inicio_patota = malloc(sizeof(sem_t));
	sem_init(sem_mutex_inicio_patota, 0, 1);

	sem_mutex_compactacion = malloc(sizeof(sem_t));
	sem_init(sem_mutex_compactacion, 0, 0);

	sem_mutex_continuar_inicio_patota  = malloc(sizeof(sem_t));
	sem_init(sem_mutex_continuar_inicio_patota, 0, 0);

	sem_mutex_obtener_tareas  = malloc(sizeof(sem_t));
	sem_init(sem_mutex_obtener_tareas, 0, 1);

	sem = malloc(sizeof(sem_t));
	sem_init(sem, 0, 1);

	sem2 = malloc(sizeof(sem_t));
	sem_init(sem2, 0, 1);

	sem3 = malloc(sizeof(sem_t));
	sem_init(sem3, 0, 0);

	sem4 = malloc(sizeof(sem_t));
	sem_init(sem4, 0, 1);

	sem_mutex_mapa = malloc(sizeof(sem_t));
	sem_init(sem_mutex_mapa, 0, 1);

	flag = 1;

	levantarElMapita();

	//iniciar_mapa();
	iniciarMemoria();

	iniciar_servidor("127.0.0.1", miramhq_config->puerto, handle_client);

}

void iniciar_mapa(){
	pthread_t thread;
	pthread_create(&thread, NULL, (void *)levantarElMapita, NULL);
	pthread_detach(thread);
}

void levantarElMapita(){
    cols = 9;
    rows = 9;

    nivel = nivel_crear("AMONGOS");

    nivel_gui_inicializar();

    nivel_gui_get_area_nivel(&cols, &rows);

    nivel_gui_dibujar(nivel);

}








