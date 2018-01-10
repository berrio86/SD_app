#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <sys/wait.h>
#include <signal.h>
#include <sys/stat.h>
#include <dirent.h>
#include <sys/statvfs.h>
#include <pthread.h>
#include <time.h>

//open
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "servidor.h"


struct servidor servidores[3]; // cada servidor tendra su array en las que se guardaran las direcciones todos los servidores secundarios
int sock_comunicacion[3]; // cada servidor tendra su array para guardar los sockets de comunicacion con otros servidores
int sock_listas[3]; // solo el servidor hará uso de este array, que guardara los sockets secundarios para la actualizacion de listas
int sock_secundario_listas; //solo tienen este socket los secundarios. Se usa para recibir la lista de los servidores del primario
int sock_clientes, elkarrizketa, sock_servidores; //sockets auxiliares
struct sockaddr_in servidor_dir; //servidor_dir para que cada uno guarde su direccion
struct sockaddr_in primario_dir; //primario_dir para guardar la direccion del primario, solo utilizado por secundarios
struct sockaddr_in clientes_dir, nuevosecundario_dir; // auxiliares 

int primario, puerto, contador_servidores, nuevo_secundario; //booleans y contadores
pthread_t trecibir[3]; //threads para 

socklen_t helb_tam, nuevosecundario_size;

int fifo; // file descriptor para el fifo
char myfifo[10]; // string para el nombre del fifo, que será el puerto que le asignemos nosotros al iniciar

int ultimos_recibidos[10];
int mensajes_recibidos;
int contador_mensajes;

int main(int argc, char *argv[]) {

    primario = 0;
    contador_servidores = 0;
    contador_mensajes = 1;
    nuevo_secundario = 1;

    if (argc == 1) {
        printf("**** Estableciendo servidor primario. ****\n");
        primario = 1;
        puerto = PORT_SERVIDORES;
    } else if (argc == 2) {
        //Para establecer servidores secundarios en el mismo ordenador
        printf("**** Estableciendo servidor secundario en el mismo ordenador. ****\n");
        primario = 0;
        puerto = atoi(argv[1]);
        memset(&primario_dir, 0, sizeof (primario_dir));
        primario_dir.sin_family = AF_INET;
        primario_dir.sin_addr.s_addr = htonl(INADDR_ANY);
        primario_dir.sin_port = htons(PORT_SERVIDORES);

    } else if (argc == 4) {
        //Para establecer servidores secundarios en diferentes ordenadores
        printf("**** Estableciendo servidor secundario en ordenador lejano. ****\n");
        primario = 0;
        memset(&primario_dir, 0, sizeof (primario_dir));
        primario_dir.sin_family = AF_INET;
        primario_dir.sin_addr.s_addr = htonl(atoi(argv[1]));
        primario_dir.sin_port = htons(PORT_SERVIDORES);
        puerto = PORT_SERVIDORES;
        //servidores[0]=primario_dir;

    } else {
        printf("Error: El número de parámetros es incorrecto!.\n");
        printf("- Primario (0 parámetros)\n");
        printf("- Secundario en el mismo ordenador(1 parámetros): puerto \n");
        printf("- Secundario en diferentes ordenadores(3 parámetros): IP primario + IP secundario + Puerto de secundario \n");
        exit(1);
    }

    establecerParteComun();

    if (primario == 1) {
        establecerPrimario();
    } else {
        establecerSecundario();
    }

}

