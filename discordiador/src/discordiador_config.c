#include "../include/discordiador_config.h"
#include <string.h>

t_config* configuracion;
t_config_discordiador* config_discordiador;
int socket_imongostore;
int socket_miramhq;

t_config_discordiador* iniciar_configuracion(t_config* configuracion, t_log** logger){

	config_discordiador = malloc(sizeof(t_config_discordiador));
	config_discordiador->ip_mi_ram = config_get_string_value(configuracion,"IP_MI_RAM_HQ");
	config_discordiador->puerto_mi_ram = config_get_string_value(configuracion,"PUERTO_MI_RAM_HQ");
	config_discordiador->ip_i_mongo_store = config_get_string_value(configuracion,"IP_I_MONGO_STORE");
	config_discordiador->puerto_i_mongo_store = config_get_string_value(configuracion,"PUERTO_I_MONGO_STORE");
	config_discordiador->grado_multitarea = config_get_int_value(configuracion,"GRADO_MULTITAREA");
	config_discordiador->algoritmo = config_get_string_value(configuracion,"ALGORITMO");
	config_discordiador->quantum = config_get_int_value(configuracion,"QUANTUM");
	config_discordiador->duracion_sabotaje = config_get_int_value(configuracion,"DURACION_SABOTAJE");
	config_discordiador->retardo_ciclo_cpu = config_get_int_value(configuracion,"RETARDO_CICLO_CPU");
	config_discordiador->ip_discordiador = config_get_string_value(configuracion,"IP_DISCORDIADOR");
	config_discordiador->puerto_discordiador = config_get_string_value(configuracion,"PUERTO_DISCORDIADOR");

	modulo_miramhq = malloc(sizeof(t_modulo));
	modulo_imongo = malloc(sizeof(t_modulo));

	*logger = init_logger("./cfg/discordiador.log", "discordiador", LOG_LEVEL_INFO);

	inicializar_modulos();

	return config_discordiador;
}

t_log* init_logger(char* path_logger, char* module_name, t_log_level log_level) {
    t_log* logger = log_create(path_logger, module_name, false, log_level);

    if (logger == NULL) {
        printf("\nERROR: the logger was not created\n");
        exit(1);
    }

    return logger;
}

void inicializar_modulos(){
	modulo_miramhq->ip = config_discordiador->ip_mi_ram;
	modulo_miramhq->puerto = config_discordiador->puerto_mi_ram;
	modulo_miramhq->nombre = "DISCORDIADOR";
	modulo_miramhq->socket = 0;

	modulo_imongo->ip = config_discordiador->ip_i_mongo_store;
	modulo_imongo->puerto = config_discordiador->puerto_i_mongo_store;
	modulo_imongo->nombre = "DISCORDIADOR";
	modulo_imongo->socket = 0;
}

algoritmo_planificacion get_algoritmo_planificacion(t_config_discordiador* config) {
	algoritmo_planificacion tipo;
	char* algoritmo = config->algoritmo;
	if (strcmp(algoritmo, "FIFO") == 0) tipo = FIFO;
	if (strcmp(algoritmo, "RR") == 0) tipo = RR;

	return tipo;
}

int get_quantum(t_config_discordiador* config){
	return config->quantum;
}
/*
t_tripulante* crear_tripulante(int patota, int posX, int posY){
	t_tripulante* tripulante = malloc(sizeof(t_tripulante));

	tripulante->patota = patota;
	tripulante->posX = posX;
	tripulante->posY = posY;
	tripulante->status = NEW;
	tripulante->puede_pasar_a_ready = 1;

	return tripulante;
}
*/
void leerDatosDeArchivo(t_config_discordiador* config){
	if(string_equals_ignore_case(config->algoritmo,"FIFO")){
		printf("ALGORITMO: %s\n",config->algoritmo);
	}if(string_equals_ignore_case(config->algoritmo,"RR")){
		printf("ALGORITMO: %s\n",config->algoritmo);
		printf("QUANTUM: %d\n",config->quantum);
	}
}

t_config* leer_config(char* archivoConfig){
	return config_create(archivoConfig);
}

