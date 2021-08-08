#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<signal.h>
#include<unistd.h>
#include<sys/socket.h>
#include<sys/stat.h>
#include<sys/types.h>
#include<sys/mman.h>
#include<netdb.h>
#include<pthread.h>
#include<stdbool.h>
#include<fcntl.h>
#include<dirent.h>
#include<unistd.h>
#include<semaphore.h>
#include<commons/log.h>
#include<commons/string.h>
#include<commons/config.h>
#include<commons/bitarray.h>
#include<commons/collections/dictionary.h>
#include<commons/collections/list.h>
#include"../../amonguscore/amonguscore/server.h"

#define PROCESS_NAME "IMONGOSTORE"

#define IP_SERVIDOR "127.0.0.1"
#define PATH_FILES "/files"
#define PATH_BITACORAS "/bitacoras"

#define IP_SERVIDOR_DISCORDIADOR "127.0.0.1"
#define PUERTO_DISCORDIADOR "6666"

#define DEFAULT_BLOCK_COUNT "32768"
#define DEFAULT_BLOCK_SIZE "256"

#define SUPERBLOQUE_FILENAME "/SuperBloque.ims"
#define BLOCKS_FILENAME "/Blocks.ims"

#define OXIGENO_FILENAME "/Oxigeno.ims"
#define COMIDA_FILENAME "/Comida.ims"
#define BASURA_FILENAME "/Basura.ims"

#define MD5_INPUT_FILENAME "/Md5Input.txt"
#define MD5_OUTPUT_FILENAME "/Md5Output.txt"

#define LENGHT_MD5 "32"

// Propiedades de los archivos de recursos
#define FILES_PROPERTY_SIZE "SIZE"
#define FILES_PROPERTY_BLOCK_COUNT "BLOCK_COUNT"
#define FILES_PROPERTY_BLOCKS "BLOCKS"
#define FILES_PROPERTY_CARACTER_LLENADO "CARACTER_LLENADO"
#define FILES_PROPERTY_MD5_ARCHIVO "MD5_ARCHIVO"

// Definición de algunas constantes de los archivos de recursos
#define FILES_BRACKETS "[]"
#define FILES_CARACTER_LLENADO_COMIDA "C"
#define FILES_CARACTER_LLENADO_OXIGENO "O"
#define FILES_CARACTER_LLENADO_BASURA "B"
#define FILES_CERO "0"
#define FILES_MD5_EMPTY "d41d8cd98f00b204e9800998ecf8427e"
#define FILES_CENTINELA "K"

#define EXTENSION ".ims"
#define TRIPULANTE_NAME "/Tripulante"
#define MAX_TRIPULANTES 99

#define SIZE_CHAR 8

#define GENERAR_OXIGENO "GENERAR_OXIGENO"
#define CONSUMIR_OXIGENO "CONSUMIR_OXIGENO"
#define GENERAR_COMIDA "GENERAR_COMIDA"
#define CONSUMIR_COMIDA "CONSUMIR_COMIDA"
#define GENERAR_BASURA "GENERAR_BASURA"
#define DESCARTAR_BASURA "DESCARTAR_BASURA"
#define NUEVO_TRIPULANTE "NUEVO_TRIPULANTE"
#define MENSAJE_TRIPULANTE "MENSAJE_TRIPULANTE"
#define OBTENER_BITACORA "OBTENER_BITACORA"
#define FSCK "FSCK"

struct dataConfigIMS{
	char* punto_montaje;
	char* puerto;
	uint32_t time_sync;
	t_list* posiciones_sabotaje;
	uint32_t cantidad_sabotajes;
	uint32_t sabotajes_enviados;
};

typedef enum{
	COMIDA = 1,
	OXIGENO = 2,
	BASURA = 3
} code_recurso;


// Funciones básicas del sistema
void run();
void leer_config();
void cargar_data_config();
void iniciar_log_general();
void join_all_threads(); // Hace un join de TODOS los hilos del sistema
void terminar_imongostore(int);
void inicializar_variables(); // Acá inicializo las variables globales que sean necesarias
void crear_semaforos_fsck(); // Crea los semáforos para las alertas de FSCK
void alerta_sabotaje(int); // Captura el SIGUSR1 y envía un mensaje al discordiador enviándole la posición del sabotaje actual
// END

