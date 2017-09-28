// Prueba de captura de notificaciones de Linux
// Adaptado de http://www.thegeekstuff.com/2010/04/inotify-c-program-example/
// por Alberto Lafuente, Fac. Informática UPV/EHU

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/types.h>
#include <linux/inotify.h>

//incluidos en la otra aplicación
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <sys/stat.h>
#include <unistd.h>

#include "cliente.h"
//incluidos en la otra aplicación

#define EVENT_SIZE  ( sizeof (struct inotify_event) )
//#define EVENT_SIZE  (3*sizeof(uint32_t)+sizeof(int))
#define EVENT_BUF_LEN     ( 1024 * ( EVENT_SIZE + 16 ) )

int main(int argc, char *argv[]) {
    int length, i = 0;
    int fd;
    int wd;
    int wd_cd;
    char buffer[EVENT_BUF_LEN];
    char testigo[1024];
    char directorio[MAX_BUF];
    
    //variables de la otra aplicacion

    char buf[MAX_BUF], param[MAX_BUF];
    char zerbitzaria[MAX_BUF];
    int portua = PORT;

    int sock, n, status, aukera;
    long fitx_tamaina, irakurrita;
    struct stat file_info;
    FILE *fp;
    struct sockaddr_in zerb_helb;
    struct hostent *hp;

    //variables de la otra aplicacion


    // kodigo de la otra aplicacion
    // Parametroak prozesatu.
    switch (argc) {
        case 4:
            portua = atoi(argv[3]);
            printf("%d \n", portua);
        case 3:
            strcpy(zerbitzaria, argv[2]);
            printf("%s \n", zerbitzaria);
            break;
        case 2:
            //strcpy(directorio, argv[1]);
            //strcat(directorio, "/");
            //printf("%s \n", directorio);
        case 1:
            strcpy(zerbitzaria, SERVER);
            strcpy(directorio, FILES_PATH);
            printf("%s \n", directorio);
            break;
        default:
            printf("Uso: %s <servidor> <puerto> <directorio>\n", argv[0]);
            exit(1);
    }

    // Crear socket
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("Error al crear el socket");
        exit(1);
    }

    // Crear direccion de socket del servidor
    zerb_helb.sin_family = AF_INET;
    zerb_helb.sin_port = htons(portua);
    if ((hp = gethostbyname(zerbitzaria)) == NULL) {
        herror("Error al obtener el nombre del servidor");
        exit(1);
    }
    memcpy(&zerb_helb.sin_addr, hp->h_addr, hp->h_length);

    // Conectarse con el servidor
    if (connect(sock, (struct sockaddr *) &zerb_helb, sizeof (zerb_helb)) < 0) {
        perror("Error al conectarse con el servidor");
        exit(1);
    }

    // Mandar nombre y usuario
    int x = 0;
    do {
        printf("Nombre de usuario: ");
        fgets(param, MAX_BUF, stdin);
        param[strlen(param) - 1] = 0;
        sprintf(buf, "%s%s\r\n", KOMANDOAK[COM_USER], param);

        write(sock, buf, strlen(buf));
        readline(sock, buf, MAX_BUF);
        status = parse(buf);
        if (status != 0) {
            fprintf(stderr, "Error: ");
            fprintf(stderr, "%s", ER_MEZUAK[status]);
            continue;
        }

        printf("Password: ");
        fgets(param, MAX_BUF, stdin);
        param[strlen(param) - 1] = 0;
        sprintf(buf, "%s%s\r\n", KOMANDOAK[COM_PASS], param);

        write(sock, buf, strlen(buf));
        readline(sock, buf, MAX_BUF);
        status = parse(buf);
        if (status != 0) {
            fprintf(stderr, "Errorea: ");
            fprintf(stderr, "%s", ER_MEZUAK[status]);
            continue;
        }
        break;
    } while (1);


    //codigo de la otra aplicación
    
    fprintf(stderr, "---Prueba de inotify sobre %s\n", directorio);
    fprintf(stderr, "---Notifica crear/borrar ficheros/directorios sobre %s\n", directorio);
    fprintf(stderr, "---%s debe exixtir!\n", directorio);
    fprintf(stderr, "---Para salir, borrar %s/inotify.example.executing\n", directorio);

    sprintf(testigo, "%s/inotify.example.executing", directorio);
    fprintf(stderr, "      Testigo: %s\n", testigo);

    /*creating the INOTIFY instance*/
    fd = inotify_init();

    /*checking for error*/
    if (fd < 0) {
        perror("inotify_init");
    }

    /*adding the /tmp directory into watch list. Here, the suggestion is to validate the existence of the directory before adding into monitoring list.*/
    //  wd = inotify_add_watch( fd, "/tmp", IN_CREATE | IN_DELETE );

    /* Notificaremos los cambios en el directorio ./My_inotify */
    //  mkdir(argv[1]);
    wd_cd = inotify_add_watch(fd, directorio, IN_CREATE | IN_DELETE);

    /* Testigo para finalizar cuando lo borremos: */
    mkdir(testigo, ACCESSPERMS);



    /*read to determine the event change happens on the directory. Actually this read blocks until the change event occurs*/
    struct inotify_event event_st, *event;
    int k = 0;
    int exiting = 0;


    while (!exiting) {
        fprintf(stderr, "---%s: waiting for event %d...\n", argv[0], ++k);
        length = read(fd, buffer, EVENT_BUF_LEN);
        fprintf(stderr, "---%s: event %d read.\n", argv[0], k);
        /*checking for error*/
        if (length < 0) {
            perror("read");
            break;
        }
        //    struct inotify_event *event = ( struct inotify_event * ) &buffer[ i ];
        while ((i < length) && !exiting) {
            //    event= &event_st;
            event = (struct inotify_event *) &buffer[ i ];
            //    fprintf(stderr, "---example: event name length: %i\n", event->len);
            //    memcpy(event, buffer, length);
            if (event->len) {
                //      memcpy(event+EVENT_SIZE, buffer+EVENT_SIZE, length);
                if (event->mask & IN_CREATE) {
                    if (event->mask & IN_ISDIR) { // event: directory created
                        printf("---%s: New directory %s created.\n", argv[0], event->name);
                        
                        //codigo añadido por nosotros
                        printf("%s \n", event->name);
                        sprintf(buf, "%s%s\r\n", KOMANDOAK[COM_MKDR],  event->name);
                        write(sock, buf, strlen(buf)); // Enviar petición.
                        n = readline(sock, buf, MAX_BUF); // Recibir respuesta.
                        status = parse(buf);
                        if (status != 0) {
                            fprintf(stderr, "Error: ");
                            fprintf(stderr, "%s", ER_MEZUAK[status]);
                        }else{
                            printf("El directorio %s ha sido creado con éxito.\n", event->name);
                        }

                        //codigo añadido por nosotros
                        
                    } else { // event: fie created
                        printf("---%s: New file %s created.\n", argv[0], event->name);
                        
                        //codigo añadido por nosotros
                        //strcpy(param, event->name);
                        sprintf(param, "%s%s", directorio, event->name );
                        printf("%s\n",param);
                        if (stat(param, &file_info) < 0) // Conseguir tamaño de fichero
                        {
                            fprintf(stderr, "No se ha encontrado el fichero: %s .\n", param);
                        } else {
                            sprintf(buf, "%s%s?%ld\r\n", KOMANDOAK[COM_UPLO], event->name, file_info.st_size);
                            write(sock, buf, strlen(buf)); // Mandar petición
                            n = readline(sock, buf, MAX_BUF); // Obtener respuesta
                            status = parse(buf);
                            if (status != 0) {
                                fprintf(stderr, "Error: ");
                                fprintf(stderr, "%s", ER_MEZUAK[status]);
                            } else {
                                if ((fp = fopen(param, "r")) == NULL) // Abrir fichero
                                {
                                    fprintf(stderr, "No se ha podido abrir el fichero: %s.\n", param);
                                    exit(1);
                                }
                                sprintf(buf, "%s\r\n", KOMANDOAK[COM_UPL2]);
                                write(sock, buf, strlen(buf)); // Confirmar envío
                                while ((n = fread(buf, 1, MAX_BUF, fp)) == MAX_BUF) // Mandar fichero, en tamaño maximo de bloques
                                    write(sock, buf, MAX_BUF);
                                if (ferror(fp) != 0) {
                                    fprintf(stderr, "Error al mandar el fichero.\n");
                                    exit(1);
                                }
                                write(sock, buf, n); // Mandar ultimo bloque del fichero

                                n = readline(sock, buf, MAX_BUF); // Recibir respuesta
                                status = parse(buf);
                                if (status != 0) {
                                    fprintf(stderr, "Error: ");
                                    fprintf(stderr, "%s", ER_MEZUAK[status]);
                                } else
                                    printf("El fichero %s se ha subido con éxito.\n", param);
                            }
                        }


                        //codigo añadido por nosotros
                    }
                } else if (event->mask & IN_DELETE) {
                    if (event->mask & IN_ISDIR) { // event: directory removed
                        if (!strcmp(event->name, "inotify.example.executing")) {
                            rmdir("example.inotify.executing");
                            exiting = 1;
                            //              break;
                        }
                        printf("---%s: Directory %s deleted.\n", argv[0], event->name);
                        
                        //codigo añadido por nosotros
                        printf("%s \n", event->name);
                        sprintf(buf, "%s%s\r\n", KOMANDOAK[COM_DDEL],  event->name);
                        write(sock, buf, strlen(buf)); // Enviar petición.
                        n = readline(sock, buf, MAX_BUF); // Recibir respuesta.
                        status = parse(buf);
                        if (status != 0) {
                            fprintf(stderr, "Error: ");
                            fprintf(stderr, "%s", ER_MEZUAK[status]);
                        }else{
                            printf("El directorio %s ha sido borrado con éxito.\n", event->name);
                        }
                        //codigo añadido por nosotros
                        
                    } else { // event: file removed
                        printf("---%s: File %s deleted.\n", argv[0], event->name);
                        
                        //codigo añadido por nosotros
                        strcpy(param, event->name);
                        //param[strlen(param) - 1] = 0;
                        sprintf(buf, "%s%s\r\n", KOMANDOAK[COM_DELE], param);
                        write(sock, buf, strlen(buf)); // Eskaera bidali.
                        n = readline(sock, buf, MAX_BUF); // Erantzuna jaso.
                        status = parse(buf);
                        if (status != 0) {
                            fprintf(stderr, "Error: ");
                            fprintf(stderr, "%s", ER_MEZUAK[status]);
                        } else {
                            printf("El fichero %s ha sido borrado con éxito.\n", param);
                        }
                        //codigo añadido por nosotros
                    }
                }
            } else { // event ignored
                fprintf(stderr, "---%s: event ignored for %s\n", argv[0], event->name);
            }
            i += EVENT_SIZE + event->len;
            //    fprintf(stderr, "---example.event count: %i\n", i); 
        }
        i = 0;
    }
    fprintf(stderr, "---Exiting %s\n", argv[0]);
    /*removing the directory from the watch list.*/
    inotify_rm_watch(fd, wd);
    inotify_rm_watch(fd, wd_cd);
    //  rmdir(argv[1]);

    /*closing the INOTIFY instance*/
    close(fd);

}


/* Zerbitzaritik jasotako erantzuna aztertzen du.
* Jasotakoa OK bada 0 balioa itzuliko du eta bestela errorearen kode zenbakia.
*/
int parse(char *status)
{
	if(!strncmp(status,"OK",2))
		return 0;
	else if(!strncmp(status,"ER",2))
		return(atoi(status+2));
	else
	{
		fprintf(stderr,"Ustekabeko erantzuna.\n");
		exit(1); 
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
int readline(int stream, char *buf, int tam)
{
	/*
		Kontuz! Inplementazio hau sinplea da, baina ez da batere eraginkorra.
	*/
	char c;
	int guztira=0;
	int cr = 0;

	while(guztira<tam)
	{
		int n = read(stream, &c, 1);
		if(n == 0)
		{
			if(guztira == 0)
				return 0;
			else
				return -1;
		}
		if(n<0)
			return -3;
		buf[guztira++]=c;
		if(cr && c=='\n')
			return guztira;
		else if(c=='\r')
			cr = 1;
		else
			cr = 0;
	}
	return -2;
}
