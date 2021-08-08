#include "../include/imongostore.h"

t_config* configuracion;
t_log* log_general;

struct dataConfigIMS* data_config_ims;

uint32_t size_blocks_file;

// Definiciones de estructuras para mapping
uint32_t size_block;
uint32_t count_blocks;
uint32_t bytes_bitmap_superbloque;
void* map_superbloque;
t_bitarray* bitarray_superbloque;

char* map_blocks_live;
char* map_blocks_disk;

// Definiciones de HILOS
pthread_t server_imongostore;
pthread_t sync_thread;

// Definiciones de SEMAFOROS
pthread_mutex_t mutexBlocks = PTHREAD_MUTEX_INITIALIZER; // Para acceder a la variable compartida map_blocks_live
pthread_mutex_t mutexSuperbloque = PTHREAD_MUTEX_INITIALIZER; // Para acceder a la variable compartida map_superbloque
pthread_mutex_t mutex_oxigeno = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutex_basura = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutex_comida = PTHREAD_MUTEX_INITIALIZER; // Mutex para los recursos. Son esos 3 fijos, no hay necesidad de meterlos en un diccionarios

pthread_mutex_t mutex_md5 = PTHREAD_MUTEX_INITIALIZER;

// Definición de otras variables
t_dictionary* semaforosTripulantes; // Para acceder a las bitácoras de cada uno de los tripulantes. Formato: {ID_TRIPULANTE} => {SEMAFORO}
uint32_t cant_tripulantes;
uint32_t ids_tripulantes[MAX_TRIPULANTES];

// Semáforos para controlar que no haya condiciones de carrerea entre servidor/sync y FSCK
sem_t* sem_fsck_sync;
sem_t* sem_fsck_srv;

int main (void) {
	signal(SIGINT,terminar_imongostore);
	signal(SIGUSR1,alerta_sabotaje);
	run();
	return 0;
}

// Funciones básicas del sistema
void run() {
	leer_config();
	iniciar_log_general();
	inicializar_variables();
	chequear_archivos_fs();
	start_sync_thread();
	start_server_thread();
	// END

	join_all_threads(); // Siempre última función
}

void leer_config() {
	configuracion = config_create("imongostore.config");
	cargar_data_config();
}

void cargar_data_config() {
	// Persisto en memoria los datos de la config para facilitar su lectura
	data_config_ims = malloc(sizeof(struct dataConfigIMS));

	uint32_t len_punto_montaje = strlen(config_get_string_value(configuracion, "PUNTO_MONTAJE")) + 1;
	data_config_ims->punto_montaje = malloc(len_punto_montaje);
	memcpy(data_config_ims->punto_montaje, config_get_string_value(configuracion, "PUNTO_MONTAJE"), len_punto_montaje);

	uint32_t len_puerto = strlen(config_get_string_value(configuracion, "PUERTO")) + 1;
	data_config_ims->puerto = malloc(len_puerto);
	memcpy(data_config_ims->puerto, config_get_string_value(configuracion, "PUERTO"), len_puerto);

	data_config_ims->time_sync = config_get_int_value(configuracion, "TIEMPO_SINCRONIZACION");

	char** pos_sabotaje = config_get_array_value(configuracion, "POSICIONES_SABOTAJE");

	data_config_ims->posiciones_sabotaje = list_create();
	uint32_t cant_sabotajes = 0;
	char* sabotaje;

	while ((sabotaje = pos_sabotaje[cant_sabotajes]) != NULL) {
		list_add(data_config_ims->posiciones_sabotaje, sabotaje);
		cant_sabotajes++;
	}

	data_config_ims->cantidad_sabotajes = cant_sabotajes;
	data_config_ims->sabotajes_enviados = 0;

	free(pos_sabotaje);
}

void iniciar_log_general() {
	log_general = log_create("imongostore.log", "I-Mongo-Store", 1, LOG_LEVEL_INFO);
}

void join_all_threads() {
	pthread_join(server_imongostore, NULL);
	pthread_join(sync_thread, NULL);
}

void inicializar_variables() {
	semaforosTripulantes = dictionary_create();
	cant_tripulantes = 0;
	crear_semaforos_fsck();
}

void crear_semaforos_fsck() {
	sem_fsck_sync = malloc(sizeof(sem_t));
	sem_init(sem_fsck_sync, 0, 1);

	sem_fsck_srv = malloc(sizeof(sem_t));
	sem_init(sem_fsck_srv, 0, 2);
}

void terminar_imongostore(int signal) {
	// Acá meter TOOOOOOOOOODO lo que haya que limpiar de estructuras
	pthread_cancel(sync_thread);
	pthread_cancel(server_imongostore);
	config_destroy(configuracion);
	log_destroy(log_general);
	dictionary_destroy_and_destroy_elements(semaforosTripulantes, (void*) free_element);
	list_destroy_and_destroy_elements(data_config_ims->posiciones_sabotaje, (void*) free_element);
	munmap(map_blocks_live, size_blocks_file);
	munmap(map_blocks_disk, size_blocks_file);
	munmap(map_superbloque, bytes_bitmap_superbloque);
	bitarray_destroy(bitarray_superbloque);
	free(sem_fsck_sync);
	free(sem_fsck_srv);
	free(data_config_ims->punto_montaje);
	free(data_config_ims->puerto);
	free(data_config_ims);
}

void alerta_sabotaje(int signal) {
	// Debo enviar al discordiador la posición de sabotaje actual. Primero hay que chequear si hay posiciones disponibles
	if (data_config_ims->cantidad_sabotajes == data_config_ims->sabotajes_enviados) { // Ya se hicieron todos los sabotajes posibles
		log_warning(log_general, "No hay mas posiciones de sabotajes validas");
	}

	else {
		char* posicion_sabotaje = string_duplicate(list_get(data_config_ims->posiciones_sabotaje, data_config_ims->sabotajes_enviados));
		char* mensajes[2];

		mensajes[0] = "POSICION_SABOTAJE";
		mensajes[1] = posicion_sabotaje;

		int socket = send_messages_and_return_socket(PROCESS_NAME, IP_SERVIDOR_DISCORDIADOR, PUERTO_DISCORDIADOR, mensajes, 2);

		if(socket == -1){
			log_error(log_general, "No se puede conectar al discordiador en %s:%s. Posicion sabotaje: %s", IP_SERVIDOR_DISCORDIADOR, PUERTO_DISCORDIADOR, posicion_sabotaje);
		}

		else {
			data_config_ims->sabotajes_enviados++;
		}

		free(posicion_sabotaje);
	}
}
// ENDs

// Funciones del I-Mongo-Store como SERVIDOR
void start_server_thread (void) {
	pthread_create(&server_imongostore, NULL, (void *)start_server, NULL);
}

void start_server() {
	iniciar_servidor(IP_SERVIDOR, data_config_ims->puerto, handler_cliente);
}

void handler_cliente(t_result* result) {
	char* tipo_mensaje = result->mensajes->mensajes[0];
	// Con el tipo de mensajes, empiezo a chequear
	/*
	 * Mensajes que se reciben:
	 * GENERAR_OXIGENO cantidad
	 * CONSUMIR_OXIGENO cantidad
	 * GENERAR_COMIDA cantidad
	 * CONSUMIR_COMIDA cantidad
	 * GENERAR_BASURA cantidad
	 * DESCARTAR_BASURA
	 * NUEVO_TRIPULANTE id
	 * MENSAJE_TRIPULANTE id mensaje (uint32_t char*)
	 * OBTENER_BITACORA id
	 * FSCK
	 */
	sem_wait(sem_fsck_srv);

	if (strcmp(tipo_mensaje, GENERAR_OXIGENO) == 0) {
		handler_generar_oxigeno(result);
	}

	else if (strcmp(tipo_mensaje, CONSUMIR_OXIGENO) == 0) {
		handler_consumir_oxigeno(result);
	}

	else if (strcmp(tipo_mensaje, GENERAR_COMIDA) == 0) {
		handler_generar_comida(result);
	}

	else if (strcmp(tipo_mensaje, CONSUMIR_COMIDA) == 0) {
		handler_consumir_comida(result);
	}

	else if (strcmp(tipo_mensaje, GENERAR_BASURA) == 0) {
		handler_generar_basura(result);
	}

	else if (strcmp(tipo_mensaje, DESCARTAR_BASURA) == 0) {
		handler_descartar_basura(result);
	}

	else if (strcmp(tipo_mensaje, NUEVO_TRIPULANTE) == 0) {
		handler_nuevo_tripulante(result);
	}

	else if (strcmp(tipo_mensaje, MENSAJE_TRIPULANTE) == 0) {
		handler_mensaje_tripulante(result);
	}

	else if (strcmp(tipo_mensaje, OBTENER_BITACORA) == 0) {
		handler_obtener_bitacora(result);
	}

	else if (strcmp(tipo_mensaje, FSCK) == 0) {
		handler_fsck(result);
	}

	else { // El tipo de mensaje enviado no es uno de los aceptados

	}

	sem_post(sem_fsck_srv);
}
// END

// Funciones que definen los handlers para las diferentes acciones que puede enviar el discordiadior
void handler_generar_oxigeno(t_result* result) {
	uint32_t cantidad = atoi(result->mensajes->mensajes[1]);
	generar_recursos(OXIGENO, cantidad);
}

void handler_consumir_oxigeno(t_result* result) {
	uint32_t cantidad = atoi(result->mensajes->mensajes[1]);
	consumir_recursos(OXIGENO, cantidad);
}

void handler_generar_comida(t_result* result) {
	uint32_t cantidad = atoi(result->mensajes->mensajes[1]);
	generar_recursos(COMIDA, cantidad);
}

void handler_consumir_comida(t_result* result) {
	uint32_t cantidad = atoi(result->mensajes->mensajes[1]);
	consumir_recursos(COMIDA, cantidad);
}

void handler_generar_basura(t_result* result) {
	uint32_t cantidad = atoi(result->mensajes->mensajes[1]);
	generar_recursos(BASURA, cantidad);
}

void handler_descartar_basura(t_result* result) {
	consumir_recursos(BASURA, 0); // 0 para mantener la interfaz
}

void handler_nuevo_tripulante(t_result* result) {
	uint32_t id_tripulante = atoi(result->mensajes->mensajes[1]);
	cargar_tripulante_IMS(id_tripulante);
	send_message_socket(result->socket, "Tripulante cargado OK");
}

