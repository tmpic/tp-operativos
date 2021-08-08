#include "../include/miramhq_config.h"




void miramhq_finally(t_miramhq_config* miramhq_config, t_log* logger) {

	miramhq_destroy(miramhq_config);
    log_destroy(logger);
}

void miramhq_init(t_miramhq_config** miramhq_config, t_log** logger){
    *miramhq_config = miramhq_config_loader("./cfg/miramhq.config");
    *logger = init_logger((*miramhq_config)->ruta_log, "miramhq", LOG_LEVEL_INFO);

}



t_miramhq_config* miramhq_config_loader(char* path_config_file) {
    t_config* config = config_create(path_config_file);
    t_miramhq_config* miramhq_config = malloc(sizeof(t_miramhq_config));

    miramhq_config_parser(config, miramhq_config);
    config_destroy(config);

    return miramhq_config;
}

void miramhq_config_parser(t_config* config, t_miramhq_config* miramhq_config) {
	miramhq_config->tamanio_memoria = config_get_int_value(config, "TAMANIO_MEMORIA");
	miramhq_config->esquema_memoria = strdup(config_get_string_value(config, "ESQUEMA_MEMORIA"));
	miramhq_config->tamanio_pagina = config_get_int_value(config, "TAMANIO_PAGINA");
	miramhq_config->tamanio_swap = config_get_int_value(config, "TAMANIO_SWAP");
	miramhq_config->path_swap = strdup(config_get_string_value(config, "PATH_SWAP"));
	miramhq_config->algoritmo_reemplazo = strdup(config_get_string_value(config, "ALGORITMO_REEMPLAZO"));
	miramhq_config->puerto = strdup(config_get_string_value(config, "PUERTO"));
	miramhq_config->criterio_seleccion = strdup(config_get_string_value(config, "CRITERIO_SELECCION"));
	miramhq_config->ruta_log = strdup(config_get_string_value(config, "RUTA_LOG"));
}

void miramhq_destroy(t_miramhq_config* miramhq_config) {
	free(miramhq_config->esquema_memoria);
    free(miramhq_config->path_swap);
    free(miramhq_config->algoritmo_reemplazo);
    free(miramhq_config->criterio_seleccion);
    free(miramhq_config->ruta_log);
    free(miramhq_config);
}

t_log* init_logger(char* path_logger, char* module_name, t_log_level log_level) {
    t_log* logger = log_create(path_logger, module_name, false, log_level);

    if (logger == NULL) {
        printf("\nERROR: the logger was not created\n");
        exit(1);
    }

    return logger;
}


