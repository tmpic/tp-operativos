#ifndef PLANIFICADOR_H
#define PLANIFICADOR_H

#include "amonguscore/list.h"
#include <commons/string.h>
#include <stdlib.h>
#include <math.h>
#include "tripulante.h"
#include <pthread.h>
#include <unistd.h>
#include <stdio.h>

void iniciar_planificador();
void iniciar_tripulantes();
void pcb_prueba();
void iniciar_planificador_corto_plazo();
void iniciar_planificador_largo_plazo();
void iniciar_planificador_block();
void planificar_largo_plazo();
void iniciar_clock();
void planificar_sabotaje();
void clock_cpu();
void planificar_corto_plazo_FIFO();
//void asignar_datos_pedido(t_pcb* pcb);
void planificar_corto_plazo_RR();
void aumentar_quantum();
t_tripulante* obtener_tripulante_mas_cercano(t_sabotaje* sabotaje);
void planificar_block_io();
void liberarPatota(t_patota*);
void quitarDeTripGlobales(t_tripulante* tripulante);

#endif