void handler_mensaje_tripulante(t_result* result) {
	uint32_t id_tripulante = atoi(result->mensajes->mensajes[1]);
	char* accion = string_duplicate(result->mensajes->mensajes[2]);
	escribir_accion_en_FS(accion, id_tripulante);
	free(accion);
}

void handler_obtener_bitacora(t_result* result) {
	uint32_t id_tripulante = atoi(result->mensajes->mensajes[1]);
	char* bitacora = get_bitacora_tripulante(id_tripulante);
	send_message_socket(result->socket, bitacora);
	free(bitacora);
}

void handler_fsck(t_result* result) {
	protocolo_contra_sabotajes();
}

// END

// Funciones para levantar el FS
void chequear_archivos_fs() {
	// Debo chequear que exista el punto de montaje. Si no existe, lo creo
	chequear_punto_montaje();

	// Los archivos superbloque y blocks deben existir. Sino, los creo con los datos por defecto
	chequear_superbloque_file();
	chequear_blocks_file();

	// Chequeo los 3 archivos de recursos
	chequear_files_recursos();

	// Chequeo las bitácoras
	chequear_files_tripulantes();
}

void chequear_punto_montaje() {
	log_info(log_general, "Chequeando punto de montaje. PATH: %s", data_config_ims->punto_montaje);
	DIR* folder_montaje = opendir(data_config_ims->punto_montaje);
	if (folder_montaje == NULL) { // No existe el folder, lo debo crear
		log_warning(log_general, "El path especificado no existe. Se procede a la creacion. PATH: %s", data_config_ims->punto_montaje);
		crear_punto_montaje(data_config_ims->punto_montaje);
		log_info(log_general, "Se creo correctamente el punto de montaje. PATH: %s", data_config_ims->punto_montaje);
	}

	else { // Path existe...  hay que chequear el resto de las carpetas (/files y /files/bitacoras)
		log_info(log_general, "Punto de montaje encontrado en el FS. Chequeando subfolders. PATH: %s", data_config_ims->punto_montaje);
		chequear_subfolders(data_config_ims->punto_montaje);
	}

	closedir(folder_montaje);
}