// Funciones del I-Mongo-Store como SERVIDOR
void start_server_thread();
void start_server(); // Inicia el servidor del IMS. f es el handler de las peticiones (1 petición => 1 hilo)
void handler_cliente(t_result*); // Handler que va a adminstrar las peticiones que reciba el IMS y llamar a la función correspondiente
// END

// Funciones que definen los handlers para las diferentes acciones que puede enviar el discordiadior
void handler_generar_oxigeno(t_result*);
void handler_consumir_oxigeno(t_result*);
void handler_generar_comida(t_result*);
void handler_consumir_comida(t_result*);
void handler_generar_basura(t_result*);
void handler_descartar_basura(t_result*);
void handler_nuevo_tripulante(t_result*);
void handler_mensaje_tripulante(t_result*);
void handler_obtener_bitacora(t_result*);
void handler_fsck(t_result*);
// END

// Funciones para levantar el FS
void chequear_archivos_fs();
void chequear_punto_montaje();
void chequear_superbloque_file();
void chequear_blocks_file();
void chequear_files_recursos();
void chequear_files_tripulantes();
void crear_punto_montaje(char*);
bool crear_folder(char*);
bool chequear_subfolders(char*);
bool chequear_subfolder(char*);
void crear_superbloque_file(char*);
void load_superbloque_memoria(char*);
void crear_blocks_file(char*);
void load_bloque_memoria(char*);
void chequear_file_recurso_oxigeno(char*);
void chequear_file_recurso_comida(char*);
void chequear_file_recurso_basura(char*);
bool validar_file_recurso(t_config*, char*);
void regenerar_file_recurso(t_config*, char*);
// END

// Funciones para la sincronización del archivo Blocks.ims
void start_sync_thread();
void sync_blocks();
// END

// Funciones para manejo de datos de tripulantes
void cargar_tripulante_IMS(uint32_t);
void agregar_bloque_a_tripulante(uint32_t, uint32_t); // Agrega un bloque a la lista de BLOCKS en el FS. Params: uint32_t: # bloque; uint32_t: id tripulante
void actualizar_size_tripulante(uint32_t, uint32_t); // Actualiza el campo SIZE en el FS. Params: uint32_t: nuevo size; uint32_t: id tripulante
t_config* buscar_bitacora_tripulante(uint32_t); // Trae la bitácora del tripulante indicado por parámetro
sem_t* get_semaforo_tripulante(uint32_t); // Recibe el ID de un tripulante y devuelve su semáforo asociado.
void wait_semaforo_tripulante(uint32_t); // Simil wait, con algo más de lógica del tripulante
void signal_semaforo_tripulante(uint32_t); // Simil wait, con algo más de lógica del tripulante
char* create_string_path_bitacora(uint32_t); // Crea el string completo con el path para cargar la bitácora del tripulante
uint32_t get_ultimo_bloque(t_config*); // Obtiene el último bloque ocupado del tripulante. Recibe la metadata del tripulante
uint32_t get_espacio_libre_ultimo_bloque(uint32_t); // Calcula el espacio libre del último bloque ocupado por el tripulante. Recibe el espacio total de la metadata
void escribir_accion_en_FS(char*, uint32_t ); // Realiza la escritura de la acción recibida por parámetro en el FS para el tripulante recibido por parámetro
char* get_bitacora_tripulante(uint32_t); // Recibe el id de un tripulante y devuelve un string con toda la bitácora de ese tripulante
t_list* get_bloques_tripulantes(); // Devuelve una lista con todos los bloques asignados a los tripulante
char** get_bloques_tripulante(uint32_t id_tripulante); // Devuelve un char** con todos los bloques de un tripulante
// END

// Funciones para el manejo del bitarray_superbloque
uint32_t get_first_empty_block(); // Busca el primer bloque disponible. Lo marca como ocupado y devuelve su posición. El número de bloque coincide con la posición en el bitmap. Si es la posición 0 del bitmap, es el bloque #0
void free_block(uint32_t); // Libera el bloque recibido por parámetro
void free_all_blocks(char**); // Recibe una array de bloques y procede a marcar como libres a todos
t_bitarray* get_bitmap_superbloque_fs(); // Busca en el FS el bitmap del superbloque y lo carga en un t_bitarray
// END

