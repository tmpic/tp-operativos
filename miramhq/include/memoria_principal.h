/*
 * memoria_principal.h
 *
 *  Created on: 9 may. 2021
 *      Author: utnso
 */

#ifndef MIRAMHQ_MEMORIA_PRINCIPAL_H_
#define MIRAMHQ_MEMORIA_PRINCIPAL_H_

#include "memoria_segmentacion.h"

typedef enum {

	LIBRE,
	OCUPADO,

} t_estado_pagina;

typedef struct {

	void* punteroALoRequisado;
	int paginaUtilizada;
	int paginaUtilizada2;

} obtenerLoQueSea_result;

typedef struct pagina{

	int numFrame;
    int numPagina;
    int bitUso;
    int bitPresencia;
    int bitModificado;
    void *frame;
    void *swap;

} l_pagina;

typedef struct paginaasd{

	int idProceso;
    l_pagina* pagina;

} l_paginaMagicaParaTablaGlobal;

typedef struct proceso{

    int idProceso;
    int tamanioTareas;
    int cantTripulantes;
    List* punteroTablaPaginas;

} l_proceso;

void iniciarMemoria();
void iniciarMemoriaPaginacion();
l_pagina* crear_pagina(l_proceso* proceso);
void cargar_bytes_pagina(void* bytesACargar, int tamanio_bytes, l_pagina* pagina);
l_proceso* crearProcesoPaginacion(int idProceso);
void pasarPaginasAPrincipal(l_proceso *proceso);
void *frameLibre();
void ocuparFrame(void* frame, t_bitarray *bitMapp, List lista);
void desocuparFrame(void* frame, t_bitarray *bitMapp, List lista);
void *frameLibreSwap();
void modificarPagina(l_pagina* paginaSwap);
void quitarSiExiste(l_pagina* paginaSwap);
int pasarAPrincipal(l_pagina* paginaSwap);
void *ejecutarAlgoritmo();
void* ejecutarLRU();
void* ejecutarClock();
void *desalojarDePrincipal(l_pagina* pagina);
void imprimirBitMap();
void imprimirLista(List lista);
void imprimirMemoria();
void imprimirTodoPaginacion();
void imprimirMemoriaPaginacion();
obtenerLoQueSea_result* obtenerLoQueSea(int paraSumar, int paraBuscar, l_proceso* proceso);
void modificarTripulanteDeLaEstructura(int numeroTripulante, l_proceso* proceso, int byteAModificar, void* valor, int pesoAAgregar);
void eliminarTripulante(int numeroTripulante, l_proceso* proceso);
void desalojarDeEstructuraAdministrativa(int numeroPagina, l_proceso* proceso);
void liberarPagina(int numeroPagina, l_proceso* proceso);
void liberarPatota(l_proceso* proceso);
int estanLasPaginasAlrededorVacias(obtenerLoQueSea_result* resultReturn, l_proceso* proceso);
int estaPcbEnPagina(obtenerLoQueSea_result* resultReturn, l_proceso* proceso);
int chequearPaginasAlrededor(obtenerLoQueSea_result* numeroPagina, l_proceso* proceso);
int esBorrableElProcesoPaginacion(l_proceso* proceso);
List* obtenerTcbsAdelante(obtenerLoQueSea_result* numeroPagina, l_proceso* proceso);
List* obtenerTcbsAtras(obtenerLoQueSea_result* numeroPagina , l_proceso* proceso);
char* obtenerTareas(l_proceso* proceso);
void obtenerTareasYActivarlas(l_proceso* proceso, int bytesSaltear, int tamanioTarea);
void imprimirDumpMemoriaPaginacion();
void imprimirDumpMemoriaPaginacion2();
void tripulanteVolveAMemoria(l_proceso* proceso);
l_paginaMagicaParaTablaGlobal* find_pagina_lista(int idFrame, List *tablaGlobalPaginas);
l_proceso *find_proceso_lista_paginacion(char* idProceso, List *procesos);
l_paginaMagicaParaTablaGlobal* find_pagina_lista_swap(int idFrame, List *tablaGlobalPaginas);
void iniciarMemoriaSwap();
int max (int x, int y);

#endif /* MIRAMHQ_MEMORIA_PRINCIPAL_H_ */
