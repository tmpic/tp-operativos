#ifndef REPARTIDOR_H
#define REPARTIDOR_H

#include "amonguscore/list.h"
#include <commons/string.h>
#include <stdlib.h>
#include <semaphore.h>
#include "manejo_mensajes.h"
int contadorGlobal;


void ejecutar_tripulante(t_tripulante* tripulante);
void laburar_en_nave(t_tripulante* tripulante);
void ir_hacia_tarea(t_tripulante* tripulante);
void realizar_tarea(t_tripulante* tripulante);
bool misma_posicion(t_posicion posicion1, t_posicion posicion2);
void avanzar_hacia(t_tripulante* tripulante, t_posicion destino);
void mover_hacia_izquierda(t_tripulante* tripulante);
void mover_hacia_derecha(t_tripulante* tripulante);
void mover_hacia_abajo(t_tripulante* tripulante);
void mover_hacia_arriba(t_tripulante* tripulante);
void pasar_a_ready(t_tripulante* tripulante);
void pasar_a_block(t_tripulante* tripulante);
void desuscribirse_clock(sem_t* ciclo_cpu);
int se_ejecuta_en_exec(t_tarea* tarea);
void pasar_a_exit(t_tripulante* tripulante);
int cumplio_quantum(t_tripulante* tripulante);
void quitar_de_exec(t_tripulante* tripulante);
void habilitar_exec(t_tripulante* tripulante);
List* ordenarListaTid(List lista);
t_tarea* solicitar_siguiente_tarea(t_tripulante* tripulante);
void pasar_a_expulsar_tripulante(t_tripulante* tripulante);
void imprimirLista(List* lista, char* nombre);
void free_tripulante(t_tripulante* tripulante);

#endif

