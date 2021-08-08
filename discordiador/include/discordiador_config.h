#include<commons/log.h>
#include<commons/string.h>
#include<commons/config.h>
#include<commons/error.h>
#include<commons/log.h>
#include<netdb.h>
#include<pthread.h>
#include<semaphore.h>
#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<sys/socket.h>
#include<sys/syscall.h>
#include<sys/types.h>
#include<time.h>
#include<unistd.h>
#include <math.h>
#include "amonguscore/server.h"
#include "amonguscore/list.h"


typedef enum{NEW,READY,EXEC,BLOCK_IO,BLOCK_EMERGENCY,EXIT}t_status;

typedef struct {
	char* ip;
	char* puerto;
	char* nombre;
	int socket;
} t_modulo;

typedef struct{
	char* ip_mi_ram;
	char* puerto_mi_ram;
	char* ip_i_mongo_store;
	char* puerto_i_mongo_store;
	char* ip_discordiador;
	char* puerto_discordiador;
	int grado_multitarea;
	char* algoritmo;
	int quantum;
	int duracion_sabotaje;
	int retardo_ciclo_cpu;
}t_config_discordiador;

typedef struct {
    int posx;
    int posy;
} t_posicion;

typedef struct {
    char* nombre_tarea;
    t_posicion posicion;
    int duracion;
    int compuesta;
    int cantidad;
} t_tarea;

typedef struct {
    int tid;
    char* pid;
    t_status estado;
    t_posicion posicion;
    sem_t* new_a_ready;
    sem_t* ready_a_exec;
    sem_t* block_io;
    sem_t* ciclo_cpu;
    int primeraVez;
    int exit;
    t_tarea* tarea;
    int quantum;
    t_status estado_anterior_sabotaje;
} t_tripulante;

typedef struct {
	t_posicion posicion;
	t_tripulante* tripulante;
	int duracion;
} t_sabotaje;

typedef struct {
	sem_t* sem_iniciar_patota;
	List* tripulantes;
	char* tarea;
	List* posiciones;
} t_patota;

typedef enum {
	FIFO = 1,
	RR = 2
} algoritmo_planificacion;

t_modulo* modulo_miramhq;
t_modulo* modulo_imongo;
t_config_discordiador* config_discordiador;
char contadorPatota;
t_sabotaje* sabotaje;
List suscriptores_cpu;
sem_t* sem_pcb_new;
List pcb_new;
sem_t* sem_pcb_block;
List pcb_block;
sem_t* sem_pcb_ready;
List pcb_ready;
List pcb_exec;
List tripulantes_totales;
sem_t* sem_grado_multiprocesamiento;
sem_t* sem_ciclo_espera_HRRN;
sem_t* sem_planificar;
sem_t* sem_io_libre;
sem_t* sem_lock_new;
sem_t* sem_quantum;
sem_t* sem_continuar_clock;
sem_t* sem_sabotaje;
sem_t* sem_ready;
sem_t* sem_enviar_msj;
List tripulantes_emergencia;
sem_t* sem_desuscribir;
sem_t* sem_planificador;
t_log* logger;
sem_t* sem_mutex_ready;

void crear_queue_estados();
algoritmo_planificacion get_algoritmo_planificacion(t_config_discordiador* config);
int get_quantum(t_config_discordiador* config);
t_config_discordiador* iniciar_configuracion(t_config* configuracion, t_log** logger);
t_config* leer_config(char* archivoConfig);
void inicializar_modulos();
void leerDatosDeArchivo(t_config_discordiador* config);
char* obtener_status_string(t_status status);
char* obtener_status_char(t_status status);
void free_string_split(char**);
char ** separar_por_comillas(char** string_separado_por_espacios);
t_log* init_logger(char* path_logger, char* module_name, t_log_level log_level);

