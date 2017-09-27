#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <netinet/in.h>
#include <netdb.h>
#include <unistd.h>
#include <errno.h>
#include <linux/inotify.h>

#include "bez_fitx.h"

int main(int argc, char *argv[])
{
	char buf[MAX_BUF], param[MAX_BUF];
	char zerbitzaria[MAX_BUF];
	int portua = PORT;
	
	int sock, n, status, aukera;
	long fitx_tamaina, irakurrita;
	
	//notify 
	int length, x = 0;
  	int fd;
  	int wd;
  	int wd_cd;
  	char buffer[EVENT_BUF_LEN];
  	char testigo[1024];
  	char directorio[MAX_BUF];
  	//end notify
  	
	struct stat file_info;
	FILE *fp;
	struct sockaddr_in zerb_helb;
	struct hostent *hp;

	// Parametroak prozesatu.
	switch(argc)
	{
		case 4:
			portua = atoi(argv[3]);
			printf("%d \n", portua);
		case 3:
			strcpy(zerbitzaria, argv[2]);
			printf("%s \n", zerbitzaria);
			break;
		case 2:
			strcpy(directorio, argv[1]);
			printf("%s \n", directorio);
		case 1:
			strcpy(zerbitzaria, SERVER);
			break;
		default:
			printf("Erabilera: %s <zerbitzaria> <portua> <directorio>\n", argv[0]);
			exit(1);
	}
	
	fprintf(stderr, "---Prueba de inotify sobre %s\n", directorio);
  	fprintf(stderr, "---Notifica crear/borrar ficheros/directorios sobre %s\n", directorio);
	fprintf(stderr, "---%s debe exixtir!\n", directorio);
  	fprintf(stderr, "---Para salir, borrar %s/inotify.example.executing\n", directorio); 

  	sprintf(testigo, "%s/inotify.example.executing", directorio);
  	fprintf(stderr, "      Testigo: %s\n", testigo); 
	
	/*creating the INOTIFY instance*/
	fd = inotify_init();

	/*checking for error*/
	if ( fd < 0 ) {
	  perror( "inotify_init" );
	}

	/* Notificaremos los cambios en el directorio ./My_inotify */
	wd_cd = inotify_add_watch( fd, directorio, IN_CREATE | IN_DELETE );

	/* Testigo para finalizar cuando lo borremos: */
	mkdir(testigo, ACCESSPERMS);
	
	/*read to determine the event change happens on the directory. Actually this read blocks until the change event occurs*/ 
	struct inotify_event event_st, *event;
	int k=0;
	int exiting= 0;
	
	// Socketa sortu.
	if((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0)
	{
		perror("Errorea socketa sortzean");
		exit(1);
	}

	// Zerbitzariko socketaren helbidea sortu.
	zerb_helb.sin_family = AF_INET;
	zerb_helb.sin_port = htons(portua);
	if((hp = gethostbyname(zerbitzaria)) == NULL)
	{
		herror("Errorea zerbitzariaren izena ebaztean");
		exit(1);
	}
	memcpy(&zerb_helb.sin_addr, hp->h_addr, hp->h_length);
	
	// Konektatu zerbitzariarekin.
	if(connect(sock, (struct sockaddr *) &zerb_helb, sizeof(zerb_helb)) < 0)
	{
		perror("Errorea zerbitzariarekin konektatzean");
		exit(1);
	}
	
	// Erabiltzaile eta pasahitza bidali.
	int i=0;
	do
	{
		printf("Erabiltzaile izena: ");
		fgets(param,MAX_BUF,stdin);
		param[strlen(param)-1] = 0;
		sprintf(buf,"%s%s\r\n",KOMANDOAK[COM_USER],param);
		
		write(sock, buf, strlen(buf));
		readline(sock, buf, MAX_BUF);
		status = parse(buf);
		if(status != 0)
		{
			fprintf(stderr,"Errorea: ");
			fprintf(stderr,"%s",ER_MEZUAK[status]);
			continue;
		}

		printf("Pasahitza: ");
		fgets(param,MAX_BUF,stdin);
		param[strlen(param)-1] = 0;
		sprintf(buf,"%s%s\r\n",KOMANDOAK[COM_PASS],param);
		
		write(sock, buf, strlen(buf));
		readline(sock, buf, MAX_BUF);
		status = parse(buf);
		if(status != 0)
		{
			fprintf(stderr,"Errorea: ");
			fprintf(stderr,"%s",ER_MEZUAK[status]);
			continue;
		}
		break;
	} while(1);

	// Begizta bat aukeren menua erakutsi eta aukera prozesatzeko.
	
/*
	do
	{
		aukera = menua();		// Erakutsi aukeren menua.
		switch(aukera)
		{
			case OP_LIST:	// Fitxategi zerrenda eskatu eta pantailaratu.
				sprintf(buf,"%s\r\n",KOMANDOAK[COM_LIST]);
				write(sock,buf,strlen(buf));				// Bidali komandoa.
				n = readline(sock, buf, MAX_BUF);		// Erantzuna jaso.
				status = parse(buf);
				if(status != 0)
				{
					fprintf(stderr,"Errorea: ");
					fprintf(stderr,"%s",ER_MEZUAK[status]);
				}
				else
				{
					// Fitxategi zerrenda jaso eta pantailaratu lerroz lerro.
					int kop = 0;	// Zerrendako fitxategi kopurua kontrolatzeko.
					printf("Zerbitzaritik jasotako fitxategi zerrenda:\n");
					printf("------------------------------------------\n");
					n = readline(sock, buf, MAX_BUF);
					while(n > 2)
					{
						buf[n-2] = 0;
						printf("%s\t\t",strtok(buf,"?"));
						fitx_tamaina = atol(strtok(NULL,"?"));
						if(fitx_tamaina < 0)
							printf("Tamaina ezezaguna\n");
						else
						{
							if(fitx_tamaina < 1024)
								printf("% 5ld B\n", fitx_tamaina);
							else if(fitx_tamaina < 1024*1024)
								printf("%5.1f KB\n", fitx_tamaina/1024.0);
							else if(fitx_tamaina < 1024*1024*1024)
								printf("%5.1f MB\n", fitx_tamaina/(1024.0*1024));
							else
								printf("%5.1f GB\n", fitx_tamaina/(1024.0*1024*1024));
						}
						kop++;
						n = readline(sock, buf, MAX_BUF);
					}
					if(kop > 0)
					{
						printf("------------------------------------------\n");
						printf("Guztira %d fitxategi eskuragarri.\n",kop);
					}
					else
						printf("Ez dago fitxategirik eskuragarri.\n");
					printf("------------------------------------------\n");
				}	
				break;
			case OP_DOWN:		// Fitxategi bat jaitsi.
				printf("Idatzi jaitsi nahi duzun fitxategiaren izena: ");
				fgets(param,MAX_BUF,stdin);
				param[strlen(param)-1] = 0;
				sprintf(buf,"%s%s\r\n",KOMANDOAK[COM_DOWN], param);
				write(sock,buf,strlen(buf));			// Eskaera bidali.
				n = readline(sock, buf, MAX_BUF);	// Erantzuna jaso.
				status = parse(buf);
				if(status != 0)
				{
					fprintf(stderr,"Errorea: ");
					fprintf(stderr,"%s",ER_MEZUAK[status]);
				}
				else
				{
					buf[n-2] = 0; //EOL ezabatu.
					fitx_tamaina = atol(buf+2);
					sprintf(buf,"%s\r\n",KOMANDOAK[COM_DOW2]);
					write(sock,buf,strlen(buf));	// Eskaera konfirmatu.
					n = readline(sock, buf, MAX_BUF);	// Erantzuna jaso.
					status = parse(buf);
					if(status != 0)
					{
						fprintf(stderr,"Errorea: ");
						fprintf(stderr,"%s",ER_MEZUAK[status]);
					}
					else
					{
						irakurrita = 0;
						if((fp = fopen(param,"w")) == NULL)		// Fitxategia sortu diskoan.
						{
							perror("Ezin da fitxategia disko lokalean gorde");
							exit(1);
						}
						while(irakurrita < fitx_tamaina)	// Fitxategia jaso eta diskoan gorde.
						{
							n = read(sock, buf, MAX_BUF);
							if(fwrite(buf, 1, n, fp) < 0)
							{
								perror("Arazoa fitxategia disko lokalean gordetzean");
								exit(1);
							}
							irakurrita += n;
						}
						fclose(fp);
						printf("%s fitxategia jaso da.\n",param);
					}
				}
				break;
			case OP_UP:		// Fitxategi bat igo.
				printf("Idatzi igo nahi duzun fitxategiaren izena: ");
				fgets(param,MAX_BUF,stdin);
				param[strlen(param)-1] = 0;
				if(stat(param, &file_info) < 0)	// Fitxategiaren tamaina lortu.
				{
					fprintf(stderr,"%s fitxategia ez da aurkitu.\n",param);
				}
				else
				{
					sprintf(buf,"%s%s?%ld\r\n",KOMANDOAK[COM_UPLO], param, file_info.st_size);
					write(sock,buf,strlen(buf));				// Eskaera bidali.
					n = readline(sock, buf, MAX_BUF);		// Erantzuna jaso.
					status = parse(buf);
					if(status != 0)
					{
						fprintf(stderr,"Errorea: ");
						fprintf(stderr,"%s",ER_MEZUAK[status]);
					}
					else
					{
						if((fp = fopen(param,"r")) == NULL)	// Fitxategia ireki.
						{
							fprintf(stderr,"%s fitxategia ezin izan da ireki.\n",param);
							exit(1);
						}
						sprintf(buf,"%s\r\n",KOMANDOAK[COM_UPL2]);
						write(sock,buf,strlen(buf));	// Bidalketa konfirmatu.
						while((n=fread(buf,1,MAX_BUF,fp))==MAX_BUF)	// Fitxategia bidali, tamaina maximoko blokeetan.
							write(sock,buf,MAX_BUF);
						if(ferror(fp)!=0)
						{
							fprintf(stderr,"Errorea gertatu da fitxategia bidaltzean.\n");
							exit(1);
						}
						write(sock,buf,n);	// Fitxategiaren azkeneko blokea bidali.
					
						n = readline(sock, buf, MAX_BUF);	// Erantzuna jaso.
						status = parse(buf);
						if(status != 0)
						{
							fprintf(stderr,"Errorea: ");
							fprintf(stderr,"%s",ER_MEZUAK[status]);
						}
						else
							printf("%s fitxategia igo da.\n", param);
					}
				}
				break;
			case OP_DEL:		// Fitxategi bat ezabatu.
				printf("Idatzi ezabatu nahi duzun fitxategiaren izena: ");
				fgets(param,MAX_BUF,stdin);
				param[strlen(param)-1] = 0;
				sprintf(buf,"%s%s\r\n",KOMANDOAK[COM_DELE], param);
				write(sock,buf,strlen(buf));				// Eskaera bidali.
				n = readline(sock, buf, MAX_BUF);		// Erantzuna jaso.
				status = parse(buf);
				if(status != 0)
				{
					fprintf(stderr,"Errorea: ");
					fprintf(stderr,"%s",ER_MEZUAK[status]);
				}
				else
				{
					printf("%s fitxategia ezabatua izan da.\n", param);
				}
				break;
			case OP_EXIT:		// Sesioa amaitu.
				sprintf(buf,"%s\r\n",KOMANDOAK[COM_EXIT]);
				write(sock,buf,strlen(buf));				// Eskaera bidali.
				n = readline(sock, buf, MAX_BUF);		// Erantzuna jaso.
				status = parse(buf);
				if(status != 0)
				{
					fprintf(stderr,"Errorea: ");
					fprintf(stderr,"%s",ER_MEZUAK[status]);
				}
				break;
		}
	} while(aukera != 5);
*/

	while (!exiting) {
	    fprintf(stderr, "---%s: waiting for event %d...\n", argv[0], ++k); 
	    length = read( fd, buffer, EVENT_BUF_LEN ); 
	    fprintf(stderr, "---%s: event %d read.\n", argv[0], k); 
	    /*checking for error*/
	    if ( length < 0 ) {
		perror( "read" );
		break;
	    }
	//    struct inotify_event *event = ( struct inotify_event * ) &buffer[ i ];
	    while ( (x < length) && !exiting ) {
	//    event= &event_st;
		event = ( struct inotify_event * ) &buffer[ x ];
	//    fprintf(stderr, "---example: event name length: %i\n", event->len);
	//    memcpy(event, buffer, length);
		if ( event->len ) {
	//      memcpy(event+EVENT_SIZE, buffer+EVENT_SIZE, length);
		  if ( event->mask & IN_CREATE ) {
		    if ( event->mask & IN_ISDIR ) {	// event: directory created
		      printf( "---%s: New directory %s created.\n", argv[0], event->name );
		      sprintf(buf,"%s%s\r\n",KOMANDOAK[COM_MKDR], event->name);
		      write(sock,buf,strlen(buf));		// Enviar petición.
		      n = readline(sock, buf, MAX_BUF);		// Recibir respuesta.
		      status = parse(buf);
		      if(status != 0)
		      {
			   fprintf(stderr,"Error: ");
			   fprintf(stderr,"%s",ER_MEZUAK[status]);
		      }
		      break;  // Oier: no se porque sin este break se mete en un bucle sin fin, 
		      	 //lo he puesto para hacer una prueba y se ha arreglado solo. Asi que no lo quiteis.
		    }
		    else {	// event: file created
		      printf( "---%s: New file %s created.\n", argv[0], event->name );
		      strcpy(param, event->name);
		      if(stat(param, &file_info) < 0)	// Conseguir el tamaño del fichero.
		      {
			   fprintf(stderr,"%s fitxategia ez da aurkitu.\n", param);
		      }
		      else
	              {
			    sprintf(buf,"%s%s?%ld\r\n",KOMANDOAK[COM_UPLO], param, file_info.st_size);
			    write(sock, buf, strlen(buf));		// Enviar petición.
			    n = readline(sock, buf, MAX_BUF);		// Recibir respuesta.
			    status = parse(buf);
			    if(status != 0)
			    {
				  fprintf(stderr,"Error: ");
				  fprintf(stderr,"%s",ER_MEZUAK[status]);
			    }
			    else
			    {
				  if((fp = fopen(param,"r")) == NULL)	// Abrir el fichero.
				  {
					fprintf(stderr,"El fichero %s no se puede abrir.\n",param);
					exit(1);
			        }
					sprintf(buf,"%s\r\n",KOMANDOAK[COM_UPL2]);
					write(sock,buf,strlen(buf));	// Confirmar el envío.
					while((n=fread(buf,1,MAX_BUF,fp))==MAX_BUF)	// Enviar el fichero con el tamaño máximo de bloque.
						write(sock,buf,MAX_BUF);
						if(ferror(fp)!=0)
						{
							fprintf(stderr,"Se ha producido un error al enviar el fichero.\n");
							exit(1);
						}
						write(sock,buf,n);	// Enviar el último bloque del fichero.
					
						n = readline(sock, buf, MAX_BUF);	// Recibir respuesta.
						status = parse(buf);
						if(status != 0)
						{
							fprintf(stderr,"Error: ");
							fprintf(stderr,"%s",ER_MEZUAK[status]);
						}
			    }
		       }
		    }
		  }
		  else if ( event->mask & IN_DELETE ) {
		    if ( event->mask & IN_ISDIR ) {	// event: directory removed
		      if (!strcmp(event->name, "inotify.example.executing")) {
		        rmdir("example.inotify.executing");
		        exiting=1;
	//              break;
		      }
		      printf( "---%s: Directory %s deleted.\n", argv[0], event->name );
		    }
		    else {	// event: fie removed
		      printf( "---%s: File %s deleted.\n", argv[0], event->name );
		    }
		  }
		}
		else {	// event ignored
		  fprintf(stderr, "---%s: event ignored for %s\n", argv[0], event->name); 
		}
		i += EVENT_SIZE + event->len;
	//    fprintf(stderr, "---example.event count: %i\n", i); 
	    }
	    x= 0;
  	}
  	
  	fprintf(stderr, "---Exiting %s\n", argv[0]); 
	/*removing the directory from the watch list.*/
	inotify_rm_watch( fd, wd );
	inotify_rm_watch( fd, wd_cd );
	//  rmdir(argv[1]);

 	/*closing the INOTIFY instance*/
  	close( fd );
	
	close(sock);
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

int menua()
{
	char katea[MAX_BUF];
	int aukera;
	
	printf("\n");
	printf("\t\t\t\t*********************************************\n");
	printf("\t\t\t\t*********************************************\n");
	printf("\t\t\t\t**                                         **\n");
	printf("\t\t\t\t**       1. Fitxategi zerrenda             **\n");
	printf("\t\t\t\t**       2. Fitxategia jeitsi              **\n");
	printf("\t\t\t\t**       3. Fitxategia igo                 **\n");
	printf("\t\t\t\t**       4. Fitxategia ezabatu             **\n");
	printf("\t\t\t\t**       5. Saioa amaitu                   **\n");
	printf("\t\t\t\t**                                         **\n");
	printf("\t\t\t\t*********************************************\n");
	printf("\t\t\t\t*********************************************\n");
	
	printf("\t\t\t\tEgin zure aukera: ");
	while(1)
	{
		fgets(katea,MAX_BUF,stdin);
		aukera = atoi(katea);
		if(aukera > 0 && aukera < 6)
			break;
		printf("\t\t\t\tAukera okerra, saiatu berriro: ");
	}
	printf("\n");
	return aukera;
}
