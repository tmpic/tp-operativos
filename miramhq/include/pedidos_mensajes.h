/*
 * pedidos_mensajes.h
 *
 *  Created on: 25 may. 2021
 *      Author: utnso
 */

#ifndef INCLUDE_PEDIDOS_MENSAJES_H_
#define INCLUDE_PEDIDOS_MENSAJES_H_

#include "pedidos_segmentacion.h"

void handle_client(t_result* result);
void handle_iniciar_patota_paginacion2(t_result* result);
void handle_recibir_ubicacion_tripulante_paginacion(t_result* result);
void handle_enviar_proxima_tarea_paginacion(t_result* result);
void handle_actualizar_estado_paginacion(t_result* result);
void handle_expulsar_tripulante_paginacion(t_result* result);



#endif /* INCLUDE_PEDIDOS_MENSAJES_H_ */