void inicializar_parametros(){
	initlist(&suscriptores_cpu);
	    initlist(&pcb_new);
	    initlist(&pcb_block);
	    initlist(&pcb_ready);
	    initlist(&pcb_ready);
	    initlist(&tripulantes_totales);
	    initlist(&tripulantes_emergencia);

	    sem_pcb_new =  malloc(sizeof(sem_t));
	    sem_init(sem_pcb_new, 0, 0);

	    sem_pcb_block =  malloc(sizeof(sem_t));
	    sem_init(sem_pcb_block, 0, 0);

	    sem_pcb_ready =  malloc(sizeof(sem_t));
	    sem_init(sem_pcb_ready, 0, 0);

	    sem_io_libre = malloc(sizeof(sem_t));
	    sem_init(sem_io_libre, 0, 1);

		sem_planificar =  malloc(sizeof(sem_t));
		sem_init(sem_planificar, 0, 0);

		sem_lock_new = malloc(sizeof(sem_t));
		sem_init(sem_lock_new, 0, 1);

		sem_quantum = malloc(sizeof(sem_t));
		sem_init(sem_quantum, 0, 0);

		sem_sabotaje = malloc(sizeof(sem_t));
		sem_init(sem_sabotaje, 0, 1);

		sem_continuar_clock = malloc(sizeof(sem_t));
		sem_init(sem_continuar_clock, 0, 1);

	    sem_grado_multiprocesamiento = malloc(sizeof(sem_t));
	    sem_init(sem_grado_multiprocesamiento, 0, config_discordiador->grado_multitarea);

	    sem_desuscribir = malloc(sizeof(sem_t));
	    sem_init(sem_desuscribir, 0, 0);

	    sem_planificador = malloc(sizeof(sem_t));
	    sem_init(sem_planificador, 0, 1);

	    sem_ready = malloc(sizeof(sem_t));
	    sem_init(sem_ready, 0, 1);

	    sem_enviar_msj = malloc(sizeof(sem_t));
	    sem_init(sem_enviar_msj, 0, 1);

}

char* obtener_status_char(t_status status){

	char* respuesta = obtener_status_string(status);

	char* bufferReturn = malloc(2);

	memcpy(bufferReturn, respuesta, 1);
	memcpy(bufferReturn+1, "\0", 1);

	free(respuesta);

	return bufferReturn;



}

char* obtener_status_string(t_status status){

	char* respuesta = string_new();

	switch(status){

		case NEW: string_append(&respuesta, "NEW");
		break;
		case READY: string_append(&respuesta, "READY");
		break;
		case EXEC: string_append(&respuesta, "EXEC");
		break;
		case BLOCK_IO: string_append(&respuesta, "BLOCK_IO");
		break;
		case BLOCK_EMERGENCY: string_append(&respuesta, "BLOCK_EMERGENCY");
		break;
		case EXIT: string_append(&respuesta, "EXIT");
		break;
	}

	return respuesta;

}

void free_string_split(char** string){

	int i = 0;

	while(string[i] != NULL){
		free(string[i]);
		i++;
	}

	free(string);

}

char ** list_a_char(List lista)
{
    int size = sizelist(lista);
    int i = 0;
    char ** resultado = malloc(sizeof(char*) * size);

    for(i = 0; i < size; i++)
    {
    	char* string = (char*)popfrontlist(&lista);
        resultado[i] = string;
    }

    emptylist(&lista);

    return resultado;
}

char ** separar_por_comillas(char** string_separado_por_espacios){
    List lista_separado_por_comillas;
    initlist(&lista_separado_por_comillas);

    for (int i = 0; string_separado_por_espacios[i] != NULL; i++){

        if (string_starts_with(string_separado_por_espacios[i], "\"")){
            if (string_ends_with(string_separado_por_espacios[i], "\"")){
                char* string_sin_comillas = string_substring(string_separado_por_espacios[i], 1, strlen(string_separado_por_espacios[i]) - 2);
                pushbacklist(&lista_separado_por_comillas, string_sin_comillas);
            } else {
                char* string_concatenado = string_new();
                string_append(&string_concatenado, string_separado_por_espacios[i]);
                i++;
                int finalize_correctamente = 0;
                while(string_separado_por_espacios[i] != NULL){
                    string_append(&string_concatenado, " ");
                    string_append(&string_concatenado, string_separado_por_espacios[i]);
                    if (string_ends_with(string_separado_por_espacios[i], "\"")){
                        finalize_correctamente = 1;
                        break;
                    }
                    i++;
                }
                if (finalize_correctamente == 1){
                    char* string_sin_comillas = string_substring(string_concatenado, 1, strlen(string_concatenado) - 2);
                    pushbacklist(&lista_separado_por_comillas, string_sin_comillas);
                } else {
                    return NULL;
                }
            }
        } else {
            pushbacklist(&lista_separado_por_comillas, string_separado_por_espacios[i]);
        }

    }

    char ** separado_por_comillas = list_a_char(lista_separado_por_comillas);

    free(string_separado_por_espacios);

    return separado_por_comillas;

}