void establecerParteComun() {

    pthread_t tserv;
    memset(&ultimos_recibidos, 0, sizeof (ultimos_recibidos));

    //Crear socket para servidores
    if ((sock_servidores = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("Error al crear el socket");
        exit(1);
    }
    //inicializar todos los sockets con 0s
    memset(&servidor_dir, 0, sizeof (servidor_dir));

    servidor_dir.sin_family = AF_INET;
    servidor_dir.sin_addr.s_addr = htonl(INADDR_ANY);
    servidor_dir.sin_port = htons(puerto);

    // Asignar dirección al socket	
    if (bind(sock_servidores, (struct sockaddr *) &servidor_dir, sizeof (servidor_dir)) < 0) {
        perror("Error al asignar una direccion al socket");
        exit(1);
    }

    /*ESTO ES PARTE DE LA COMUNICACION CON LOS SERVIDORES*/
    /* SE ESTABLECE LA VARIABLE puerto COMO PUERTO DE ESCUCHA SOLO PARA SERVIDORES*/

    //establecer direccion para guardar nuevas direcciones de secundarios
    memset(&nuevosecundario_dir, 0, sizeof (nuevosecundario_dir));
    nuevosecundario_dir.sin_family = AF_INET;
    nuevosecundario_size = sizeof (nuevosecundario_dir);

    // Establecer socket de escucha para servidores
    if (listen(sock_servidores, 5) < 0) {
        perror("Error al establecer socket como socket de escucha");
        exit(1);
    }

    if (pthread_create(&tserv, NULL, establecerSocketServidores, NULL)) {
        perror("Error al crear thread de clientes");
        exit(1);
    }

}

void establecerPrimario() {
    pthread_t tcli;

    /*ESTO ES PARTE DE LA COMUNICACION CON LOS CLIENTES*/
    /* SE ESTABLECE EL PUERTO 6012 COMO PUERTO DE ESCUCHA SOLO PARA CLIENTES*/
    // Crear socket para clientes
    if ((sock_clientes = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("Error al crear el socket");
        exit(1);
    }

    memset(&clientes_dir, 0, sizeof (clientes_dir));
    clientes_dir.sin_family = AF_INET;
    clientes_dir.sin_addr.s_addr = htonl(INADDR_ANY);
    clientes_dir.sin_port = htons(PORT);

    // Asignar dirección al socket	
    if (bind(sock_clientes, (struct sockaddr *) &clientes_dir, sizeof (clientes_dir)) < 0) {
        perror("Error al asignar una direccion al socket");
        exit(1);
    }

    // Establecer socket de escucha para clientes
    if (listen(sock_clientes, 5) < 0) {
        perror("Error al establecer socket como socket de escucha");
        exit(1);
    }

    if (pthread_create(&tcli, NULL, establecerSocketClientes, NULL)) {
        perror("Error al crear thread de clientes");
        exit(1);
    }

    if (pthread_join(tcli, NULL)) {
        printf("\n ERROR joining thread");
        exit(1);
    }

}

void establecerSecundario() {

    pthread_t trecepcion_lista, tr_entregar;
    /*ESTO ES PARTE DE LA DE LOS SERVIDORES SECUNDARIOS*/

    mensajes_recibidos = 0;
    ultimos_recibidos[0] = 0;

    //crear colas fifo
    sprintf(myfifo, "%d", puerto);
    mkfifo(myfifo, 0666);
    fifo = open(myfifo, 0666);

    //crear thread para leer de cola FIFO
    /*if (pthread_create(&tr_entregar, NULL, r_entregar, NULL)) {
        perror("    Error al crear thread de recepcion de lista de servidores");
        exit(1);
    }*/

    //crear los sockets necesarios
    if ((sock_comunicacion[0] = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("    Error al crear el socket secundario");
        exit(1);
    }

    if ((sock_secundario_listas = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("    Error al crear el socket secundario para listas");
        exit(1);
    }

    //conectar con primario para la transmision de mensajes
    if (connect(sock_comunicacion[0], (struct sockaddr *) &primario_dir, sizeof (primario_dir)) < 0) {
        perror("    Error al conectarse con el servidor primario con socket de recepcion de mensajes");
        exit(1);
    } else {
        printf("    El secundario se ha conectado al primario con exito con socket de recepcion de mensajes\n");
    }

    if (write(sock_comunicacion[0], &puerto, sizeof (puerto)) < 0) {
        perror("    Error al enviar el puerto establecido en el secundario al servidor primario\n");
        exit(1);
    } else {
        printf("    El secundario ha enviado el puerto al primario con exito\n");
    }

    //conectar con el primario para la transmision de listas
    if (connect(sock_secundario_listas, (struct sockaddr *) &primario_dir, sizeof (primario_dir)) < 0) {
        perror("    Error al conectarse con el servidor primario con socket de recepcion de listas");
        exit(1);
    } else {
        printf("    El secundario se ha conectado al primario con exito con socket de recepcion de listas\n");
    }

    //crear thread para recibir listas actualizadas de los servidores desde el primario
    if (pthread_create(&trecepcion_lista, NULL, recibirListaDeServidores, NULL)) {
        perror("    Error al crear thread de recepcion de lista de servidores");
        exit(1);
    }

    //crear thread para recibir mensajes desde el primario
    int *arg = malloc(sizeof (*arg));
    *arg = sock_comunicacion[0];
    if (pthread_create(&trecibir[0], NULL, recibir, arg)) {
        perror("    Error al crear thread de recibir");
        exit(1);
    }

    sesioa(0);

    if (pthread_join(trecepcion_lista, NULL)) {
        perror("\n ERROR joining thread \n");
        exit(1);
    }

}

void * establecerSocketClientes(void * a) {
    printf("\n**** Estableciendo thread de escucha para clientes, establecido solo por el servidor primario ****\n");
    // No se hara nada al recibir SIG_CHLD. De esta forma los prozesos hijo terminados, no se quedarán tipo zombie
    signal(SIGCHLD, SIG_IGN);
    while (1) {
        // Onartu konexio eskaera eta sortu elkarrizketa socketa.
        // Aceptar petición de conexión y crear socket
        if ((elkarrizketa = accept(sock_clientes, NULL, NULL)) < 0) {
            perror("    Error al conectarse");
            exit(1);
        }

        // Crear procesos hijo para conectar con cliente
        switch (fork()) {
            case 0:
                //esto es lo que ya habia
                close(sock_clientes);
                sesioa(elkarrizketa);
                close(elkarrizketa);
                exit(0);

                //esto es lo que se ha escrito
                /*close(sock_clientes);
                //De alguna forma llamar a secundarios para que hagan un fork
                //llamar a thread recibir de cliente con parametro elkarrizketa
                int *arg = malloc(sizeof (*arg));
                *arg = elkarrizketa;
                if (pthread_create(&trecibir[0], NULL, recibirDelCliente, arg)) {
                    perror("    Error al crear thread de recibir del cliente");
                    exit(1);
                }
                sesioa(0);
                close(elkarrizketa);
                exit(0);*/
            default:
                close(elkarrizketa);
        }
    }
}

void * establecerSocketServidores(void * a) {
    printf("\n**** Estableciendo socket de escucha para servidores, establecido por todos los servidores ****\n");
    // No se hara nada al recibir SIG_CHLD. De esta forma los prozesos hijo terminados, no se quedarán tipo zombie
    signal(SIGCHLD, SIG_IGN);
    int puerto_helper;
    while (1) {

        if (primario == 1) {
            // Aceptar petición de conexión y crear socket
            if ((sock_comunicacion[contador_servidores] = accept(sock_servidores, (struct sockaddr *) &nuevosecundario_dir, &nuevosecundario_size)) < 0) {
                perror("    Error al conectarse");
                exit(1);
            } else {
                printf("\n**** Nueva conexion recibida ****\n");
            }

            //Crear thread recibir en el socket que ha recibido la peticion
            int *arg = malloc(sizeof (*arg));
            *arg = sock_comunicacion[contador_servidores];
            if (pthread_create(&trecibir[contador_servidores], NULL, recibir, arg)) {
                perror("    Error al crear thread de recibir");
                exit(1);
            }

            //leer puerto del secundario
            if (read(sock_comunicacion[contador_servidores], &puerto_helper, sizeof (puerto_helper)) < 0) {
                perror("    Error al leer el puerto desde secundario");
                exit(1);
            } else {
                printf("    El puerto de comunicacion que utiliza el servidor secundario %d, es el %d\n", contador_servidores, puerto_helper);
            }

            // Se establece otra comunicacion para las listas
            if ((sock_listas[contador_servidores] = accept(sock_servidores, (struct sockaddr *) &nuevosecundario_dir, &nuevosecundario_size)) < 0) {
                perror("    Error al conectarse");
                exit(1);
            } else {
                printf("    Nueva conexion recibida, para la actualización de listas.\n");
            }

            join(nuevosecundario_dir, puerto_helper);
            contador_servidores += 1;
            enviarListaDeServidores();

        } else {
            // Aceptar petición de conexión y crear socket
            int x;
            x = accept(sock_servidores, (struct sockaddr *) &nuevosecundario_dir, &nuevosecundario_size);
            if (x < 0) {
                perror("    Error al conectarse");
                exit(1);
            } else {
                sock_comunicacion[contador_servidores - 1] = x;
                printf("\n**** Nueva conexion recibida ****\n");
            }

            //Crear thread recibir en el socket que ha recibido la peticion
            int *arg = malloc(sizeof (*arg));
            *arg = sock_comunicacion[contador_servidores - 1];
            if (pthread_create(&trecibir[contador_servidores - 1], NULL, recibir, arg)) {
                perror("    Error al crear thread de recibir");
                exit(1);
            }
        }
        imprimirListaSecundarios();
        imprimirListaSockets();
    }
}

void join(struct sockaddr_in dir, int puerto_escucha) {
    printf("\n**** Se está actualizando la lista de servidores: JOIN ****\n");
    printf("    Contador servidores secundarios: %d\n", contador_servidores);
    char ip[INET_ADDRSTRLEN];
    servidores[contador_servidores].id = contador_servidores;
    servidores[contador_servidores].dir = dir;
    servidores[contador_servidores].dir.sin_port = htons(puerto_escucha);
    int i;
    for (i = 0; i <= contador_servidores; i++) {
        inet_ntop(AF_INET, &(servidores[i].dir.sin_addr), ip, INET_ADDRSTRLEN);
        printf("    Direccion IP del servidor %d: %s:%d\n", i, ip, ntohs(servidores[i].dir.sin_port));
        printf("    Identificador del socket de %s: %d\n", ip, sock_comunicacion[i]);
    }
}

void leave(int id, struct sockaddr_in dir) {
    printf("\n**** Se está actualizando la lista de servidores: LEAVE ****\n");
    int i = 0;
    int found = 0;
    for (i = 0; i <= contador_servidores; i++) {
        if (found == 0) {
            if (servidores[i].id == id) {
                printf("    Elimnando secundario de la lista\n");
                servidores[i] = servidores[i + 1];
                sock_comunicacion[i] = sock_comunicacion[i + 1];
                sock_listas[i] = sock_listas[i + 1];
                found = 1;
            }
        } else {
            printf("    Recolocando elementos del array\n");
            servidores[i] = servidores[i + 1];
            sock_comunicacion[i] = sock_comunicacion[i + 1];
            sock_listas[i] = sock_listas[i + 1];
        }
    }
    contador_servidores -= 1;
    printf("    Contador servidores secundarios: %d\n", contador_servidores);
}

void enviarListaDeServidores() {
    printf("\n**** Se está enviando la lista de servidores ****\n");
    int i = 0;
    for (i = 0; i < contador_servidores; i++) {
        if (write(sock_listas[i], &contador_servidores, sizeof (contador_servidores)) < 0) {
            perror("    Error al enviar el contador de servidores\n");
            exit(1);
        } else {
            printf("    Enviando contador de servidores (%d) a servidor número %d\n", contador_servidores, i);
        }

        if (write(sock_listas[i], &servidores, sizeof (servidores)) < 0) {
            perror("    Error al enviar la lista de servidores\n");
            exit(1);
        } else {
            printf("    Enviando lista a servidor número %d\n", i);
        }
    }
}

void * recibirListaDeServidores(void * a) {
    printf("\n**** Recibiendo lista de servidores ****\n");
    char ip[INET_ADDRSTRLEN];
    struct servidor servidores_helper[3];
    while (1) {
        while (1) {
            //recibir contador de servidores
            int x;
            x = read(sock_secundario_listas, &contador_servidores, sizeof (contador_servidores));
            if (x < 0) {
                perror("    Error al recibir contador de servidores, el error es en el primario\n");
                exit(1);
            } else if (x == 0) {
                printf("    Error de conexión: se debe ejecutar funcion leave: socket_listas\n");
                //leave
                close(sock_secundario_listas);
                break;
            } else {
                printf("    Contador de servidores recibida y actualizada: %d\n", contador_servidores);
            }

            //recibir lista de direcciones
            x = read(sock_secundario_listas, &servidores_helper, sizeof (servidores_helper));
            if (x < 0) {
                perror("    Error al recibir lista de servidores, el error es en el primario\n");
                exit(1);
            } else if (x == 0) {
                printf("    Error de conexión: se debe ejecutar funcion leave: socket_listas\n");
                close(sock_secundario_listas);
                break;
            } else {
                int i;
                for (i = 0; i < contador_servidores; i++) {
                    servidores[i] = servidores_helper[i];
                    inet_ntop(AF_INET, &(servidores[i].dir.sin_addr), ip, INET_ADDRSTRLEN);
                    printf("    Direccion IP del servidor %d: %s:%d\n", i, ip, ntohs(servidores[i].dir.sin_port));
                }
                printf("    Lista de direcciones nueva recibida y actualizada\n");
            }

            //si es el ultimo secundario en conectarse, hacer peticiones de conexion al resto de secundarios
            if (nuevo_secundario == 1) {
                nuevo_secundario = 0;
                int i;
                // empieza desde 1 porque el 0 es el socket del servidor primario, y ya está conectado
                // termina en contador_servidores - 1, porque la ultima direccion es el del nuevo secundario, es decir, uno mismo si nuevo_secundario == 0
                // se podría mejorar, pero no he conseguido comparar direcciones sockaddr_in...
                for (i = 1; i < contador_servidores; i++) {
                    printf("    Haciendo peticiones de comunicacion a servidores secundarios\n");
                    if ((sock_comunicacion[i] = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
                        perror("    Error al crear el socket de comunicacion entre secundarios");
                        exit(1);
                    }
                    if (connect(sock_comunicacion[i], (struct sockaddr *) &servidores[i - 1].dir, sizeof (servidores[i - 1].dir)) < 0) {
                        perror("    Error al conectarse con el servidor secundario");
                        exit(1);
                    } else {
                        printf("    El servidor se ha conectado al secundario numero %d con exito\n", i - 1);
                    }
                    //crear un thread para recibir mensajes desde el socket correspondiente
                    int *arg = malloc(sizeof (*arg));
                    *arg = sock_comunicacion[i];
                    if (pthread_create(&trecibir[i], NULL, recibir, arg)) {
                        perror("Error al crear thread de recepcion de mensajes desde secundario");
                        exit(1);
                    }
                }
                imprimirListaSockets();
            }
        }
    }
}

/*
 * Funtzio honetan kodetzen da bezeroarekin komunikatu behar den prozesu umeak egin beharrekoa, aplikazio protokoloak zehaztu bezala.
 * Parametro gisa elkarrizketa socketa pasa behar zaio.
 */
void sesioa(int s) {
    char buf[MAX_BUF], file_path[MAX_BUF], file_name[MAX_BUF], dir_path[MAX_BUF], dir_name[MAX_BUF];
    int n, erabiltzaile, komando, error;
    FILE *fp;
    struct stat file_info;
    unsigned long file_size, irakurrita;
    char * sep;

    // Zehaztu uneko egoera bezala hasierako egoera.
    egoera = ST_INIT;

    while (1) {

        // primario o secundario aqui, fifo o readline
        memset(&buf, 0, sizeof (buf));
        if (primario == 1) {
            if ((n = readline(s, buf, MAX_BUF)) <= 0)
                return;
            difundir(buf);
        } else {
            //leer desde secundario mediante colas fifo.
            read(fifo, &buf, sizeof (buf));
            printf("\n**** R_entregando mensaje: R_ENTREGAR ****\n");
            printf("    Se está entregando mensaje %s\n", buf);
        }

        // Aztertu jasotako komandoa ezaguna den ala ez.
        if ((komando = bilatu_substring(buf, KOMANDOAK)) < 0) {
            ustegabekoa(s);
            continue;
        }

        // Jasotako komandoaren arabera egin beharrekoa egin.
        switch (komando) {
            case COM_USER:
                if (egoera != ST_INIT) // Egiaztatu esperotako egoeran jaso dela komandoa.
                {
                    ustegabekoa(s);
                    continue;
                }
                buf[n - 2] = 0; // Lerro bukaera, edo "End Of Line" (EOL), ezabatzen du.
                // Baliozko erabiltzaile bat den egiaztatu.
                if ((erabiltzaile = bilatu_string(buf + 4, erab_zer)) < 0) {
                    write(s, "ER2\r\n", 5);
                } else {
                    write(s, "OK\r\n", 4);
                    egoera = ST_AUTH;
                }
                break;
            case COM_PASS:
                if (egoera != ST_AUTH) // Egiaztatu esperotako egoeran jaso dela komandoa.
                {
                    ustegabekoa(s);
                    continue;
                }
                buf[n - 2] = 0; // EOL ezabatu.
                // Pasahitza zuzena dela egiaztatu.
                if (erabiltzaile == 0 || !strcmp(pass_zer[erabiltzaile], buf + 4)) {
                    write(s, "OK\r\n", 4);
                    egoera = ST_MAIN;
                } else {
                    write(s, "ER3\r\n", 5);
                    egoera = ST_INIT;
                }
                break;
            case COM_LIST:
                if (n > 6 || egoera != ST_MAIN) // Egiaztatu esperotako egoeran jaso dela komandoa eta ez dela parametrorik jaso.
                {
                    ustegabekoa(s);
                    continue;
                }
                if (bidali_zerrenda(s) < 0)
                    write(s, "ER4\r\n", 5);
                break;
            case COM_DOWN:
                if (egoera != ST_MAIN) // Egiaztatu esperotako egoeran jaso dela komandoa.
                {
                    ustegabekoa(s);
                    continue;
                }
                buf[n - 2] = 0; // EOL ezabatu.
                sprintf(file_path, "%s/%s", FILES_PATH, buf + 4); // Fitxategiak dauden karpeta eta fitxategiaren izena kateatu.
                if (stat(file_path, &file_info) < 0) // Lortu fitxategiari buruzko informazioa.
                {
                    write(s, "ER5\r\n", 5);
                } else {
                    sprintf(buf, "OK%ld\r\n", file_info.st_size);
                    write(s, buf, strlen(buf));
                    egoera = ST_DOWN;
                }
                break;
            case COM_DOW2:
                if (n > 6 || egoera != ST_DOWN) // Egiaztatu esperotako egoeran jaso dela komandoa eta ez dela parametrorik jaso.
                {
                    ustegabekoa(s);
                    continue;
                }
                egoera = ST_MAIN;
                // Fitxategia ireki.
                if ((fp = fopen(file_path, "r")) == NULL) {
                    write(s, "ER6\r\n", 5);
                } else {
                    // Fitxategiaren lehenengo zatia irakurri eta errore bat gertatu bada bidali errore kodea.
                    if ((n = fread(buf, 1, MAX_BUF, fp)) < MAX_BUF && ferror(fp) != 0) {
                        write(s, "ER6\r\n", 5);
                    } else {
                        write(s, "OK\r\n", 4);
                        // Bidali fitxategiaren zatiak.
                        do {
                            write(s, buf, n);
                        } while ((n = fread(buf, 1, MAX_BUF, fp)) == MAX_BUF);
                        if (ferror(fp) != 0) {
                            close(s);
                            return;
                        } else if (n > 0)
                            write(s, buf, n); // Bidali azkeneko zatia.
                    }
                    fclose(fp);
                }
                break;
            case COM_UPLO:
                if (egoera != ST_MAIN) // Egiaztatu esperotako egoeran jaso dela komandoa.
                {
                    ustegabekoa(s);
                    continue;
                }
                buf[n - 2] = 0; // EOL kendu.
                // Mezuak dauzkan bi zatiak (fitxategi izena eta tamaina) erauzi.
                if ((sep = strchr(buf, '?')) == NULL) {
                    ustegabekoa(s);
                    continue;
                }
                *sep = 0;
                sep++;
                strcpy(file_name, buf + 4); // Fitxategi izena lortu.
                file_size = atoi(sep); // Fitxategi tamaina lortu.
                sprintf(file_path, "%s/%s", FILES_PATH, file_name); // Fitxategiak dauden karpeta eta fitxategiaren izena kateatu.
                if (file_size > MAX_UPLOAD_SIZE) // Fitxategi tamainak maximoa gainditzen ez duela egiaztatu.
                {
                    write(s, "ER7\r\n", 5);
                    continue;
                }
                if (toki_librea() < file_size + SPACE_MARGIN) // Mantendu beti toki libre minimo bat diskoan.
                {
                    write(s, "ER8\r\n", 5);
                    continue;
                }
                write(s, "OK\r\n", 4);
                egoera = ST_UP;
                break;
            case COM_UPL2:
                //hay que meter el thread aqui?? es decir, recibir tiene que ir aqui, en com_dele y com_MKDR y com_ddl?
                if (n > 6 || egoera != ST_UP) // Egiaztatu esperotako egoeran jaso dela komandoa eta ez dela parametrorik jaso.
                {
                    ustegabekoa(s);
                    continue;
                }
                egoera = ST_MAIN;
                irakurrita = 0L;
                fp = fopen(file_path, "w"); // Sortu fitxategi berria diskoan.
                error = (fp == NULL);
                while (irakurrita < file_size) {
                    // Jaso fitxategi zati bat.
                    if ((n = read(s, buf, MAX_BUF)) <= 0) {
                        close(s);
                        return;
                    }
                    // Ez bada errorerik izan gorde fitxategi zatia diskoan.
                    // Errorerik gertatuz gero segi fitxategi zatiak jasotzen.
                    // Fitxategi osoa jasotzeak alferrikako trafikoa sortzen du, baina aplikazio protokoloak ez du beste aukerarik ematen.
                    if (!error) {
                        if (fwrite(buf, 1, n, fp) < n) {
                            fclose(fp);
                            unlink(file_path);
                            error = 1;
                        }
                    }
                    irakurrita += n;
                }
                if (!error) {
                    fclose(fp);
                    write(s, "OK\r\n", 4);
                } else
                    write(s, "ER9\r\n", 6);
                break;

            case COM_DELE:
                if (egoera != ST_MAIN) // Egiaztatu esperotako egoeran jaso dela komandoa.
                {
                    ustegabekoa(s);
                    continue;
                }
                buf[n - 2] = 0; // EOL ezabatu.
                sprintf(file_path, "%s/%s", FILES_PATH, buf + 4); // Fitxategiak dauden karpeta eta fitxategiaren izena kateatu.
                if (unlink(file_path) < 0) // Ezabatu fitxategia.
                    write(s, "ER10\r\n", 6);
                else
                    write(s, "OK\r\n", 4);
                break;

            case COM_MKDR:
                if (egoera != ST_MAIN) {
                    ustegabekoa(s);
                    continue;
                }
                buf[n - 2] = 0; // EOL ezabatu.
                strcpy(dir_name, buf + 4); // Obtener el nombre del directorio.
                sprintf(dir_path, "%s/%s", FILES_PATH, dir_name); // Conseguir el PATH del nuevo directorio.
                error = mkdir(dir_path, ACCESSPERMS); // Crear el nuevo directorio.
                if (error < 0)
                    write(s, "ER11\r\n", 6);
                else
                    write(s, "OK\r\n", 4);
                break;

            case COM_DDEL:
                if (egoera != ST_MAIN) {
                    ustegabekoa(s);
                    continue;
                }
                buf[n - 2] = 0; // EOL ezabatu.
                strcpy(dir_name, buf + 4); // Obtener el nombre del directorio.
                sprintf(dir_path, "%s/%s", FILES_PATH, dir_name); // Conseguir el PATH del nuevo directorio.
                error = rmdir(dir_path); // barr el directorio.
                if (error < 0)
                    write(s, "ER11\r\n", 6);
                else
                    write(s, "OK\r\n", 4);
                break;

            case COM_EXIT:
                if (n > 6) // Egiaztatu ez dela parametrorik jaso.
                {
                    ustegabekoa(s);
                    continue;
                }
                write(s, "OK\r\n", 4);
                return;
        }
    }
}

/*
 * Telneteko lerro jauzi estandar bat ("\r\n") aurkitu arte datuak irakurtzen ditu stream batetik.
 * Erraztasunagatik lerro jauzia irakurketa bakoitzeko azken bi bytetan soilik bilatzen da.
 * Dena ondo joanez gero irakurritako karaktere kopurua itzuliko du.
 * Fluxua amaituz gero ezer irakurri gabe 0 itzuliko du.
 * Zerbait irakurri ondoren fluxua amaituz gero ("\r\n" aurkitu gabe) -1 itzuliko du.
 * 'tam' parametroan adierazitako karaktere kopurua irakurriz gero "\r\n" aurkitu gabe -2 itzuliko du.
 * Beste edozein error gertatu ezkero -3 itzuliko du.
 */


int readline(int stream, char *buf, int tam) {
    char c;
    int guztira = 0;
    int cr = 0;

    while (guztira < tam) {
        int n = read(stream, &c, 1);
        if (n == 0) {
            if (guztira == 0)
                return 0;
            else
                return -1;
        }
        if (n < 0)
            return -3;
        buf[guztira++] = c;
        if (cr && c == '\n') {
            return guztira;
        } else if (c == '\r')
            cr = 1;
        else
            cr = 0;
    }
    return -2;
}

void * recibirDelCliente(void *a) {
    char buf[MAX_BUF];
    int socket = *((int *) a);
    int n;
    while (1) {
        //leer del socket creado por el subproceso
        n = readline(socket, buf, MAX_BUF);
        //escibir en cola fifo
        write(fifo, buf, sizeof(MAX_BUF));
        //difundir
        difundir(buf);
    }
}

void * r_entregar(void * a) {
    struct mensaje mensaje_a_entregar;
    while (1) {
        read(fifo, &mensaje_a_entregar.valor, sizeof (mensaje_a_entregar.valor));
        printf("\n**** R_entregando mensaje: R_ENTREGAR ****\n");
        printf("    Se está entregando mensaje %s\n", mensaje_a_entregar.valor);
        //llamar a funcion sesion
        //sesioa(0);
    }

}

void enviar(int socket, struct mensaje msg) {
    printf("\n**** Enviando mensaje al socket %d: ENVIAR ****\n", socket);
    if (write(socket, &msg, sizeof (msg)) < 0) {
        perror("    Error al enviar mensaje al difundir");
    } else {
        printf("    Se ha enviado el mensaje: %d.\n", msg.cont);
        printf("    El mensaje enviado es: %s\n", msg.valor);
    }
}

void difundir(char* msg) {
    printf("\n**** Difundiendo mensaje: DIFUNDIR ****\n");
    struct mensaje mensaje;
    mensaje.cont = contador_mensajes;
    strcpy(mensaje.valor, msg);
    printf("    Se está difundiendo mensaje: %d.\n", mensaje.cont);
    printf("    El mensaje difundido es: %s\n", mensaje.valor);

    int i;


    if (primario == 1) {
        mensajes_recibidos = (mensajes_recibidos + 1) % 10;
        ultimos_recibidos[mensajes_recibidos] = mensaje.cont;
        for (i = 0; i < contador_servidores; i++) {
            enviar(sock_comunicacion[i], mensaje);
        }
        contador_mensajes++;
        imprimirListaMensajes();
    } else {
        for (i = 1; i < contador_servidores; i++) {
            enviar(sock_comunicacion[i], mensaje);
        }
    }
}

void imprimirListaMensajes() {
    printf("\n**** Imprimir lista de mensajes ****\n");
    int i;
    printf("    [");
    for (i = 0; i < 9; i++) {
        printf("%d,", ultimos_recibidos[i]);
    }
    printf("%d", ultimos_recibidos[9]);
    printf("]\n");
}

void imprimirListaSecundarios() {
    printf("\n**** Imprimir lista de secundarios ****\n");
    int i, puerto_helper;
    char ip[INET_ADDRSTRLEN];
    for (i = 0; i < 3; i++) {
        inet_ntop(AF_INET, &(servidores[i].dir.sin_addr), ip, INET_ADDRSTRLEN);
        puerto_helper = ntohs(servidores[i].dir.sin_port);
        printf("    Posicion %d: %s:%d\n", i, ip, puerto_helper);
    }
}

void imprimirListaSockets() {
    printf("\n**** Imprimir lista de sockets ****\n");
    int i;
    for (i = 0; i < 3; i++) {
        printf("    Posicion %d: %d\n", i, sock_comunicacion[i]);
    }
}

void * recibir(void *a) {
    int socket = *((int *) a);
    //a veces ni siquiera se pasa bien el identificador del socket
    printf("\n**** Estableciendo thread de lectura de mensajes en %d: RECIBIR ****\n", socket);
    int x;
    struct mensaje mensaje_recibido;

    while (1) {
        x = read(socket, &mensaje_recibido, sizeof (mensaje_recibido));
        if (x < 0) {
            perror("    Error al recibir mensaje\n");
            exit(1);
        } else if (x == 0) {
            printf("    Error de conexión: se debe ejecutar funcion leave: socket_recibir %d\n", socket);
            break;
        } else {
            printf("\n**** Recibiendo mensaje %d desde socket %d: RECIBIR ****\n", mensaje_recibido.cont, socket);
            if (chequearMensaje(mensaje_recibido) < 0) {
                printf("    El mensaje %d está en la lista de mensajes recibidos \n", mensaje_recibido.cont);
            } else {
                mensajes_recibidos = (mensajes_recibidos + 1) % 10; //aplicamos el modulo para que siempre tenga un valor entre 0 y 9
                ultimos_recibidos[mensajes_recibidos] = mensaje_recibido.cont;
                //difusion fiable entre secundarios, si el mensaje no ha sido recibido, se mete en la cola fifo, se difunde y se trata la peticion
                contador_mensajes = mensaje_recibido.cont;
                printf("    El mensaje %d se meterá en una cola fifo \n", mensaje_recibido.cont);
                write(fifo, &mensaje_recibido.valor, sizeof (mensaje_recibido.valor));
                difundir(mensaje_recibido.valor);

                //añadimos el identificador del mensaje recibido a la lista de los ultimos 10 recibidos.

                memset(&mensaje_recibido, 0, sizeof (mensaje_recibido));
            }
            imprimirListaMensajes();
        }
    }
}

int chequearMensaje(struct mensaje msg) {
    int aux;
    for (aux = 0; aux < 10; aux++) {
        if (msg.cont == ultimos_recibidos[aux]) {
            return -1;
        }
    }
    return 0;
}

/*
 * 'string' parametroko karaktere katea bilatzen du 'string_zerr' parametroan. 'string_zerr' bektoreko azkeneko elementua NULL izan behar da.
 * 'string' katearen lehen agerpenaren indizea itzuliko du, edo balio negatibo bat ez bada agerpenik aurkitu.
 */

int bilatu_string(char *string, char **string_zerr) {
    int i = 0;
    while (string_zerr[i] != NULL) {
        if (!strcmp(string, string_zerr[i]))
            return i;
        i++;
    }
    return -1;
}

/*
 * 'string' parametroa 'string_zerr' bektoreko karaktere kateetako batetik hasten den egiaztatzen du. 'string_zerr' bektoreko azkeneko elementua NULL izan behar da.
 * 'string' parametro karaktere katearen hasierarekin bat egiten duen 'string_zerr' bektoreko lehenego karaktere katearen indizea itzuliko du. Ez bada bat-egiterik egon balio negatibo bat itzuliko du.
 */
int bilatu_substring(char *string, char **string_zerr) {
    int i = 0;
    while (string_zerr[i] != NULL) {
        if (!strncmp(string, string_zerr[i], strlen(string_zerr[i])))
            return i;
        i++;
    }
    return -1;
}

/*
 * Ustekabeko zerbait gertatzen denean egin beharrekoa: dagokion errorea bidali bezeroari eta egoera eguneratu.
 */
void ustegabekoa(int s) {
    write(s, "ER1\r\n", 5);
    if (egoera == ST_AUTH) {
        egoera = ST_INIT;
    } else if (egoera == ST_DOWN || egoera == ST_UP) {
        egoera = ST_MAIN;
    }
}

/*
 * 's' streamaren bitartez eskuragarri dauden fitxategien zerrenda bidaltzen du. Ez bada posible fitxategiak dauden katalogoa aztertzea balio negatibo bat itzuliko du eta bestela, zerrendako fitxategi kopurua.
 */
int bidali_zerrenda(int s) {
    struct dirent ** fitxategi_izenak;
    int i, fitxategi_kop;
    long fitxategi_tam;
    char buf[MAX_BUF], fitxategi_path[MAX_BUF];
    struct stat file_info;

    fitxategi_kop = scandir(FILES_PATH, &fitxategi_izenak, ez_ezkutua, alphasort);
    if (fitxategi_kop < 0)
        return -1;
    if (write(s, "OK\r\n", 4) < 4)
        return -1;

    for (i = 0; i < fitxategi_kop; i++) {
        sprintf(fitxategi_path, "%s/%s", FILES_PATH, fitxategi_izenak[i]->d_name);
        if (stat(fitxategi_path, &file_info) < 0)
            fitxategi_tam = -1L;
        else
            fitxategi_tam = file_info.st_size;
        sprintf(buf, "%s?%ld\r\n", fitxategi_izenak[i]->d_name, fitxategi_tam);
        if (write(s, buf, strlen(buf)) < strlen(buf))
            return i;
    }
    write(s, "\r\n", 2);
    return fitxategi_kop;
}

/*
 * Eskuragarri dauden fitxategiak kokatuta dauden diskoan eskuragarri dagoen tokia itzultzen du bytetan. Erroreren bat gertatuz gero balio negatibo bat itzuliko du.
 */
unsigned long toki_librea() {
    struct statvfs info;

    if (statvfs(FILES_PATH, &info) < 0)
        return -1;
    return info.f_bsize * info.f_bavail;
}

/*
 * Katalogo bat eskaneatzerakoan fitxategi ezkutuak kontuan ez hartzeko eginiko iragazkia. Fitxategia '.' karaktereaz hasten bada 0 itzuliko du eta bestela 1. Kontuan izan fitxategi ezkutuak adierazteko modu hau UNIX eta antzeko sistemetan erabiltzen dela, beraz Windows sistemetan ez du esperotako portaera izango.
 */
int ez_ezkutua(const struct dirent *entry) {
    if (entry->d_name[0] == '.')
        return (0);
    else
        return (1);
}

/* comparador de direcciones */

/*
int     sock_addr_cmp_addr(const struct sockaddr * sa, const struct sockaddr * sb)
{
    if (sa->sa_family != sb->sa_family)
        return (sa->sa_family - sb->sa_family);

    if (sa->sa_family == AF_INET) {
        return (SOCK_ADDR_IN_ADDR(sa).s_addr - SOCK_ADDR_IN_ADDR(sb).s_addr);
#ifdef HAS_IPV6
    } else if (sa->sa_family == AF_INET6) {
        return (memcmp((char *) &(SOCK_ADDR_IN6_ADDR(sa)),
                       (char *) &(SOCK_ADDR_IN6_ADDR(sb)),
                       sizeof(SOCK_ADDR_IN6_ADDR(sa))));
#endif
    } else {
        msg_panic("sock_addr_cmp_addr: unsupported address family %d",
                  sa->sa_family);
    }
}
 */
/* comparador de puertos */
/*
int sock_addr_cmp_port(struct sockaddr * sa, struct sockaddr * sb) {
    if (sa->sa_family != sb->sa_family)
        return (sa->sa_family - sb->sa_family);

    if (sa->sa_family == AF_INET) {
        return (SOCK_ADDR_IN_PORT(sa) - SOCK_ADDR_IN_PORT(sb));
    } else {
        msg_panic("Esta familia no está soportada: %d", sa->sa_family);
    }
}


//int sockaddr_cmp(struct sockaddr *x, struct sockaddr *y)
int sock_addr_cmp_addr(struct sockaddr *x, struct sockaddr *y)
{
#define CMP(a, b) if (a != b) return a < b ? -1 : 1

    CMP(x->sa_family, y->sa_family);

    if (x->sa_family == AF_UNIX) {
        struct sockaddr_un *xun = (void*)x, *yun = (void*)y;
        int r = strcmp(xun->sun_path, yun->sun_path);
        if (r != 0)
            return r;
    } else if (x->sa_family == AF_INET) {
        struct sockaddr_in *xin = (void*)x, *yin = (void*)y;
        CMP(ntohl(xin->sin_addr.s_addr), ntohl(yin->sin_addr.s_addr));
        CMP(ntohs(xin->sin_port), ntohs(yin->sin_port));
    } else if (x->sa_family == AF_INET6) {
        struct sockaddr_in6 *xin6 = (void*)x, *yin6 = (void*)y;
        int r = memcmp(xin6->sin6_addr.s6_addr, yin6->sin6_addr.s6_addr, sizeof(xin6->sin6_addr.s6_addr));
        if (r != 0)
            return r;
        CMP(ntohs(xin6->sin6_port), ntohs(yin6->sin6_port));
        CMP(xin6->sin6_flowinfo, yin6->sin6_flowinfo);
        CMP(xin6->sin6_scope_id, yin6->sin6_scope_id);
    } else {
        assert(!"unknown sa_family");
    }

#undef CMP
    return 0;
}

 */