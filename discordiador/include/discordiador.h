#include "planificador.h"
#include <stdio.h>
#include <readline/readline.h>
#include <readline/history.h>
#include <stdlib.h>
#include <string.h>

#define PROCESS_NAME "DISCORDIADOR"

int flag_first_init;


void expulsar_tripulante(char* patotaTid);
void iniciar_planificacion();
void listar_tripulantes();
void obtener_bitacora(char* patotaTid);
void pausar_planificacion();
int obtener_numero_mensaje(char* mensaje);
void interpretar_mensaje(char* mensaje);
void iniciar_patota(char** string_prueba, int size_msj);
char* obtener_tarea_archivo(char* path);
t_tripulante* crear_tripulante(int tid, int posx, int posy);
void inicializar_parametros();
void leer_consola();
void iniciar_servidor_desacoplado();
void iniciar_servidor_discordiador();
void handler_client(t_result* result);

