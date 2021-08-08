#ifndef MANEJO_MENSAJES_H
#define MANEJO_MENSAJES_H

#include "amonguscore/list.h"
#include <commons/string.h>
#include <stdlib.h>
#include <math.h>
#include "discordiador_config.h"
#include <pthread.h>
#include <unistd.h>
#include <stdio.h>

int enviar_mensaje_modulo(t_modulo* modulo, char** mensaje, int cantidadMensajes);
char* enviar_mensaje_iniciar_patota(t_modulo* modulo, t_patota* mensajes);
char* enviar_mensaje_proxima_tarea(t_modulo* modulo, char* patota, char* tid );
char* unir_pid_tid(char* patota, char* tid);
void enviar_mensaje_mover_tripulante(t_modulo* modulo, char* patota, char* tid, char* posx, char* posy);
char* enviar_mensaje_expulsar_tripulante(t_modulo* modulo, char* patotaTid);
void enviar_mensaje_cambio_estado(t_modulo* modulo, char* patota, char* tid, t_status estado);
void enviar_mensaje_tarea(t_modulo* modulo, t_tarea* tarea);
void enviar_mensaje_nuevo_tripulante(t_modulo* modulo, char* pid, int tid);
char* enviar_mensaje_obtener_bitacora(t_modulo* modulo, char* pid, int tid);
void enviar_mensaje_notificar_fileSystem(t_modulo* modulo, char* mensaje, char* pid, int tid);
void enviar_mensaje_fsck(t_modulo* modulo);
//void enviar_mensaje_obtener_bitacora(t_modulo* modulo, char* pid, int tid);
int calcularNumTripulanteGlobal(char* pid, int tid);

#endif