// Funciones para el manejo de los bloques
void escribir_bloque(uint32_t numero_bloque, uint32_t inicio_bloque, uint32_t fin_bloque, char* string); // Copia en la porción de bloque definido, tantos caracteres como marque el inicio y fin
char* get_data_from_bloque(uint32_t numero_bloque, uint32_t cant_bytes); // Devuelve cant_bytes del principio del bloque indicado por parámetro
void write_all_blocks_file(t_config* metadata, uint32_t code_recurso); // Para el archivo dado, lo reescribe con el caracter apropiado
// END

// Funciones para el manejo de los recursos
void consumir_recursos(uint32_t code_recurso, uint32_t cantidad); // Consume cantidad de recursos del FS
void generar_recursos(uint32_t code_recurso, uint32_t cantidad); // Genera cantidad de recursos en el FS
t_config* generar_metadata_recursos(uint32_t code_recurso); // Trae del FS la metadata del recurso
char* crear_path_recurso(uint32_t code_recurso); // Genera el string donde está ubicado el path del recurso
void lock_mutex_recurso(uint32_t code_recurso);
void unlock_mutex_recurso(uint32_t code_recurso); // Funciones que bloquean o liberan un mutex determinado, dado por el code_recurso
char get_char_llenado(uint32_t code_recurso); // Para el code_recurso indicado, devuelve el caracter de llenado del recurso
void agregar_bloque_a_recurso(t_config* metadata, uint32_t next_bloque); // Agrega next_bloque a la metadata del recurso indicada por parámetro
void actualizar_size_and_blocks_recurso(t_config* metadata, uint32_t nueva_cantidad_bloques, uint32_t nuevo_size); // Actualiza los valores de block_count y size de metadata
void actualizar_md5_recurso(t_config* metadata); // Actualiza el MD5 del recurso en base a los bloques asignados
char* calcular_md5_recurso(char*); // Calcula y retorna el hash MD5 del string dado
t_list* get_bloques_recursos(); // Devuelve una lista con todos los bloques ocupados por los recursos
char** get_bloques_recurso(uint32_t code_recurso); // Devuelve un char** con los bloques traidos de la metadata
bool validar_block_count_recurso(uint32_t code_recurso); // Verifica para el recurso dado si la cantidad de bloques se coincide con la cantidad de bloques reales. Sino, corrige el error y devuelve true. False si está \todo OK
bool validar_blocks_recurso(uint32_t code_recurso); // Verifica para el recurso dado si los bloques son válidos dentro del FS y si no se alteró el orden de los mismos
bool validar_orden_bloques_recurso(t_config* metadata, uint32_t code_recurso); // Verifica que el orden de los bloques sea el correcto
bool validar_numeros_blocks_recurso(t_config* metadata, uint32_t code_recurso); // Verifica si los bloques del recurso son válidos en el FS
// END

// Funciones para las situaciones de sabotaje
void protocolo_contra_sabotajes(); // Cuando se recibe la señal de fsck, se activa el protocolo que llama a todas las verificaciones. Todas las verificaciones devuelven true cuando el sabotaje está en su correspondiente órbita
bool sabotaje_superbloque_cant_bloques(); // Verifica si la cantidad de bloques del sistema es la que dice el superbloque
bool sabotaje_superbloque_bitmap(); // Verifica si los bloques libres y ocupados son correctos
bool sabotaje_files_block_count(); // Verifica que la cantidad de bloques del recurso se coincida con la indicada en block_count
bool sabotaje_files_blocks(); // Verifica que los bloques de la lista de bloques sean válidos en el FS
bool sabotaje_files_size(); // Verifica que el size se corresponda con el contenido de los bloques
// END

// Funciones auxiliares
char* concat_two_strings(char*, char*); // Recibe 2 strings y devuelve uno nuevo concatenado
static void free_element (void*); // Free encapsulado para pasarlo como parámetro
void destroy_array_from_config(char**); // Destruye los datos y la estructura de un array devuelto por config_get_array_value
// END
