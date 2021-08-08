typedef struct {

    int idTripulante;
    char estadoTripulante; //'New', 'Ready', 'Executing', 'Blocked', 'Finished'
    int pos_x;
    int pos_y;
    int proxima_instruccion_a_ejecutar;
    int puntero_PCB;

} t_tcb;
