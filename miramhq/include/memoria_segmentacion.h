/*
 * memoria_segmentacion.h
 *
 *  Created on: 2 ago. 2021
 *      Author: utnso
 */

#ifndef INCLUDE_MEMORIA_SEGMENTACION_H_
#define INCLUDE_MEMORIA_SEGMENTACION_H_

#include <stdio.h>
#include <stdint.h>
#include <commons/bitarray.h>
#include <semaphore.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>
#include <commons/bitarray.h>
#include <commons/temporal.h>
#include <commons/string.h>
#include <stdbool.h>
#include <amonguscore/list.h>
#include "miramhq_config.h"
#include <stdlib.h>
#include <commons/txt.h>

#pragma pack(1);
typedef struct {

	uint32_t idTripulante;
    char estadoTripulante; //'New', 'Ready', 'Executing', 'Blocked', 'Finished'
    uint32_t pos_x;
    uint32_t pos_y;
    uint32_t proxima_instruccion_a_ejecutar;
    uint32_t puntero_PCB;

} t_tcb;

typedef struct {

	uint32_t idPatota;
	uint32_t direccionInicioTareas;

} t_pcb;

typedef enum tipo_segmento{
	tipo_pcb = 1,
	tipo_tcb = 2,
	tipo_tareas = 3,
	libre = 4
} t_tipo_segmento;

typedef struct segmento{

	char* idSegmento;
	t_tipo_segmento tipoSegmento;
    int byteInicio;
    int tamanio_segmento;
    void* direccion_inicio;
    int ocupado;

} l_segmento;

typedef struct proceso2{

    int idProceso;
    List* punteroTablaSegmentos;

} l_proceso_segmento;


typedef struct proceso3{

	int idProceso;
	int cantTripulantes;

} l_procesoAux;

// SEMAFOROS

sem_t* sem_mutex_algoritmos;
sem_t* sem_ocupar_frame;
sem_t* sem_mutex_swap_libre;
sem_t* sem_mutex_crear_segmento;
sem_t* sem_mutex_eliminar_segmento;
sem_t* sem_mutex_num_pagina;
sem_t* semaforo_contador;
sem_t* sem_mutex_inicio_patota;
sem_t* sem_mutex_continuar_inicio_patota;
sem_t* sem_mutex_compactacion;
sem_t* sem_mutex_mapa;
sem_t* sem3;
sem_t* sem4;
sem_t* sem_mutex_obtener_tareas;

//VARIABLES GLOBALES
int primeraParticion;
int flag;
List l_particiones;//Vendria a ser la tabla global de todos los segmentos.
List tablaProcesos;
List tablaFrames;
char* punteroBitMap;
t_bitarray *bitMap;
int numeroPaginaGlobal;
List pilaPaginasAlgoritmos;
List tablaProcesosAux;
//SWAP
void *puntero_memoria_swap;
int archivoSwap;
t_bitarray *bitMapSwap;
List tablaSwap;
void* punteroBitMapSwap;
int inicioClock;
IteratorList iteradorClock;

//BITMAP
char* algoritmo;
int tamanioMemoria;
int tamanioSwap;
int tamanioBitMapPrincipal;
int tamanioBitMapSwap;

void eliminar_proceso_lista(List* lista, l_proceso_segmento* proceso);
int esBorrableElProcesoSegmentacion(l_proceso_segmento* proceso);
void iniciarMemoriaSegmentacion();
void liberarSegmento(l_segmento* segmento);
l_segmento* crearSegmento(l_proceso_segmento* proceso, t_tipo_segmento tipoSegmento, char* idSegmento, int tamanio_segmento, void* estructura);
l_proceso_segmento *crearProceso(int idProceso);
l_segmento *find_segmento_lista(char* idSegmento, List *segmentos);
l_proceso_segmento *find_proceso_lista(char* idProceso, List *procesos);
void cargar_segmento_memoria(l_proceso_segmento* proceso, l_segmento* segmento, void* structACargarEnMemoria);
void* eliminar_segmento_lista(char* idSegmento, List *segmentos);
l_segmento* eliminar_segmento_lista_global(int byteInicio, List* segmentos);
void realizarParticion(int byteInicio, int size, List* lista);
int hay_hueco(int size, List* lista);
int hueco_en_memoria(int size, List* lista);
void pushearEnPosicion(l_segmento* segmentoAsignar, List* lista);
void algoritmoBoliviano();
void juntarParticionesOcupadas(List* lista);
int ejecutarCompactacion();
void imprimirDump(FILE* archivo);
void dump();
void imprimirDumpMemoriaPaginacionArchivo(FILE* archivo);
l_proceso_segmento* buscarSegmentoEntreProcesos(int byteInicio);
void imprimirTodoSegmentacionArchivo(FILE* archivo);
void imprimirTodoSegmentacion();
char* concat_two_strings(char* string1, char* string2);
void borrarLista(List* listaAEliminar);







#endif /* INCLUDE_MEMORIA_SEGMENTACION_H_ */
