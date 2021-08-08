/*
 * miramhq_config.h
 *
 *  Created on: 18 jun. 2021
 *      Author: utnso
 */

#ifndef INCLUDE_MIRAMHQ_CONFIG_H_
#define INCLUDE_MIRAMHQ_CONFIG_H_

#include<commons/log.h>
#include<commons/config.h>
#include<stdlib.h>
#include<string.h>
#include <inttypes.h>
#include <semaphore.h>

//COSAS MAPITA

sem_t* sem;
sem_t* sem2;

#include <nivel-gui/nivel-gui.h>//Los pongo aca porque es el ultimo .h
#include <nivel-gui/tad_nivel.h>
NIVEL* nivel;
pthread_t mapita;
int spawnerDeTripulantes;
int cols;
int rows;



typedef struct {
	int tamanio_memoria;
	char* esquema_memoria;
	int tamanio_pagina;
	int tamanio_swap;
	char* path_swap;
	char* algoritmo_reemplazo;
	char* puerto;
	char* criterio_seleccion;
	char* ruta_log;

} t_miramhq_config;

void* puntero_memoria_principal;

t_miramhq_config* miramhq_config;
t_log* logger;
int contadorProcesosGlobal;
int contadorTripulantesGlobal;

void miramhq_init(t_miramhq_config** miramhq_config, t_log** logger);

t_miramhq_config* miramhq_config_loader(char* path_config_file);
void miramhq_config_parser(t_config* config, t_miramhq_config* miramhq_config);
void miramhq_destroy(t_miramhq_config* miramhq_config);
t_log* init_logger(char* path_logger, char* module_name, t_log_level log_level);
void terminar_miramhq(int signal);
void miramhq_finally(t_miramhq_config* miramhq_config, t_log* logger);

#endif /* INCLUDE_MIRAMHQ_CONFIG_H_ */
