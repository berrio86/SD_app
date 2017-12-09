#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <sys/wait.h>
#include <signal.h>
#include <sys/stat.h>
#include <dirent.h>
#include <sys/statvfs.h>
#include <pthread.h>
#include <time.h>

#include "servidor.h"


int sock_clientes, elkarrizketa, sock_servidores, sock_comunicacion, sock_secundario;
struct sockaddr_in servidor_dir, primario_dir, clientes_dir;
socklen_t helb_tam;
int primario, puerto;
struct sockaddr_in servidores[3];
pthread_t tserv, tcli;
char buf[MAX_BUF];

int main(int argc, char *argv[]) {

    primario = 0;

    if (argc == 1) {
        printf("Estableciendo servidor primario.\n");
        primario = 1;
        puerto = PORT_SERVIDORES;
    } else if (argc == 2) {
        //Para establecer servidores secundarios en diferentes ordenadores
        printf("Estableciendo servidor secundario.\n");
        primario = 0;
        memset(&primario_dir, 0, sizeof (primario_dir));
        primario_dir.sin_family = AF_INET;
        primario_dir.sin_addr.s_addr = htonl(atoi(argv[1]));
        primario_dir.sin_port = htons(PORT_SERVIDORES);
        puerto = PORT_SERVIDORES;

    } else if (argc == 3) {
        //Para establecer servidores secundarios en el mismo ordenadores
        printf("Estableciendo servidor secundario.\n");
        primario = 0;
        puerto = atoi(argv[2]);
        memset(&primario_dir, 0, sizeof (primario_dir));
        primario_dir.sin_family = AF_INET;
        primario_dir.sin_addr.s_addr = htonl(INADDR_ANY);
        primario_dir.sin_port = htons(PORT_SERVIDORES);

    } else {
        printf("Error: El número de parámetros es incorrecto!.\n");
        printf("- Primario (0 parámetros)\n");
        printf("- Secundario(1 parámetros): IP primario \n");
        printf("- Secundario(2 parámetros): cualquierCosa + puerto de secundario \n");
        exit(1);
    }


    //Crear socket para servidores
    if ((sock_servidores = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("Error al crear el socket");
        exit(1);
    }

    memset(&servidor_dir, 0, sizeof (servidor_dir));
    servidor_dir.sin_family = AF_INET;
    servidor_dir.sin_addr.s_addr = htonl(INADDR_ANY);
    servidor_dir.sin_port = htons(puerto);

    // Asignar dirección al socket	
    if (bind(sock_servidores, (struct sockaddr *) &servidor_dir, sizeof (servidor_dir)) < 0) {
        perror("Error al asignar una direccion al socket");
        exit(1);
    }

    if (primario == 1) {
        establecerPrimario();
    } else {
        establecerSecundario();
    }

}

/*IÑAKIK GEHITUTAKO KODEA*/

void establecerPrimario() {
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


    /*ESTO ES PARTE DE LA COMUNICACION CON LOS SERVIDORES*/
    /* SE ESTABLECE EL PUERTO 6013 COMO PUERTO DE ESCUCHA SOLO PARA SERVIDORES*/
    // Establecer socket de escucha para servidores
    if (listen(sock_servidores, 5) < 0) {
        perror("Error al establecer socket como socket de escucha");
        exit(1);
    }

    if (pthread_create(&tserv, NULL, establecerSocketServidores, NULL)) {
        perror("Error al crear thread de clientes");
        exit(1);
    }

    if (pthread_join(tserv, NULL)) {
        printf("\n ERROR joining thread");
        exit(1);
    }
    
    if (pthread_join(tcli, NULL)) {
        printf("\n ERROR joining thread");
        exit(1);
    }

}

void establecerSecundario() {
    /*ESTO ES PARTE DE LA DE LOS SERVIDORES SECUNDARIOS*/
    /**/

    if ((sock_secundario = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("Error al crear el socket de secundarios");
        exit(1);
    }

    if (connect(sock_secundario, (struct sockaddr *) &primario_dir, sizeof (primario_dir)) < 0) {
        perror("Error al conectarse con el servidor primario");
        exit(1);
    } else {
        printf("El secundario se ha conectado al primario con exito\n");
    }
    
    while(1){
        //establecer lo que tiene que hacer el secundario
        readline(sock_secundario, buf, MAX_BUF);
    }
}

void * establecerSocketClientes(void * a) {
    printf("El thread de socket de clientes funciona\n");
    // No se hara nada al recibir SIG_CHLD. De esta forma los prozesos hijo terminados, no se quedarán tipo zombie
    signal(SIGCHLD, SIG_IGN);
    while (1) {
        // Onartu konexio eskaera eta sortu elkarrizketa socketa.
        // Aceptar petición de conexión y crear socket
        if ((elkarrizketa = accept(sock_clientes, NULL, NULL)) < 0) {
            perror("Error al conectarse");
            exit(1);
        }

        // Crear procesos hijo para conectar con cliente
        switch (fork()) {
            case 0:
                close(sock_clientes);
                sesioa(elkarrizketa);
                close(elkarrizketa);
                exit(0);
            default:
                close(elkarrizketa);
        }
    }
}


void waitFor (unsigned int secs) {
    unsigned int retTime = time(0) + secs;   // Get finishing time.
    while (time(0) < retTime);               // Loop until it arrives.
}

void * establecerSocketServidores(void * a) {
    printf("El thread de socket de servidores funciona\n");
    // No se hara nada al recibir SIG_CHLD. De esta forma los prozesos hijo terminados, no se quedarán tipo zombie
    signal(SIGCHLD, SIG_IGN);
    while (1) {
        // Onartu konexio eskaera eta sortu elkarrizketa socketa.
        // Aceptar petición de conexión y crear socket
        if ((sock_comunicacion = accept(sock_servidores, NULL, NULL)) < 0) {
            perror("Error al conectarse");
            exit(1);
        }

        // Crear procesos hijo para conectar con otros servidores
        switch (fork()) {
            case 0:
                close(sock_servidores);
                printf("Se ha conectado el socket de escucha del servidor y se ha abierto una nueva\n");
                // si el puerto se queda en una situacion inestable, utilizar fuser -k 6013/tcp en la terminal linux para cerrar el puerto. No deberia
                close(sock_comunicacion);
                exit(0);
                break;
            default:
                close(sock_comunicacion);
        }
    }
}


void enviar(struct sockaddr_in servidor, char msg[]) {
    //implementar función write() para enviar mensajes al resto de servidores.
    printf("Se está enviando mensaje a la ip: %d\n", inet_aton(servidor.sin_addr.s_addr));
    printf("El mensaje enviado es: %s\n", msg);
}

void difundir(char msg[]) {
    printf("Se está difundiendo mensaje.\n");
    int i = 0;
    while (i<sizeof (servidores)) {
        enviar(servidores[i], msg);
        i++;
    }
}

void recibir() {
    mkfifo("FIFOrecibir", 0666);
    int fd = open("FIFOrecibir");
    while (1) {
        /*
        int a = read();
        printf("Se ha recibido un mensaje\n");
        write(fd, &a, 1);
        //llamar a r_entregar mediante cola fifo*/
    }
}

void r_entregar(char msg[]) {
    //entregar mensaje a la aplicación
    printf("Se está entregando mensaje %s\n", msg);
}

int chequearMensaje(char msg[], int cont) {
    //Si el mensaje se ha recibido anteriormente
    /*if (mirar el buffer de mensajes) { 
        return 0;
    } else {
        return 1;
    }*/
}
 
/*IÑAKIK GEHITUTAKO KODEA*/

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
        // Irakurri bezeroak bidalitako mezua.
        if ((n = readline(s, buf, MAX_BUF)) <= 0)
            return;

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
    /*
            Kontuz! Inplementazio hau sinplea da, baina ez da batere eraginkorra.
     */
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
        if (cr && c == '\n')
            return guztira;
        else if (c == '\r')
            cr = 1;
        else
            cr = 0;
    }
    return -2;
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