void crear_punto_montaje(char* punto_montaje) {
	int _mkdir_base = mkdir(punto_montaje, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
	if (_mkdir_base == 0) { // Carpeta creada correctamente. Creo las subcarpetas
		log_info(log_general, "Se creo correctamente el folder. PATH: %s", punto_montaje);
		char* path_subfolder = concat_two_strings(punto_montaje, PATH_FILES);
		if (crear_folder(path_subfolder)) { // Carpeta creada correctamente. Creo la última
			log_info(log_general, "Se creo correctamente el folder. PATH: %s", path_subfolder);
			string_append(&path_subfolder, PATH_BITACORAS);
			if (crear_folder(path_subfolder)) { // Carpeta creada correctamente. Cerramos
				log_info(log_general, "Se creo correctamente el folder. PATH: %s", path_subfolder);
			}

			else { // No se creó la carpeta
				log_error(log_general, "No se puede crear el folder. PATH: %s", path_subfolder);
				exit(1);
			}
		}

		else { // No se creó la carpeta
			log_error(log_general, "No se puede crear el folder. PATH: %s", path_subfolder);
			exit(1);
		}

		free(path_subfolder); // Limpio el string creado
	}

	else { // No se pudo crear la carpeta base
		log_error(log_general, "No se puede crear el folder. PATH: %s", punto_montaje);
		exit(1);
	}
}

void chequear_superbloque_file() {
	// String auxiliar para chequear existencia del archivo superbloque
	char* superbloque_path = concat_two_strings(data_config_ims->punto_montaje, SUPERBLOQUE_FILENAME);

	log_info(log_general, "Chequeando archivo SuperBloque.ims. PATH: %s", superbloque_path);

	if (access(superbloque_path, F_OK) == 0) { // Existe el archivo de superbloque. Informo y lo verifico
		log_info(log_general, "Archivo SuperBloque encontrado. Se procede a la verificacion del mismo. PATH: %s", superbloque_path);
	}

	else { // No existe, debo crearlo
		log_warning(log_general, "El archivo SuperBloque no existe. Intentando creacion. PATH: %s", superbloque_path);
		crear_superbloque_file(superbloque_path);
	}

	load_superbloque_memoria(superbloque_path);
	free(superbloque_path);
}

void chequear_blocks_file() {
	// String auxiliar para chequear existencia del archivo blocks
	char* bloque_path = concat_two_strings(data_config_ims->punto_montaje, BLOCKS_FILENAME);

	log_info(log_general, "Chequeando archivo Blocks.ims. PATH: %s", bloque_path);

	if (access(bloque_path, F_OK) == 0) { // Existe el archivo blocks
		log_info(log_general, "Archivo Blocks encontrado. Se procede a la verificacion del mismo. PATH: %s", bloque_path);
	}

	else { // No existe, debo crearlo
		log_warning(log_general, "El archivo Blocks no existe. Intentando creacion. PATH: %s", bloque_path);
		crear_blocks_file(bloque_path);
	}

	load_bloque_memoria(bloque_path);
	free(bloque_path);
}

void chequear_files_recursos() {
	char* files_path = concat_two_strings(data_config_ims->punto_montaje, PATH_FILES);
	log_info(log_general, "Chequeando archivos de recursos. PATH: %s", files_path);
	// Chequeo los 3 archivos (oxigeno, comida y basura)
	chequear_file_recurso_oxigeno(files_path);
	chequear_file_recurso_comida(files_path);
	chequear_file_recurso_basura(files_path);

	free(files_path);
}

void chequear_files_tripulantes() {
	char* path_bitacoras = string_new();
	string_append_with_format(&path_bitacoras, "%s%s%s", data_config_ims->punto_montaje, PATH_FILES, PATH_BITACORAS);
	DIR *directorio;
	struct dirent *dir;
	char* file_bitacora;
	sem_t* semaforo_tripulante;

	directorio = opendir(path_bitacoras);
	if (directorio) {
		while ((dir = readdir(directorio)) != NULL) {
			if (dir->d_type == DT_REG) {
				/*
				 * El formato del nombre de los archivos de bitácoras es fijo. Tengo que sacar la palabra Tripulante de adelante y la extensión .ims del final
				 * Usando string_substring y pasando los siguientes parámetros, obtengo el ID del tripulante
				 * START: len(TRIPULANTE_NAME) - 1 (Longitud del string Tripulante -1 por la barra para el directorio). Lo calculo así por si este string cambia
				 * LENGTH: len(file) - 10 - 4 (Al largo total del archivo, le descuento los strings Tripulante y .ims, de 10 y 4 longitud respectivamente)
				 */
				file_bitacora = string_substring(dir->d_name, string_length(TRIPULANTE_NAME) - 1,  string_length(dir->d_name) - (string_length(TRIPULANTE_NAME) - 1) - string_length(EXTENSION));
				ids_tripulantes[cant_tripulantes] = atoi(file_bitacora);
				cant_tripulantes++;

				semaforo_tripulante = malloc(sizeof(sem_t));
				sem_init(semaforo_tripulante, 0, 1);
				dictionary_put(semaforosTripulantes, file_bitacora, semaforo_tripulante);

				free(file_bitacora);
			}
		}

		closedir(directorio);
	}

	else {
		log_error(log_general, "No se puede abrir el folder de bitacoras");
		exit(1);
	}

	free(path_bitacoras);
}

bool crear_folder(char* punto_montaje) {
	return mkdir(punto_montaje, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH) == 0 ? true : false;
}

bool chequear_subfolders(char* path) {
	bool answer;
	char* path_subfolder = concat_two_strings(path, PATH_FILES);

	if (chequear_subfolder(path_subfolder)) {
		string_append(&path_subfolder, PATH_BITACORAS);
		answer = chequear_subfolder(path_subfolder);
	}

	else {
		answer = false;
	}

	free(path_subfolder); // Limpio el string creado
	return answer;
}

bool chequear_subfolder(char* path) {
	DIR* folder_files = opendir(path);
	bool answer;
	if (folder_files == NULL) {
		log_warning(log_general, "El path especificado no existe. Se procede a la creacion. PATH: %s", path);
		if(crear_folder(path)) {
			log_info(log_general, "Se creo correctamente el folder. PATH: %s", path);
			answer = true;
		}

		else {
			log_error(log_general, "No se puede crear el folder. PATH: %s", path);
			answer = false;
		}

	}

	else {
		log_info(log_general, "El folder %s ya existe", path);
		answer = true;
	}

	closedir(folder_files);
	return answer;
}

void crear_superbloque_file(char* path) {
	FILE* superbloque_file = fopen(path, "wb");
	if (superbloque_file == NULL) { // No se pudo crear el archivo, no se puede seguir la ejecución
		log_error(log_general, "No se puede crear el archivo SuperBloque. PATH: %s", path);
		exit(1);
	}

	log_info(log_general, "Archivo SuperBloque creado correctamente. Intentando escritura. PATH: %s", path);

	uint32_t cant_bloques = atoi(DEFAULT_BLOCK_COUNT);
	uint32_t size_bloque = atoi(DEFAULT_BLOCK_SIZE);
	uint32_t size_bitarray = (cant_bloques + (SIZE_CHAR - 1)) / SIZE_CHAR;

	char* bitarray_bitmap = (char*)calloc(size_bitarray, SIZE_CHAR);
	//char* bitarray_bitmap = malloc(size_bitarray);
	//memset(bitarray_bitmap, '0', size_bitarray);

	fwrite(&size_bloque, sizeof(uint32_t), 1, superbloque_file);
	fwrite(&cant_bloques, sizeof(uint32_t), 1, superbloque_file);
	fwrite(bitarray_bitmap, size_bitarray, 1, superbloque_file);

	free(bitarray_bitmap);
	fclose(superbloque_file);
}

void load_superbloque_memoria(char* path) {
	int superbloque = open(path, O_RDWR , (mode_t)0600);
	if (superbloque == -1) { // No se puede abrir el archivo
		log_error(log_general, "No se puede abrir el archivo SuperBloque. PATH: %s", path);
		exit(1);
	}

	// Leo los 2 primeros valores (tamaño y cantidad de bloques)
	read(superbloque, &size_block, sizeof(uint32_t));
	read(superbloque, &count_blocks, sizeof(uint32_t));
	// Los informo para control
	log_info(log_general, "Tamaño del bloque: %i", size_block);
	log_info(log_general, "Cantidad de bloques: %i", count_blocks);

	// Calculo el tamaño del bitarray
	bytes_bitmap_superbloque = count_blocks / SIZE_CHAR;

	// Cargo el bitmap para mapearlo y crear el bitarray
	map_superbloque = mmap(0, bytes_bitmap_superbloque, PROT_READ | PROT_WRITE, MAP_SHARED, superbloque, 0);
	map_superbloque += 2 * sizeof(uint32_t); // Adelanto 8 bytes el puntero para no tomar los datos anteriores del archivo

	if (map_superbloque == MAP_FAILED) {
		log_error(log_general, "Fallo la carga del Superbloque en memoria");
		exit(1);
	}

	// Creo el bitarray
	bitarray_superbloque = bitarray_create_with_mode(map_superbloque, bytes_bitmap_superbloque, LSB_FIRST);

	// Tengo los datos de cantidad de bloques y tamaño del bloque. Seteo el tamaño del archivo blocks.ims
	size_blocks_file = size_block * count_blocks;

	log_info(log_general, "Archivo SuperBloque cargado correctamente");
}

void crear_blocks_file(char* path) {
	FILE* blocks_file = fopen(path, "w");
	if (blocks_file == NULL) { // No se pudo crear el archivo, no se puede seguir la ejecución
		log_error(log_general, "No se puede crear el archivo Blocks. PATH: %s", path);
		exit(1);
	}

	log_info(log_general, "Archivo Blocks creado correctamente. Intentando escritura de bloques. PATH: %s", path);

	fseek(blocks_file, size_blocks_file - 1, SEEK_SET);
	fputc('\0', blocks_file);

	log_info(log_general, "Escritura correcta. PATH: %s - SIZE: %d Bytes", path, ftell(blocks_file));
	fclose(blocks_file);
}

void load_bloque_memoria(char* path) {
	int blocks = open(path, O_RDWR , (mode_t)0600);
	if (blocks == -1) { // No se puede abrir el archivo
		log_error(log_general, "No se puede abrir el archivo Blocks. PATH: %s", path);
		exit(1);
	}

	map_blocks_live = mmap(0, size_blocks_file, PROT_READ | PROT_WRITE, MAP_SHARED, blocks, 0); // Mappeo el archivo con el que voy a trabajar en el programa
	map_blocks_disk = mmap(0, size_blocks_file, PROT_READ | PROT_WRITE, MAP_SHARED, blocks, 0); // Mappeo el archivo al bloque que voy a sincronizar a disco cada X segundos

	if (map_blocks_live == MAP_FAILED || map_blocks_disk == MAP_FAILED) {
		log_error(log_general, "Fallo la carga del Blocks");
		exit(1);
	}

	log_info(log_general, "Archivo Blocks cargado correctamente");
}

void chequear_file_recurso_oxigeno(char * path) {
	char* path_oxigeno = concat_two_strings(path, OXIGENO_FILENAME);
	log_info(log_general, "Chequeando archivo de recurso Oxigeno.ims. PATH: %s", path_oxigeno);

	if (access(path_oxigeno, F_OK) != 0) { // No existe el archivo de recursos. Informo y lo creo
		log_warning(log_general, "Archivo de recurso Oxigeno.ims no encontrado. Se procede a la creación. PATH: %s", path_oxigeno);
		FILE* file_recurso = fopen(path_oxigeno, "w");
		fclose(file_recurso); // Ya creado, lo cierro y sigo
	}

	else {
		log_info(log_general, "Archivo de recurso Oxigeno.ims encontrado. PATH: %s", path_oxigeno);
	}

	t_config* file_oxigeno = config_create(path_oxigeno);
	if (file_oxigeno == NULL) { // No existe el archivo
		log_error(log_general, "No se puede abrir el archivo de recursos Oxigeno.ims. PATH: %s", path_oxigeno);
		exit(1);
	}

	log_info(log_general, "Se procede a la verificación del archivo Oxigeno.ims. PATH: %s", path_oxigeno);

	if (validar_file_recurso(file_oxigeno, FILES_CARACTER_LLENADO_OXIGENO)) {
		log_info(log_general, "Archivo de recurso Oxigeno.ims verificado correctamente. PATH: %s", path_oxigeno);
	}

	else {
		log_warning(log_general, "Archivo de recurso Oxigeno.ims inconsistente. Reparando inconsistencias. PATH: %s", path_oxigeno);
		regenerar_file_recurso(file_oxigeno, FILES_CARACTER_LLENADO_OXIGENO);
		log_info(log_general, "Archivo de recurso Oxigeno.ims reparado correctamente. PATH: %s", path_oxigeno);
	}

	free(path_oxigeno);
	config_destroy(file_oxigeno);
}

void chequear_file_recurso_comida(char * path) {
	char* path_comida = concat_two_strings(path, COMIDA_FILENAME);
	log_info(log_general, "Chequeando archivo de recurso Comida.ims. PATH: %s", path_comida);

	if (access(path_comida, F_OK) != 0) { // No existe el archivo de recursos. Informo y lo creo
		log_warning(log_general, "Archivo de recurso Comida.ims no encontrado. Se procede a la creación. PATH: %s", path_comida);
		FILE* file_recurso = fopen(path_comida, "w");
		fclose(file_recurso); // Ya creado, lo cierro y sigo
	}

	else {
		log_info(log_general, "Archivo de recurso Comida.ims encontrado. PATH: %s", path_comida);
	}

	t_config* file_comida = config_create(path_comida);
	if (file_comida == NULL) { // No existe el archivo
		log_error(log_general, "No se puede abrir el archivo de recursos Comida.ims. PATH: %s", path_comida);
		exit(1);
	}

	log_info(log_general, "Se procede a la verificación del archivo Comida.ims. PATH: %s", path_comida);

	if (validar_file_recurso(file_comida, FILES_CARACTER_LLENADO_COMIDA)) {
		log_info(log_general, "Archivo de recurso Comida.ims verificado correctamente. PATH: %s", path_comida);
	}

	else {
		log_warning(log_general, "Archivo de recurso Comida.ims inconsistente. Reparando inconsistencias. PATH: %s", path_comida);
		regenerar_file_recurso(file_comida, FILES_CARACTER_LLENADO_COMIDA);
		log_info(log_general, "Archivo de recurso Comida.ims reparado correctamente. PATH: %s", path_comida);
	}

	free(path_comida);
	config_destroy(file_comida);
}

void chequear_file_recurso_basura(char * path) {
	char* path_basura = concat_two_strings(path, BASURA_FILENAME);
	log_info(log_general, "Chequeando archivo de recurso Basura.ims. PATH: %s", path_basura);

	if (access(path_basura, F_OK) != 0) { // No existe el archivo de recursos. Informo y lo creo
		log_warning(log_general, "Archivo de recurso Basura.ims no encontrado. Se procede a la creación. PATH: %s", path_basura);
		FILE* file_recurso = fopen(path_basura, "w");
		fclose(file_recurso); // Ya creado, lo cierro y sigo
	}

	else {
		log_info(log_general, "Archivo de recurso Basura.ims encontrado. PATH: %s", path_basura);
	}

	t_config* file_basura = config_create(path_basura);
	if (file_basura == NULL) { // No existe el archivo
		log_error(log_general, "No se puede abrir el archivo de recursos Basura.ims. PATH: %s", path_basura);
		exit(1);
	}

	log_info(log_general, "Se procede a la verificación del archivo Basura.ims. PATH: %s", path_basura);

	if (validar_file_recurso(file_basura, FILES_CARACTER_LLENADO_BASURA)) {
		log_info(log_general, "Archivo de recurso Basura.ims verificado correctamente. PATH: %s", path_basura);
	}

	else {
		log_warning(log_general, "Archivo de recurso Basura.ims inconsistente. Reparando inconsistencias. PATH: %s", path_basura);
		regenerar_file_recurso(file_basura, FILES_CARACTER_LLENADO_BASURA);
		log_info(log_general, "Archivo de recurso Basura.ims reparado correctamente. PATH: %s", path_basura);
	}

	free(path_basura);
	config_destroy(file_basura);
}

bool validar_file_recurso(t_config* file, char* caracter_llenado) {
	if (config_keys_amount(file) != 5) { // No tiene las 5 propiedades que debe tener
		return false;
	}

	if (
		!config_has_property(file, FILES_PROPERTY_SIZE) ||
		!config_has_property(file, FILES_PROPERTY_BLOCK_COUNT) ||
		!config_has_property(file, FILES_PROPERTY_BLOCKS) ||
		!config_has_property(file, FILES_PROPERTY_CARACTER_LLENADO) ||
		!config_has_property(file, FILES_PROPERTY_MD5_ARCHIVO)
	) { // No tiene alguna de las propiedades definidas
		return false;
	}

	if (strcmp(config_get_string_value(file, FILES_PROPERTY_CARACTER_LLENADO), caracter_llenado) != 0) { // El caracter de llenado no es el correcto
		return false;
	}

	return true; // Todas las validaciones OK. Asumo lo demás OK
}

void regenerar_file_recurso(t_config* file, char* caracter_llenado) {
	config_set_value(file, FILES_PROPERTY_SIZE, FILES_CERO);
	config_set_value(file, FILES_PROPERTY_BLOCK_COUNT, FILES_CERO);
	config_set_value(file, FILES_PROPERTY_BLOCKS, FILES_BRACKETS);
	config_set_value(file, FILES_PROPERTY_CARACTER_LLENADO, caracter_llenado);
	config_set_value(file, FILES_PROPERTY_MD5_ARCHIVO, FILES_MD5_EMPTY);
	config_save(file);
}
// END

// Funciones para la sincronización del archivo Blocks.ims
void start_sync_thread() {
	pthread_create(&sync_thread, NULL, (void *)sync_blocks, NULL);

}

void sync_blocks() {
	log_info(log_general, "Activada la sincronizacion de bloques cada %d segundos", data_config_ims->time_sync);
	while (1) {
		sleep(data_config_ims->time_sync); // Espero el tiempo (en segundos) especificado en el config
		sem_wait(sem_fsck_sync); // Chequeo que no haya sabotaje en curso
		log_info(log_general, "Iniciando sincronizacion de bloques");
		pthread_mutex_lock(&mutexBlocks); // Bloqueo hasta que se permita la ejecución
		memcpy(map_blocks_live, map_blocks_disk, size_blocks_file); // Copio un bloque en el otro
		pthread_mutex_unlock(&mutexBlocks); // Libero la variable compartida
		msync(map_blocks_disk, size_blocks_file, MS_SYNC); // Guardo en disco. No necesito usar un mutex porque la variable map_blocks_disk solo se utiliza en esta función => no hay riesgo de condición de carrera
		//msync(map_superbloque, bytes_bitmap_superbloque, MS_SYNC);
		log_info(log_general, "Sincronizacion de bloques finalizadas");
		sem_post(sem_fsck_sync);
	}
}
// END

// Funciones para manejo de datos de tripulantes
void cargar_tripulante_IMS(uint32_t id_tripulante) {
	// Debo verificar si el tripulante ya existe en sistema
	for (uint32_t i = 0; i < cant_tripulantes; i++) {
		if (ids_tripulantes[i] == id_tripulante) {
			log_error(log_general, "El tripulante %d ya esta ingresado. Abortando operacion", id_tripulante);
			return;
		}
	}

	char* path_tripulante = create_string_path_bitacora(id_tripulante);
	char* id_tripulante_string = string_itoa(id_tripulante);


	log_info(log_general, "Llego el tripulante %d. PATH: %s", id_tripulante, path_tripulante);

	FILE* file_tripulante = fopen(path_tripulante, "w+"); // 2 en 1, si existe lo trunca, si no, lo crea vacío
	fclose(file_tripulante); // Cierro para poder usarlo como "config"

	t_config* bitacora = config_create(path_tripulante);
	config_set_value(bitacora, FILES_PROPERTY_SIZE, FILES_CERO);
	config_set_value(bitacora, FILES_PROPERTY_BLOCKS, FILES_BRACKETS); // Seteo los valores
	config_save(bitacora); // Los guardo
	config_destroy(bitacora); // Destruyo la estructura en memoria

	sem_t* semaforo_tripulante = malloc(sizeof(sem_t));
	sem_init(semaforo_tripulante, 0, 1);
	dictionary_put(semaforosTripulantes, id_tripulante_string, semaforo_tripulante); // Creo un semáforo para este tripulante y lo meto en el diccionario de semáforos
	ids_tripulantes[cant_tripulantes] = id_tripulante;
	cant_tripulantes++;

	log_info(log_general, "Tripulante %d ingresado correctamente", id_tripulante);

	free(id_tripulante_string);
	free(path_tripulante);
}

void agregar_bloque_a_tripulante(uint32_t id_tripulante, uint32_t numero_bloque) {
	char* new_bloques;
	char* bloque_string = string_itoa(numero_bloque);

	//wait_semaforo_tripulante(id_tripulante); // Pido acceso a la región crítica para leer y escribir el archivo config

	t_config* bitacora = buscar_bitacora_tripulante(id_tripulante);
	char* bloques = config_get_string_value(bitacora, FILES_PROPERTY_BLOCKS); // Traigo los bloques como STRING

	if (strcmp(bloques, FILES_BRACKETS) == 0) { // Se carga un bloque por primera vez
		new_bloques = string_new();
		string_append(&new_bloques, "[");
		string_append(&new_bloques, bloque_string);
		string_append(&new_bloques, "]");
	}

	else { // Ya existe al menos 1 bloque cargado, manipulo el string
		new_bloques = string_substring(bloques, 0, string_length(bloques) - 1); // -1 para quitar el corchete del final
		string_append(&new_bloques, ",");
		string_append(&new_bloques, bloque_string);
		string_append(&new_bloques, "]");
	}

	config_set_value(bitacora, FILES_PROPERTY_BLOCKS, new_bloques); // Escribo los nuevos bloques
	config_save(bitacora); // Guardo los cambios
	//signal_semaforo_tripulante(id_tripulante); // Libero el semáforo

	// Por último, libero la memoria
	config_destroy(bitacora);
	free(new_bloques);
	free(bloque_string);
}

void actualizar_size_tripulante(uint32_t id_tripulante, uint32_t new_size) {
	char* new_size_string = string_itoa(new_size);

	//wait_semaforo_tripulante(id_tripulante); // Pido acceso a la región crítica para leer y escribir el archivo config
	t_config* bitacora = buscar_bitacora_tripulante(id_tripulante); // Traigo la bitácora
	config_set_value(bitacora, FILES_PROPERTY_SIZE, new_size_string); // Escribo los nuevos bloques
	config_save(bitacora); // Guardo los cambios
	//signal_semaforo_tripulante(id_tripulante); // Libero el semáforo

	// Por último, libero la memoria
	config_destroy(bitacora);
	free(new_size_string);
}

t_config* buscar_bitacora_tripulante(uint32_t id_tripulante) {
	char* path_bitacora = create_string_path_bitacora(id_tripulante);
	t_config* bitacora = config_create(path_bitacora);
	free(path_bitacora);
	return bitacora;
}

sem_t* get_semaforo_tripulante(uint32_t id_tripulante) {
	char* id_tripulante_string = string_itoa(id_tripulante);
	sem_t* semaforo = dictionary_get(semaforosTripulantes, id_tripulante_string);
	if (semaforo == NULL) { // Ojo, hay un retardo al trabajar con hilos en paralelo. Puede ser que quiera acceder al semáforo antes de que se haya creado. Pongo un retardo de 1 seg y vuelvo a intentar
		sleep(1);
		semaforo = dictionary_get(semaforosTripulantes, id_tripulante_string);
		if (semaforo == NULL) {
			log_error(log_general, "Se intento acceder a un tripulante inexistente. ID_TRIPULANTE: %d", id_tripulante);
			return NULL;
		}

	}

	free(id_tripulante_string);
	return semaforo;
}

void wait_semaforo_tripulante(uint32_t id_tripulante) {
	sem_t* sem_tripulante = get_semaforo_tripulante(id_tripulante);
	if (sem_tripulante == NULL) {
		return;
	}

	sem_wait(sem_tripulante);
}

void signal_semaforo_tripulante(uint32_t id_tripulante) {
	sem_t* sem_tripulante = get_semaforo_tripulante(id_tripulante);
	if (sem_tripulante == NULL) {
		return;
	}

	sem_post(sem_tripulante);
}

char* create_string_path_bitacora(uint32_t id_tripulante) {
	char* path_tripulante = string_new();
	char* id_tripulante_string = string_itoa(id_tripulante);
	string_append_with_format(&path_tripulante, "%s%s%s%s%s%s", data_config_ims->punto_montaje, PATH_FILES, PATH_BITACORAS, TRIPULANTE_NAME, id_tripulante_string, EXTENSION); // path_tripulante tiene la ruta correcta del archivo
	free(id_tripulante_string);
	return path_tripulante;
}

uint32_t get_ultimo_bloque(t_config* metadata_tripulante) {
	char** bloques = config_get_array_value(metadata_tripulante, FILES_PROPERTY_BLOCKS); // Traigo los bloques como ARRAY

	uint32_t i = 0;
	uint32_t numero_ultimo_bloque = -1;

	while(bloques[i] != NULL) {
		i++;
	}

	if (i == 0) {
		free(bloques);
		return numero_ultimo_bloque;
	}

	numero_ultimo_bloque = atoi(bloques[i-1]);

	destroy_array_from_config(bloques);
	return numero_ultimo_bloque;

}

uint32_t get_espacio_libre_ultimo_bloque(uint32_t size_metadata) {
	return (size_block - (size_metadata % size_block) == size_block) ? 0 : size_block - (size_metadata % size_block);
}

void escribir_accion_en_FS(char* accion, uint32_t id_tripulante) {
	uint32_t next_bloque, limite_escritura_bloque;
	char* accion_aux;
	char* accion_copy = string_duplicate(accion); // Para trabajarla sin preocuparme por el string original
	uint32_t len_accion = string_length(accion); // Para calcular el espacio que necesito
	int32_t caracteres_restantes_por_escribir = len_accion; // Para ir iterando y ver cuánto me hace falta escribir

	wait_semaforo_tripulante(id_tripulante); // Espero para acceder

	t_config* bitacora = buscar_bitacora_tripulante(id_tripulante); // Para obtener la data
	uint32_t espacio_libre_ultimo_bloque = get_espacio_libre_ultimo_bloque(config_get_int_value(bitacora, FILES_PROPERTY_SIZE)); // El espacio que hay libre en el último bloque escrito
	uint32_t ultimo_bloque = get_ultimo_bloque(bitacora); // El último bloque que se escribió

	if (espacio_libre_ultimo_bloque > 0) { // Si hay espacio en el último bloque, escribo en él lo que se pueda escribir
		limite_escritura_bloque = (len_accion >= espacio_libre_ultimo_bloque) ?  size_block - 1 : size_block - espacio_libre_ultimo_bloque + len_accion - 1;
		escribir_bloque(ultimo_bloque, (size_block - espacio_libre_ultimo_bloque), limite_escritura_bloque, accion); // Escribo el restante del último bloque usado
		//caracteres_restantes_por_escribir -= (limite_escritura_bloque - espacio_libre_ultimo_bloque + 1);
		caracteres_restantes_por_escribir -= espacio_libre_ultimo_bloque;
		if (caracteres_restantes_por_escribir > 0) {
			free(accion_copy); // Libero lo que tenía antes accion_copy para no tener leaks
			accion_copy = string_substring_from(accion, len_accion - caracteres_restantes_por_escribir); // Busco el string con el que voy a trabajar de ahora en más
		}
	}

	while (caracteres_restantes_por_escribir > 0) { // Mientras haya caracteres por escribir, los escribo (thanks captain obvious)
		limite_escritura_bloque = (caracteres_restantes_por_escribir >= size_block) ?  size_block - 1 : caracteres_restantes_por_escribir - 1;
		next_bloque = get_first_empty_block(); // Pido un bloque
		escribir_bloque(next_bloque, 0, limite_escritura_bloque, accion_copy); // Escribo en el bloque
		agregar_bloque_a_tripulante(id_tripulante, next_bloque); // Se lo asigno al tripulante en su metadata
		caracteres_restantes_por_escribir -= (limite_escritura_bloque + 1);

		if (caracteres_restantes_por_escribir > 0) {
			accion_aux = string_duplicate(accion_copy); // Para no perder referencia al hacer el siguiente free y poder calcular el nuevo accion_copy
			free(accion_copy);
			accion_copy = string_substring_from(accion_aux, limite_escritura_bloque + 1);
			free(accion_aux); // Ya no la necesito, la libero
		}
	}

	actualizar_size_tripulante(id_tripulante, len_accion + config_get_int_value(bitacora, FILES_PROPERTY_SIZE)); // Actualizo el size de la metadata
	signal_semaforo_tripulante(id_tripulante);
	free(accion_copy); // Queda un string sin liberar
	config_destroy(bitacora);
}

char* get_bitacora_tripulante(uint32_t id_tripulante) {
	uint32_t i = 0;
	uint32_t bloque_actual, bytes_a_leer;
	char* bitacora = string_new();
	char* data_bloque;

	wait_semaforo_tripulante(id_tripulante); // Espero que se habilite la lectura
	t_config* metadata = buscar_bitacora_tripulante(id_tripulante);
	char** bloques = config_get_array_value(metadata, FILES_PROPERTY_BLOCKS); // Traigo los bloques como ARRAY
	uint32_t size_bitacora = config_get_int_value(metadata, FILES_PROPERTY_SIZE); // Traigo el tamaño de la bitácora para no traer basura del último bloque
	uint32_t bytes_restantes_por_leer = size_bitacora;

	while (bytes_restantes_por_leer > 0) {
		bytes_a_leer = (bytes_restantes_por_leer < size_block) ? bytes_restantes_por_leer : size_block;
		bloque_actual = atoi(bloques[i]);
		data_bloque = get_data_from_bloque(bloque_actual, bytes_a_leer); // Traigo la info de este bloque
		string_append(&bitacora, data_bloque); // La agrego a toda la info anterior
		free(data_bloque);
		i++;
		bytes_restantes_por_leer -= bytes_a_leer; // Calculo la nueva cantidad de bytes que quedan por leer
	}

	signal_semaforo_tripulante(id_tripulante); // Libero la bitácora para su uso
	config_destroy(metadata);
	destroy_array_from_config(bloques);
	return bitacora;
}

t_list* get_bloques_tripulantes() {
	t_list* bloques_tripulantes = list_create();
	uint32_t id_tripulante;
	uint32_t indice_bloque;
	char** lista_bloques;
	char* bloque;


	for (uint32_t i = 0; i < cant_tripulantes; i++) {
		id_tripulante = ids_tripulantes[i];
		lista_bloques = get_bloques_tripulante(id_tripulante);
		indice_bloque = 0;

		while ((bloque = lista_bloques[indice_bloque]) != NULL) {
			list_add(bloques_tripulantes, bloque);
			indice_bloque++;
		}

		free(lista_bloques);
	}

	return bloques_tripulantes;
}

char** get_bloques_tripulante(uint32_t id_tripulante) {
	t_config* metadata = buscar_bitacora_tripulante(id_tripulante);
	char** bloques = config_get_array_value(metadata, FILES_PROPERTY_BLOCKS);
	config_destroy(metadata);
	return bloques;
}
// END

// Funciones para el manejo del bitarray_superbloque
uint32_t get_first_empty_block() {
	uint32_t bloque_vacio = -1; // Default. Si no hay ninguno vacio devuelvo -1 para avisar el estado
	pthread_mutex_lock(&mutexSuperbloque); // Solicito acceso a la variable
	for (int i = 0; i < bitarray_get_max_bit(bitarray_superbloque); i++) { // Busco el primer bloque vacío
		if (!bitarray_test_bit(bitarray_superbloque, i)) { // Si devuelve falso, es 0, por ende está libre
			bloque_vacio = i;
			bitarray_set_bit(bitarray_superbloque, i);
			break;
		}
	}

	pthread_mutex_unlock(&mutexSuperbloque); // Libero la variable
	return bloque_vacio;
}

void free_block(uint32_t numero_bloque) {
	pthread_mutex_lock(&mutexSuperbloque); // Solicito acceso a la variable
	bitarray_clean_bit(bitarray_superbloque, numero_bloque);
	pthread_mutex_unlock(&mutexSuperbloque); // Libero la variable
}

void free_all_blocks(char** bloques) {
	uint32_t i = 0;
	while (bloques[i] != NULL) {
		free_block(atoi(bloques[i]));
		i++;
	}
}

t_bitarray* get_bitmap_superbloque_fs() {
	char* superbloque_path = concat_two_strings(data_config_ims->punto_montaje, SUPERBLOQUE_FILENAME);
	int superbloque = open(superbloque_path, O_RDWR , (mode_t)0600);
	if (superbloque == -1) { // No se puede abrir el archivo
		log_error(log_general, "No se puede abrir el archivo SuperBloque. PATH: %s", superbloque_path);
		exit(1);
	}

	void* map_superbloque = mmap(0, bytes_bitmap_superbloque, PROT_READ | PROT_WRITE, MAP_SHARED, superbloque, 0);

	// Creo el bitarray
	t_bitarray* bitarray_superbloque = bitarray_create_with_mode(map_superbloque, bytes_bitmap_superbloque + 8, LSB_FIRST); // OJO, los primeros 8 bytes hay que saltearlos, tienen información que no nos interesa para esto
	free(superbloque_path);
	return bitarray_superbloque;
}
// END

// Funciones para el manejo de los bloques
void escribir_bloque(uint32_t numero_bloque, uint32_t inicio_bloque, uint32_t fin_bloque, char* string) {
	uint32_t bytes_a_copiar = fin_bloque - inicio_bloque + 1;
	uint32_t desplazamiento_map = (numero_bloque * size_block) + inicio_bloque;
	pthread_mutex_lock(&mutexBlocks); // Solicito acceso a la variable
	memcpy(map_blocks_live + desplazamiento_map, string, bytes_a_copiar); // Copio al mapa en vivo
	pthread_mutex_unlock(&mutexBlocks); // Libero la variable
}

char* get_data_from_bloque(uint32_t numero_bloque, uint32_t cant_bytes) {
	char* data_bloque = malloc(cant_bytes * sizeof(char) + 1); // sizeof(char) = 1, se podría evitar, d'oh
	memset(data_bloque, 0, cant_bytes * sizeof(char) + 1); // Para que Valgrind no joda
	uint32_t desplazamiento_map = numero_bloque * size_block;
	pthread_mutex_lock(&mutexBlocks); // Solicito acceso a la variable
	memcpy(data_bloque, map_blocks_live + desplazamiento_map, cant_bytes); // Copio del mapa en vivo a la variable
	pthread_mutex_unlock(&mutexBlocks); // Libero la variable
	return data_bloque;
}

void write_all_blocks_file(t_config* metadata, uint32_t code_recurso) {
	char char_llenado = get_char_llenado(code_recurso);
	uint32_t size_file = config_get_int_value(metadata, FILES_PROPERTY_SIZE);
	uint32_t size_ultimo_bloque = (size_file % size_block) == 0 ? size_block : size_file % size_block;
	uint32_t cant_bloques = config_get_int_value(metadata, FILES_PROPERTY_BLOCK_COUNT);
	char** bloques = config_get_array_value(metadata, FILES_PROPERTY_BLOCKS);
	char* bloque_completo = string_repeat(char_llenado, size_block);

	for (int i = 0; i < cant_bloques - 1; i++) { // El último bloque lo escribo aparte por el centinela
		escribir_bloque(atoi(bloques[i]), 0, size_block - 1, bloque_completo);
	}

	if (size_ultimo_bloque == size_block) { // El último bloque está completo
		escribir_bloque(atoi(bloques[cant_bloques - 1]), 0, size_ultimo_bloque - 1, bloque_completo); // Escribo el último bloque
	}

	else { // Debo generar el string con el centinela para cortar
		char* string_ultimo_bloque = string_repeat(char_llenado, size_ultimo_bloque);
		string_append(&string_ultimo_bloque, FILES_CENTINELA); // Agrego el centinela
		escribir_bloque(atoi(bloques[cant_bloques - 1]), 0, size_ultimo_bloque, string_ultimo_bloque); // Escribo el último bloque
		free(string_ultimo_bloque);
	}

	destroy_array_from_config(bloques);
	free(bloque_completo);
}
// END

// Funciones para el manejo de los recursos
void consumir_recursos(uint32_t code_recurso, uint32_t cantidad) {
	lock_mutex_recurso(code_recurso); // Bloqueo
	t_config* metadata = generar_metadata_recursos(code_recurso);

	// Tratamiento distinto para basura que para los otros 2
	if (code_recurso == BASURA) { // Si es basura, debo liberar TODOS los bloques que tenga en uso y destruir el archivo
		if (metadata == NULL) {
			log_error(log_general, "No existe basura para descartar");
			return;
		}

		char** bloques = config_get_array_value(metadata, FILES_PROPERTY_BLOCKS);
		free_all_blocks(bloques);
		remove(metadata->path);
		config_destroy(metadata);
		destroy_array_from_config(bloques);
	}

	else {
		uint32_t cantidad_actual = config_get_int_value(metadata, FILES_PROPERTY_SIZE); // A jugar con el size
		uint32_t new_size = (cantidad > cantidad_actual) ? 0 : cantidad_actual - cantidad;
		char* new_blocks;
		char* new_cant_bloques;
		char* new_size_str;

 		// 2 casos, los recursos alcanzan, o no
		if (new_size == 0) { // Caso 1: Los recursos no alcanzan
			new_blocks = string_duplicate(FILES_BRACKETS);
			new_cant_bloques = string_duplicate(FILES_CERO);

			if (cantidad > cantidad_actual) {
				log_warning(log_general, "Se quisieron eliminar mas caracteres %c de los existentes", get_char_llenado(code_recurso));
			}
		}

		else { // Caso 2: Los recursos alcanzan. Debo recalcular el string de bloques y la nueva cantidad de bloques, además de ir liberando los bloques en el bitmap
			uint32_t cantidad_bloques_actual = config_get_int_value(metadata, FILES_PROPERTY_BLOCK_COUNT);
			uint32_t bloques_a_quitar = 0; // Se quitarán esta cantidad de bloques desde la derecha
			uint32_t size_ultimo_bloque = size_block - get_espacio_libre_ultimo_bloque(cantidad_actual);
			int32_t cant_aux = cantidad;
			uint32_t caracteres_a_limpiar_ultimo_bloque = cantidad % size_block; // El resto de esta división es lo que se va a sacar del único bloque que no se saque \todo su contenido
			char** bloques = config_get_array_value(metadata, FILES_PROPERTY_BLOCKS);

			if (size_ultimo_bloque <= caracteres_a_limpiar_ultimo_bloque) {
				bloques_a_quitar++;
				cant_aux -= size_ultimo_bloque;
			}

			while (cant_aux > size_block) { // en cant_aux tengo la cantidad de caracteres a consumir, mientras haya, itero
				bloques_a_quitar++;
				cant_aux -= size_block;
			}

			for (int i = 1; i <= bloques_a_quitar; i++) {
				uint32_t bloque_actual = atoi(bloques[cantidad_bloques_actual - i]);
				free_block(bloque_actual);
			}


			// Reescribo el string de bloques
			uint32_t new_cant_bloques_aux = cantidad_bloques_actual - bloques_a_quitar;
			new_cant_bloques = string_itoa(new_cant_bloques_aux);
			new_blocks = string_new();
			string_append(&new_blocks, "[");
			char* ultimo_bloque_aux;
			for (int i = 0; i < atoi(new_cant_bloques); i++) {
				char* block_aux = bloques[i];
				string_append(&new_blocks, block_aux);
				if (i != atoi(new_cant_bloques) - 1) {
					string_append(&new_blocks, ",");
				}

				else {
					ultimo_bloque_aux = string_duplicate(block_aux);
				}
			}

			string_append(&new_blocks, "]");

			free(ultimo_bloque_aux);
			destroy_array_from_config(bloques);
		}

		new_size_str = string_itoa(new_size);

		config_set_value(metadata, FILES_PROPERTY_BLOCKS, new_blocks);
		config_set_value(metadata, FILES_PROPERTY_BLOCK_COUNT, new_cant_bloques);
		config_set_value(metadata, FILES_PROPERTY_SIZE, new_size_str);
		actualizar_md5_recurso(metadata);

		config_save(metadata);

		// Antes de liberar la memoria, veo si tengo que poner un centinela en el ultimo bloque
		uint32_t espacio_ultimo_bloque = new_size % size_block;
		if (espacio_ultimo_bloque > 0) {
			uint32_t ultimo_bloque = get_ultimo_bloque(metadata);
			char* data_bloque = get_data_from_bloque(ultimo_bloque, espacio_ultimo_bloque);
			string_append(&data_bloque, FILES_CENTINELA);
			escribir_bloque(ultimo_bloque, 0, espacio_ultimo_bloque, data_bloque);
			free(data_bloque);
		}

		config_destroy(metadata);


		free(new_size_str);
		free(new_blocks);
		free(new_cant_bloques);
	}

	unlock_mutex_recurso(code_recurso); // Libero
}

void generar_recursos(uint32_t code_recurso, uint32_t cantidad) {
	char* string_a_guardar = string_repeat(get_char_llenado(code_recurso), cantidad); // Creo el string con el que voy a trabajar
	char* string_copy = string_duplicate(string_a_guardar);
	char* string_aux;
	int32_t caracteres_restantes_por_escribir = cantidad; // Para ir iterando y ver cuánto me hace falta escribir
	uint32_t limite_escritura_bloque, next_bloque;
	uint32_t cantidad_nuevos_bloques = 0;
	lock_mutex_recurso(code_recurso); // Bloqueo

	t_config* metadata = generar_metadata_recursos(code_recurso);

	if (metadata == NULL && code_recurso == BASURA) { // Chequeo adicional por si quiero generar basura después de descartarla
		char* files_path = concat_two_strings(data_config_ims->punto_montaje, PATH_FILES);
		chequear_file_recurso_basura(files_path);
		metadata = generar_metadata_recursos(code_recurso);
		free(files_path);
	}

	uint32_t ultimo_bloque = get_ultimo_bloque(metadata); // El último bloque que se escribió
	uint32_t cantidad_actual = config_get_int_value(metadata, FILES_PROPERTY_SIZE); // Para después actualizar el size
	uint32_t cantidad_actual_bloques = config_get_int_value(metadata, FILES_PROPERTY_BLOCK_COUNT); // Para después actualizar el block count
	uint32_t espacio_libre_ultimo_bloque = get_espacio_libre_ultimo_bloque(cantidad_actual); // Para ver si puedo meter esa cantidad de caracteres en este bloque

	if (espacio_libre_ultimo_bloque > 0) { // Puedo escribir en este bloque
		limite_escritura_bloque = (cantidad >= espacio_libre_ultimo_bloque) ?  size_block - 1 : size_block - espacio_libre_ultimo_bloque + cantidad - 1;

		if (limite_escritura_bloque < (size_block - 1)) { // Agrego el centinela al final
			string_append(&string_a_guardar, FILES_CENTINELA);
			limite_escritura_bloque++;
		}

		escribir_bloque(ultimo_bloque, (size_block - espacio_libre_ultimo_bloque), limite_escritura_bloque, string_a_guardar); // Escribo el restante del último bloque usado
		caracteres_restantes_por_escribir -= espacio_libre_ultimo_bloque;
		free(string_copy);
		if (caracteres_restantes_por_escribir > 0) {
			string_copy = string_substring_from(string_a_guardar, cantidad - caracteres_restantes_por_escribir); // Busco el string con el que voy a trabajar de ahora en más
		}
	}

	while (caracteres_restantes_por_escribir > 0) { // Mientras haya caracteres por escribir, los escribo (thanks captain obvious)
		limite_escritura_bloque = (caracteres_restantes_por_escribir >= size_block) ?  size_block - 1 : caracteres_restantes_por_escribir - 1;
		next_bloque = get_first_empty_block(); // Pido un bloque

		if (caracteres_restantes_por_escribir < size_block) { // Agrego el centinela al final
			string_append(&string_copy, FILES_CENTINELA);
			limite_escritura_bloque++;
		}

		escribir_bloque(next_bloque, 0, limite_escritura_bloque, string_copy); // Escribo en el bloque
		agregar_bloque_a_recurso(metadata, next_bloque); // Se lo asigno al recurso en su metadata
		caracteres_restantes_por_escribir -= (limite_escritura_bloque + 1);
		cantidad_nuevos_bloques++;
		if (caracteres_restantes_por_escribir > 0) {
			string_aux = string_duplicate(string_copy); // Para no perder referencia al hacer el siguiente free y poder calcular el nuevo string_copy
			free(string_copy);
			string_copy = string_substring_from(string_aux, limite_escritura_bloque + 1);
			free(string_aux); // Ya no la necesito, la libero

		}

		else {
			free(string_copy);
		}
	}

	/*if ((cantidad + cantidad_actual) % size_block > 0) { // En el último bloque quedó espacio libre, agrego el centinela
		agregar_centinela(next_bloque, (cantidad + cantidad_actual) % size_block);
	}*/

	actualizar_size_and_blocks_recurso(metadata, cantidad_actual_bloques + cantidad_nuevos_bloques, cantidad + cantidad_actual);
	actualizar_md5_recurso(metadata);
	config_destroy(metadata);
	unlock_mutex_recurso(code_recurso); // Libero

	// Libero los recursos asignados
	//free(string_copy);
	free(string_a_guardar);
}

t_config* generar_metadata_recursos(uint32_t code_recurso) {
	char* path_recurso = crear_path_recurso(code_recurso);
	t_config* metadata_recurso = config_create(path_recurso);
	free(path_recurso);
	return metadata_recurso;
}

void agregar_bloque_a_recurso(t_config* metadata, uint32_t next_bloque) {
	char* bloques = config_get_string_value(metadata, FILES_PROPERTY_BLOCKS); // Traigo los bloques como STRING
	char* new_bloques;
	char* bloque_string = string_itoa(next_bloque);

	if (strcmp(bloques, FILES_BRACKETS) == 0) { // Se carga un bloque por primera vez
		new_bloques = string_new();
		string_append(&new_bloques, "[");
		string_append(&new_bloques, bloque_string);
		string_append(&new_bloques, "]");
	}

	else { // Ya existe al menos 1 bloque cargado, manipulo el string
		new_bloques = string_substring(bloques, 0, string_length(bloques) - 1); // -1 para quitar el corchete del final
		string_append(&new_bloques, ",");
		string_append(&new_bloques, bloque_string);
		string_append(&new_bloques, "]");
	}

	config_set_value(metadata, FILES_PROPERTY_BLOCKS, new_bloques); // Escribo los nuevos bloques
	config_save(metadata); // Guardo los cambios

	free(new_bloques);
	free(bloque_string);
}

char* crear_path_recurso(uint32_t code_recurso) {
	char* path = concat_two_strings(data_config_ims->punto_montaje, PATH_FILES);
	switch (code_recurso) {
		case COMIDA:
			string_append(&path, COMIDA_FILENAME);
			break;

		case OXIGENO:
			string_append(&path, OXIGENO_FILENAME);
			break;

		case BASURA:
			string_append(&path, BASURA_FILENAME);
			break;
	}

	return path;
}

void lock_mutex_recurso(uint32_t code_recurso) {
	switch (code_recurso) {
		case COMIDA:
			pthread_mutex_lock(&mutex_comida);
			break;

		case OXIGENO:
			pthread_mutex_lock(&mutex_oxigeno);
			break;

		case BASURA:
			pthread_mutex_lock(&mutex_basura);
			break;
	}
}

void unlock_mutex_recurso(uint32_t code_recurso) {
	switch (code_recurso) {
		case COMIDA:
			pthread_mutex_unlock(&mutex_comida);
			break;

		case OXIGENO:
			pthread_mutex_unlock(&mutex_oxigeno);
			break;

		case BASURA:
			pthread_mutex_unlock(&mutex_basura);
			break;
	}
}

char get_char_llenado(uint32_t code_recurso) {
	char caracter_llenado = '\0';
	switch (code_recurso) {
		case COMIDA:
			caracter_llenado = FILES_CARACTER_LLENADO_COMIDA[0];
			break;

		case OXIGENO:
			caracter_llenado = FILES_CARACTER_LLENADO_OXIGENO[0];
			break;

		case BASURA:
			caracter_llenado = FILES_CARACTER_LLENADO_BASURA[0];
			break;
	}

	return caracter_llenado;
}

void actualizar_size_and_blocks_recurso(t_config* metadata, uint32_t nueva_cantidad_bloques, uint32_t nuevo_size) {
	char* size_string = string_itoa(nuevo_size);
	char* blocks_string = string_itoa(nueva_cantidad_bloques);

	config_set_value(metadata, FILES_PROPERTY_SIZE, size_string); // Escribo el nuevo size
	config_set_value(metadata, FILES_PROPERTY_BLOCK_COUNT, blocks_string); // Escribo el nuevo block count
	config_save(metadata); // Guardo los cambios

	free(size_string);
	free(blocks_string);
}

void actualizar_md5_recurso(t_config* metadata) {
	char* string_md5 = string_new();

	char* string_bloque;
	char** bloques = config_get_array_value(metadata, FILES_PROPERTY_BLOCKS); // Traigo los bloques como array para iterar
	uint32_t i = 0;
	while (bloques[i] != NULL) { // Itero todos los bloques y agrego su contenido al string_md5
		string_bloque = get_data_from_bloque(atoi(bloques[i]), size_block); // Busco el contenido en el bloque
		string_append(&string_md5, string_bloque); // Lo agrego al string que luego usaré para calcular el md5
		free(string_bloque);
		i++;
	}

	if (i == 0) { // No hay bloques, el archivo está vacío
		config_set_value(metadata, FILES_PROPERTY_MD5_ARCHIVO, FILES_MD5_EMPTY); // Cambio la variable
	}

	else {
		// Tal vez tenga caracteres de más al final del último bloque, los elimino en base al size del archivo de recurso
		uint32_t size_recurso = atoi(config_get_string_value(metadata, FILES_PROPERTY_SIZE));
		char* string_file = string_substring_until(string_md5, size_recurso);

		char* hash_calculado = calcular_md5_recurso(string_file);

		config_set_value(metadata, FILES_PROPERTY_MD5_ARCHIVO, hash_calculado); // Cambio la variable
		free(string_file);
		free(hash_calculado);
	}

	config_save(metadata); // Escribo a disco
	free(string_md5);
	destroy_array_from_config(bloques);
}

char* calcular_md5_recurso(char* string_md5) {
	uint32_t len_md5 = atoi(LENGHT_MD5);
	char* string_cmd = string_new();
	char* hash_calculado = malloc(len_md5 * sizeof(char) + 1); // sizeof(char) = 1, se podría evitar, d'oh
	memset(hash_calculado, 0, len_md5 * sizeof(char) + 1); // Para que Valgrind no joda
	// Guardo el contenido del string en un archivo txt para luego calcular el md5
	char* md5_cmd = "md5sum";
	char* path = concat_two_strings(data_config_ims->punto_montaje, PATH_FILES);
	char* input = concat_two_strings(path, MD5_INPUT_FILENAME);
	char* output = concat_two_strings(path, MD5_OUTPUT_FILENAME);
	string_append_with_format(&string_cmd, "%s %s > %s", md5_cmd, input, output);
	pthread_mutex_lock(&mutex_md5);
	FILE* input_file = fopen(input, "w");
	fprintf(input_file, "%s", string_md5);
	fclose(input_file); // Tengo en input el contenido de todos los bloques asociados

	system(string_cmd); // Ejecuto el comando... en output está el md5 que debo traer (también tiene la ruta del archivo input)

	FILE* output_file = fopen(output, "r"); // Archivo donde está guardado el md5
	fread(hash_calculado, sizeof(char), atoi(LENGHT_MD5), output_file); // En hash_calculado tengo el nuevo hash
	//log_info(log_general, "%s", hash_calculado);
	remove(input);
	remove(output);
	pthread_mutex_unlock(&mutex_md5);	
	free(output);
	free(input);
	free(path);
	fclose(output_file);
	free(string_cmd);
	return hash_calculado;
}

t_list* get_bloques_recursos() {
	char** bloques_oxigeno = get_bloques_recurso(OXIGENO);
	char** bloques_comida = get_bloques_recurso(COMIDA);
	char** bloques_basura = get_bloques_recurso(BASURA);
	t_list* bloques_recursos = list_create();

	// Itero sobre las 3 grupos de bloques para agregarlos de a uno a la lista
	uint32_t ox = 0;
	uint32_t com = 0;
	uint32_t bas = 0;
	char* bloque;

	while ((bloque = bloques_oxigeno[ox]) != NULL) {
		 list_add(bloques_recursos, bloque);
		 ox++;
	}

	while ((bloque = bloques_comida[com]) != NULL) {
		 list_add(bloques_recursos, bloque);
		 com++;
	}

	if (bloques_basura != NULL) {
		while ((bloque = bloques_basura[bas]) != NULL) {
			 list_add(bloques_recursos, bloque);
			 bas++;
		}
	}

	free(bloques_oxigeno);
	free(bloques_comida);
	free(bloques_basura);
	return bloques_recursos;
}

char** get_bloques_recurso(uint32_t code_recurso) {
	t_config* metadata = generar_metadata_recursos(code_recurso);
	if (metadata == NULL) { // Se pidió la metadata de basura y la última acción fue descartarla
		return NULL;
	}

	char** bloques = config_get_array_value(metadata, FILES_PROPERTY_BLOCKS);
	config_destroy(metadata);
	return bloques;
}

bool validar_block_count_recurso(uint32_t code_recurso) {
	t_config* metadata = generar_metadata_recursos(code_recurso);
	bool return_value = false;
	if (metadata == NULL) { // Se pidió la metadata de basura y la última acción fue descartarla
		return return_value;
	}

	char** bloques = config_get_array_value(metadata, FILES_PROPERTY_BLOCKS);
	uint32_t block_count = config_get_int_value(metadata, FILES_PROPERTY_BLOCK_COUNT);
	uint32_t cant_bloques_real = 0;

	while (bloques[cant_bloques_real] != NULL) {
		cant_bloques_real++;
	}

	if (block_count != cant_bloques_real) { // No hay la misma cantidad de bloques, arreglo el block_count de la metadata
		return_value = true;
		char* blocks_string = string_itoa(cant_bloques_real);
		config_set_value(metadata, FILES_PROPERTY_BLOCK_COUNT, blocks_string); // Escribo el nuevo block count
		config_save(metadata); // Guardo los cambios
		free(blocks_string);
	}

	// Si no entró en el IF anterior quiere decir que no hubo sabotaje y es correcto devolver false
	// Libero la memoria
	destroy_array_from_config(bloques);
	config_destroy(metadata);
	return return_value;

}

bool validar_blocks_recurso(uint32_t code_recurso) {
	t_config* metadata = generar_metadata_recursos(code_recurso);
	if (metadata == NULL) { // Se pidió la metadata de basura y la última acción fue descartarla
		return false;
	}

	bool blocks_recurso = validar_numeros_blocks_recurso(metadata, code_recurso);
	bool orden_bloques_recurso = validar_orden_bloques_recurso(metadata, code_recurso);

	config_destroy(metadata);
	return (orden_bloques_recurso || blocks_recurso);
}

bool validar_orden_bloques_recurso(t_config* metadata, uint32_t code_recurso) {
	bool return_value = false;
	char* md5_file = config_get_string_value(metadata, FILES_PROPERTY_MD5_ARCHIVO);
	char* md5_bloques = string_new();

	char* string_bloque;
	char** bloques = config_get_array_value(metadata, FILES_PROPERTY_BLOCKS); // Traigo los bloques como array para iterar
	uint32_t i = 0;
	while (bloques[i] != NULL) { // Itero todos los bloques y agrego su contenido al string_md5
		string_bloque = get_data_from_bloque(atoi(bloques[i]), size_block); // Busco el contenido en el bloque
		string_append(&md5_bloques, string_bloque); // Lo agrego al string que luego usaré para calcular el md5
		free(string_bloque);
		i++;
	}

	char* md5_file_new = calcular_md5_recurso(md5_bloques);

	if (strcmp(md5_file, md5_file_new) != 0) { // Los bloques cambiaron de orden. El sabotaje está acá.
		write_all_blocks_file(metadata, code_recurso); // Regenero el archivo
		return_value = true;
	}

	free(md5_file_new);
	free(md5_bloques);
	destroy_array_from_config(bloques);
	return return_value;
}

bool validar_numeros_blocks_recurso(t_config* metadata, uint32_t code_recurso) {
	// Solo debo agarrar todos los bloques y verificar que sean válidos. En paralelo, voy armando el nuevo string. En este punto, si o si tenemos la cantidad de bloques correcta
	char* nuevo_string_blocks = string_new(); // Lo tengo pero solo lo uso en caso de que sea necesario
	uint32_t cant_bloques_correctos = 0;
	string_append(&nuevo_string_blocks, "[");
	uint32_t cant_bloques = config_get_int_value(metadata, FILES_PROPERTY_BLOCK_COUNT);
	char** bloques = config_get_array_value(metadata, FILES_PROPERTY_BLOCKS);
	uint32_t bloque_actual;
	bool return_value = false;

	for (int i = 0; i < cant_bloques; i++) {
		bloque_actual = atoi(bloques[i]);
		if (bloque_actual < 0 || bloque_actual >= count_blocks) { // 0 <= bloque < count_blocks... No es un bloque válido si está fuera de ese rango
			return_value = true;
		}

		else {
			string_append(&nuevo_string_blocks, bloques[i]);
			string_append(&nuevo_string_blocks, ",");
			cant_bloques_correctos++;
		}
	}

	char* new_bloque;
	for (int j = cant_bloques_correctos; j < cant_bloques; j++) { // Si entro acá es porque hay al menos 1 bloque incorrecto. Actuo en consecuencia
		new_bloque = string_itoa(get_first_empty_block());
		string_append(&nuevo_string_blocks, new_bloque);
		if (cant_bloques_correctos != cant_bloques - 1) { // No estoy en el último bloque, le tengo que agregar la ","
			string_append(&nuevo_string_blocks, ",");
		}

		free(new_bloque);
	}

	string_append(&nuevo_string_blocks, "]");

	if (return_value) { // Si es true, tengo que reescribir la metadata y los bloques
		config_set_value(metadata, FILES_PROPERTY_BLOCKS, nuevo_string_blocks);
		config_save(metadata);
		write_all_blocks_file(metadata, code_recurso);
	}


	free(nuevo_string_blocks);
	destroy_array_from_config(bloques);
	return return_value;
}
// END

// Funciones para las situaciones de sabotaje
void protocolo_contra_sabotajes() {
	sem_wait(sem_fsck_sync);
	sem_wait(sem_fsck_srv); // Bloqueo las peticiones al servidor y la sincronización a disco mientras dure el sabotaje

	log_error(log_general, "Intento de sabotaje. Deteniendo funciones y analizando los daños");
	if (sabotaje_superbloque_cant_bloques()) { // HECHO ok - TESTEADO ok
		log_error(log_general, "Se ha encontrado un sabotaje en la cantidad de bloques del Superbloque. El mismo ha sido reparado");
	}

	if (sabotaje_superbloque_bitmap()) { // HECHO ok - TESTEADO ok
		log_error(log_general, "Se ha encontrado un sabotaje en el bitmap del Superbloque. El mismo ha sido reparado");
	}

	if (sabotaje_files_block_count()) { // HECHO ok - TESTEADO ok
		log_error(log_general, "Se ha encontrado un sabotaje en el block count de un recurso. El mismo ha sido reparado");
	}

	if (sabotaje_files_blocks()) { // HECHO ok - TESTEADO ok --- De paso cañazo, esto verifica el size, por lo que no hace falta la siguiente función
		log_error(log_general, "Se ha encontrado un sabotaje en los bloques de un recurso. El mismo ha sido reparado");
	}

	/*if (sabotaje_files_size()) { // HECHO en eso - TESTEADO falta
		log_error(log_general, "Se ha encontrado un sabotaje en el size de un recurso. El mismo ha sido reparado");
	}
*/
	log_info(log_general, "Finalizado chequeo contra sabotajes");
	sem_post(sem_fsck_srv);
	sem_post(sem_fsck_sync); // Permito que los hilos vuelvan a su funcionamiento normal

}

bool sabotaje_superbloque_cant_bloques() {
	/*
	 * Cantidad de bloques: Se debe garantizar que la cantidad de bloques que hay en el sistema sea igual a la cantidad que dice haber en el superbloque.
	 * Reparación: sobreescribir “cantidad de bloques” del superbloque con la cantidad de bloques real en el disco.
	 */

	/* Como no va a haber sabotajes sobre el archivo Blocks.ims, solo debo ver las siguientes 2 condiciones:
	 * 	1) Que el uint32_t del Superbloque.ims que indica la cantidad de bloques sea el correcto
	 * 	2) Que el tamaño del Superbloque.ims sea el correcto
	 * 	La 2da condición se puede evitar. Si se altera el tamaño quiere decir que se eliminaron bloques del bitarray, por ende hay que alterar el Blocks.ims también, y no es lo que se pide como reparación
	 */
	log_info(log_general, "Buscando sabotaje en el Superbloque: Cantidad de Bloques");

	char* superbloque_path = concat_two_strings(data_config_ims->punto_montaje, SUPERBLOQUE_FILENAME);
	bool return_value = false;
	int superbloque = open(superbloque_path, O_RDWR , (mode_t)0600);
	if (superbloque == -1) { // No se puede abrir el archivo
		log_error(log_general, "No se puede abrir el archivo SuperBloque. PATH: %s", superbloque_path);
		exit(1);
	}

	uint32_t tam_bloque, cant_bloques;
	// Leo los 2 primeros valores (tamaño y cantidad de bloques)
	read(superbloque, &tam_bloque, sizeof(uint32_t));
	read(superbloque, &cant_bloques, sizeof(uint32_t));

	// 1er check, comparo contra el entero del archivo
	if (cant_bloques != count_blocks) { // Si la cantidad es distinta, se alteró el número, por lo que lo debo volver a escribir
		FILE* superblock = fopen(superbloque_path, "r+b");
		fseek(superblock, 4, SEEK_SET); // Adelanto 4 bytes el puntero
		fwrite(&count_blocks, sizeof(uint32_t), 1, superblock);
		fclose(superblock);
		return_value = true;
	}

	free(superbloque_path);
	return return_value;
}

bool sabotaje_superbloque_bitmap() {
	/*
	 * Bitmap: Se debe garantizar que los bloques libres y ocupados marcados en el bitmap sean los que dicen ser.
	 * Reparación: corregir el bitmap con lo que se halle en la metadata de los archivos.
	 */
	log_info(log_general, "Buscando sabotaje en el Superbloque: Bitmap");
	t_list* bloques_ocupados = list_create();
	bool return_value = false;
	uint32_t j = 0; // Entero auxiliar
	uint32_t indice_elemento_actual = 0; // Para ir recorriendo la lista
	uint32_t bloque_elemento_actual;

	// Parte 1: Los archivos de recursos
	t_list* bloques_ocupados_por_recursos = get_bloques_recursos();

	// Parte 2: Las bitácoras de los tripulantes
	t_list* bloques_ocupados_por_tripulantes = get_bloques_tripulantes();

	// Parte 3: Junto \todo en una sola lista
	list_add_all(bloques_ocupados, bloques_ocupados_por_recursos);
	list_add_all(bloques_ocupados, bloques_ocupados_por_tripulantes);

	// Parte 3b: Ordeno la lista 1 sola vez para después iterar más rápido
	bool _compare_int (void* int1, void* int2) { // Función auxiliar para ordenar la lista
		char* int_1 = (char*) int1;
		char* int_2 = (char*) int2;
		return (atoi(int_1) <= atoi(int_2));
	}

	list_sort(bloques_ocupados, _compare_int);

	// Parte 4: Traer el bitmap de disco que pudo haber sido alterado
	t_bitarray* bitarray_disco = get_bitmap_superbloque_fs();

	// Parte 5, Comparar los bits 1 a 1 (ocupados y no ocupados). Ojo, el bitarray_disco tiene 8 bytes (64 bits) que hay que saltear. Se arranca a leer desde el bit 64
	for (uint32_t i = 64; i < bitarray_get_max_bit(bitarray_disco); i++) {
		j = i - 64;
		// Me fijo si el bloque J está en la lista de bloques_ocupados

		if (indice_elemento_actual < list_size(bloques_ocupados)) { // Todavía no recorrí todos los elementos de la lista
			bloque_elemento_actual = atoi((char*)list_get(bloques_ocupados, indice_elemento_actual)); // Me fijo que onda con este elemento. Está en la metadata de algo, entonces tiene que estar ocupado
			//log_info(log_general, "Indice actual: %d - Bloque actual: %d", j, bloque_elemento_actual);
			if (bloque_elemento_actual == j) { // El bloque es el actual que estoy iterando. Hay que ver si valen lo mismo
				if (!bitarray_test_bit(bitarray_disco, i)) { // Si no está ocupado en el FS, hay un error y debo corregirlo
					return_value = true; // El sabotaje está acá y debo informarlo. No debo hacer ninguna acción extra. En la próxima sincronización a disco esto se va a arreglar
					bitarray_set_bit(bitarray_superbloque, j);
				}

				indice_elemento_actual++; // Incremento porque debo pasar al siguiente bloque
				// No hace falta por el else, porque si devuelve TRUE, quiere decir que es correcto
			}

			else { // El bloque actual no está en ninguna metadata. Debe ser 0 en el bitarray
				if (bitarray_test_bit(bitarray_disco, i)) {
					bitarray_clean_bit(bitarray_superbloque, j);
					return_value = true; // El sabotaje está acá y debo informarlo. No debo hacer ninguna acción extra. En la próxima sincronización a disco esto se va a arreglar
				}
			}
		}

		else { // Ya recorrí todos los elementos de la metadata. Ahora debo asumir que el bloque está libre y validar ello
			if (bitarray_test_bit(bitarray_disco, i)) {
				bitarray_clean_bit(bitarray_superbloque, j);
				return_value = true; // El sabotaje está acá y debo informarlo. No debo hacer ninguna acción extra. En la próxima sincronización a disco esto se va a arreglar
			}
		}
	}

	list_destroy(bloques_ocupados_por_recursos);
	list_destroy(bloques_ocupados_por_tripulantes);
	list_destroy_and_destroy_elements(bloques_ocupados, (void*) free_element);
	bitarray_destroy(bitarray_disco);
	return return_value;
}

bool sabotaje_files_block_count() {
	/*
	 * Block_count y blocks: Al encontrar una diferencia entre estos dos se debe restaurar la consistencia.
	 * Reparación: Actualizar el valor de Block_count en base a lo que está en la lista de Blocks.
	 */
	log_info(log_general, "Buscando sabotaje en los files: Block Count");
	bool count_oxigeno = validar_block_count_recurso(OXIGENO);
	bool count_comida = validar_block_count_recurso(COMIDA);
	bool count_basura = validar_block_count_recurso(BASURA);

	return (count_oxigeno || count_comida|| count_basura);
}

bool sabotaje_files_blocks() {
	/*
	 * Blocks: Se debe validar que los números de bloques en la lista sean válidos dentro del FS.
	 * Reparación: Restaurar archivo.
	 * El proceso de restauración de archivos consiste en tomar como referencia el size del archivo y el caracter de llenado e ir llenando los bloques hasta completar el size del archivo,
	 * en caso de que falten bloques, los mismos se deberán agregar al final del mismo.
	 * Comentario hecho por Dami sobre este último caso
		1.- Ver si algún bloque se pasa de la cantidad máxima (por ejemplo les ponemos el bloque 1024 y en el FS solo tienen 512 bloques)
		2.- Validar que los bloques estén ocupados en el bitmap (ya que capaz les seteamos un bloque vacío)
		3.- Ver si el orden de los bloques esta bien y para eso tienen que validar con el MD5 para ver que el orden de los bloque este ok (posiblemente solo falle si cambian el último bloque)
	 */
	// Debo ver 2 cosas en orden. 1) Ver si algún bloque se pasa de la cantidad máxima. 2) Ver si el orden de los bloques esta bien y para eso tienen que validar con el MD5 para ver que el orden de los bloque este ok
	log_info(log_general, "Buscando sabotaje en los files: Blocks");
	bool blocks_oxigeno = validar_blocks_recurso(OXIGENO);
	bool blocks_comida = validar_blocks_recurso(COMIDA);
	bool blocks_basura = validar_blocks_recurso(BASURA);

	return (blocks_oxigeno || blocks_comida|| blocks_basura);
}

bool sabotaje_files_size() {
	/*
	 * Size: Se debe asegurar que el tamaño del archivo sea el correcto, validando con el contenido de sus bloques.
	 * Reparación: Asumir correcto lo encontrado en los bloques.
	 */
	/*
	 * Como es el último sabotaje se puede afirmar lo siguiente:
	 * 	1) La cantidad de bloques es correcta. Si hubiera habido un sabotaje, ya estaría resuelto
	 * 	2) Los bloques están en orden. De no serlo, hubiera salta
	 */
	return false;
}
// END

// Funciones auxiliares
char* concat_two_strings(char* string1, char* string2) {
	char* new_string = string_new();
	string_append(&new_string, string1);
	string_append(&new_string, string2);
	return new_string;
}

static void free_element (void* element) {
	free(element);
}

void destroy_array_from_config(char** array) {
	uint32_t j = 0;
	while(array[j] != NULL) {
		free(array[j]);
		j++;
	}

	free(array);
}
// END

// TODO IMS - Las bitácoras se están grabando como el orto hermano



