/*
 * pedidos_segmentacion.h
 *
 *  Created on: 2 ago. 2021
 *      Author: utnso
 */

#ifndef INCLUDE_PEDIDOS_SEGMENTACION_H_
#define INCLUDE_PEDIDOS_SEGMENTACION_H_

#include <amonguscore/server.h>
//#include <amonguscore/list.h>
#include "memoria_principal.h"
#include<stdio.h>

typedef enum tipo_mensaje{
	iniciar_patota = 1,
	iniciar_tripulante = 2,
	recibir_ubicacion_tripulante = 3,
	enviar_proxima_tarea = 4,
	expulsar_tripulante = 5
} tipo_mensaje_t;

sem_t* sem;
int err;

void dibujar();
void handle_iniciar_patota(t_result* result);
int espacioLibreMemoria();
void* crearPcb(uint32_t idPatota, uint32_t direccionInicioTareas);
void* crearTcb(uint32_t idTripulante, char estadoTripulante, uint32_t pos_x, uint32_t pos_y, uint32_t proxima_instruccion_a_ejecutar, uint32_t puntero_PCB);
void handle_recibir_ubicacion_tripulante(t_result* result);
void handle_enviar_proxima_tarea(t_result* result);
void handle_actualizar_estado(t_result* result);
void handle_expulsar_tripulante(t_result* result);
int calcularNumTripulante(void* procesoABuscar, int numTripulante);
void free_string_split(char** string);
void resolverTarea(char* tarea);
int validarItem(char id, uint32_t pos_x, uint32_t pos_y, uint32_t cantidadRecursos);
void handle_tarea(char* tarea, uint32_t cantidadRecursos, uint32_t pos_x, uint32_t pos_y);
void agregar_n_recursos(char id, int n);
void quitar_n_recursos(char id, int n);
void imprimirPuntero(void* puntero, int tamanioTareas, int cantidadTripulantes);
int traductorCharAInt(char charmander);
char traductorTripAChar(int tripulante);


#endif /* INCLUDE_PEDIDOS_SEGMENTACION_H_ */
