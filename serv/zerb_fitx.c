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

#include "zerb_fitx.h"

int main()
{
	int sock, elkarrizketa;
	struct sockaddr_in zerb_helb;
	socklen_t helb_tam;
	
	// Sortu socketa.
	if((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0)
	{
		perror("Errorea socketa sortzean");
		exit(1);
	}
	
	memset(&zerb_helb, 0, sizeof(zerb_helb));
	zerb_helb.sin_family = AF_INET;
	zerb_helb.sin_addr.s_addr = htonl(INADDR_ANY);
	zerb_helb.sin_port = htons(PORT);

	// Esleitu helbidea socketari.	
	if(bind(sock, (struct sockaddr *) &zerb_helb, sizeof(zerb_helb)) < 0)
	{
		perror("Errorea socketari helbide bat esleitzean");
		exit(1);
	}
	
	// Ezarri socketa entzute socket gisa.
	if(listen(sock,5) < 0)
	{
		perror("Errorea socketa entzute socket bezala ezartzean");
		exit(1);
	}

	// Adierzi SIG_CHLD seinalea jasotzean ez dela ezer egingo. Modu honetan amaitutako prozesu umeak ez dira zonbi egoeran geratuko.
	signal(SIGCHLD, SIG_IGN);

	while(1)
	{
	
		// Onartu konexio eskaera eta sortu elkarrizketa socketa.
		if((elkarrizketa = accept(sock, NULL, NULL)) < 0)
		{
			perror("Errorea konexioa onartzean");
			exit(1);
		}

		// Sortu prozesu ume bat bezeroarekin komunikatzeko.
		switch(fork())
		{
			case 0:
				close(sock);
				sesioa(elkarrizketa);
				close(elkarrizketa);
				exit(0);
			default:
				close(elkarrizketa);
		}
	}
}

/*
* Funtzio honetan kodetzen da bezeroarekin komunikatu behar den prozesu umeak egin beharrekoa, aplikazio protokoloak zehaztu bezala.
* Parametro gisa elkarrizketa socketa pasa behar zaio.
*/
void sesioa(int s)
{
	char buf[MAX_BUF], file_path[MAX_BUF], file_name[MAX_BUF], dir_path[MAX_BUF], dir_name[MAX_BUF];
	int n, erabiltzaile, komando, error;
	FILE *fp;
	struct stat file_info;
	unsigned long file_size, irakurrita;
	char * sep;
	
	// Zehaztu uneko egoera bezala hasierako egoera.
	egoera = ST_INIT;
	
	while(1)
	{
		// Irakurri bezeroak bidalitako mezua.
		if((n=readline(s,buf,MAX_BUF)) <= 0)
			return;
		
		// Aztertu jasotako komandoa ezaguna den ala ez.
		if((komando=bilatu_substring(buf,KOMANDOAK)) < 0)
		{
			ustegabekoa(s);
			continue;
		}
		
		// Jasotako komandoaren arabera egin beharrekoa egin.
		switch(komando)
		{
			case COM_USER:
				if(egoera != ST_INIT)		// Egiaztatu esperotako egoeran jaso dela komandoa.
				{
					ustegabekoa(s);
					continue;
				}
				buf[n-2] = 0;	// Lerro bukaera, edo "End Of Line" (EOL), ezabatzen du.
				// Baliozko erabiltzaile bat den egiaztatu.
				if((erabiltzaile = bilatu_string(buf+4, erab_zer)) < 0)
				{
					write(s,"ER2\r\n",5);
				}
				else
				{
					write(s,"OK\r\n",4);
					egoera = ST_AUTH;
				}
				break;
			case COM_PASS:
				if(egoera != ST_AUTH)		// Egiaztatu esperotako egoeran jaso dela komandoa.
				{
					ustegabekoa(s);
					continue;
				}
				buf[n-2] = 0;	// EOL ezabatu.
				// Pasahitza zuzena dela egiaztatu.
				if(erabiltzaile == 0 || !strcmp(pass_zer[erabiltzaile], buf+4))
				{
					write(s,"OK\r\n",4);
					egoera = ST_MAIN;
				}
				else
				{
					write(s,"ER3\r\n",5);
					egoera = ST_INIT;
				}
				break;
			case COM_LIST:
				if(n>6 || egoera != ST_MAIN)	// Egiaztatu esperotako egoeran jaso dela komandoa eta ez dela parametrorik jaso.
				{
					ustegabekoa(s);
					continue;
				}
				if(bidali_zerrenda(s) < 0)
					write(s,"ER4\r\n",5);
				break;
			case COM_DOWN:
				if(egoera != ST_MAIN)		// Egiaztatu esperotako egoeran jaso dela komandoa.
				{
					ustegabekoa(s);
					continue;
				}
				buf[n-2] = 0; // EOL ezabatu.
				sprintf(file_path,"%s/%s",FILES_PATH,buf+4);		// Fitxategiak dauden karpeta eta fitxategiaren izena kateatu.
				if(stat(file_path, &file_info) < 0)							// Lortu fitxategiari buruzko informazioa.
				{
					write(s,"ER5\r\n",5);
				}
				else
				{
					sprintf(buf,"OK%ld\r\n", file_info.st_size);
					write(s, buf, strlen(buf));
					egoera = ST_DOWN;
				}
				break;
			case COM_DOW2:
				if(n > 6 || egoera != ST_DOWN)		// Egiaztatu esperotako egoeran jaso dela komandoa eta ez dela parametrorik jaso.
				{
					ustegabekoa(s);
					continue;
				}
				egoera = ST_MAIN;
				// Fitxategia ireki.
				if((fp=fopen(file_path,"r")) == NULL)
				{
					write(s,"ER6\r\n",5);
				}
				else
				{
					// Fitxategiaren lehenengo zatia irakurri eta errore bat gertatu bada bidali errore kodea.
					if((n=fread(buf,1,MAX_BUF,fp))<MAX_BUF && ferror(fp) != 0)
					{
						write(s,"ER6\r\n",5);
					}
					else
					{
						write(s,"OK\r\n",4);
						// Bidali fitxategiaren zatiak.
						do
						{
							write(s,buf,n);
						} while((n=fread(buf,1,MAX_BUF,fp)) == MAX_BUF);
						if(ferror(fp) != 0)
						{
							close(s);
							return;
						}
						else if(n>0)
							write(s,buf,n);	// Bidali azkeneko zatia.
					}
					fclose(fp);
				}
				break;
			case COM_UPLO:
				if(egoera != ST_MAIN)		// Egiaztatu esperotako egoeran jaso dela komandoa.
				{
					ustegabekoa(s);
					continue;
				}
				// Erabiltzaile anonimoak ez dauka ekintza honetarako baimenik.
				if(erabiltzaile==0)
				{
					write(s,"ER7\r\n",5);
					continue;
				}
				
				buf[n-2] = 0;	// EOL kendu.
				// Mezuak dauzkan bi zatiak (fitxategi izena eta tamaina) erauzi.
				if((sep = strchr(buf,'?')) == NULL)
				{
					ustegabekoa(s);
					continue;
				}
				*sep = 0;
				sep++;
				strcpy(file_name,buf+4);	// Fitxategi izena lortu.
				file_size = atoi(sep);		// Fitxategi tamaina lortu.
				sprintf(file_path,"%s/%s",FILES_PATH,file_name);	// Fitxategiak dauden karpeta eta fitxategiaren izena kateatu.
				if(file_size > MAX_UPLOAD_SIZE)	// Fitxategi tamainak maximoa gainditzen ez duela egiaztatu.
				{
					write(s,"ER8\r\n",5);
					continue;
				}
				if(toki_librea() < file_size + SPACE_MARGIN)	// Mantendu beti toki libre minimo bat diskoan.
				{
					write(s,"ER9\r\n",5);
					continue;
				}
				write(s,"OK\r\n",4);
				egoera = ST_UP;
				break;
			case COM_UPL2:
				if(n > 6 || egoera != ST_UP)		// Egiaztatu esperotako egoeran jaso dela komandoa eta ez dela parametrorik jaso.
				{
					ustegabekoa(s);
					continue;
				}
				egoera = ST_MAIN;
				irakurrita = 0L;
				fp=fopen(file_path,"w");		// Sortu fitxategi berria diskoan.
				error = (fp == NULL);
				while(irakurrita < file_size)
				{
					// Jaso fitxategi zati bat.
					if((n=read(s,buf,MAX_BUF)) <= 0)
					{
						close(s);
						return;
					}
					// Ez bada errorerik izan gorde fitxategi zatia diskoan.
					// Errorerik gertatuz gero segi fitxategi zatiak jasotzen.
					// Fitxategi osoa jasotzeak alferrikako trafikoa sortzen du, baina aplikazio protokoloak ez du beste aukerarik ematen.
					if(!error)
					{
						if(fwrite(buf, 1, n, fp) < n)
						{
							fclose(fp);
							unlink(file_path);
							error = 1;
						}
					}
					irakurrita += n;
				}
				if(!error)
				{
					fclose(fp);
					write(s,"OK\r\n",4);
				}
				else
					write(s,"ER10\r\n",6);
				break;
			case COM_MKDR:
				if(egoera != ST_MAIN)
				{
					ustegabekoa(s);
					continue;
				}
				buf[n-2] = 0; // EOL ezabatu.
				strcpy(dir_name,buf+4);	// Obtener el nombre del directorio.
				sprintf(dir_path,"%s/%s",FILES_PATH, dir_name);	// Conseguir el PATH del nuevo directorio.
				error=mkdir(dir_path, ACCESSPERMS);  // Crear el nuevo directorio.
				if(error < 0)
					write(s, "ER11\r\n",6);
				break;
			case COM_DELE:
				if(egoera != ST_MAIN)		// Egiaztatu esperotako egoeran jaso dela komandoa.
				{
					ustegabekoa(s);
					continue;
				}
				// Erabiltzaile anonimoak ez dauka ekintza honetarako baimenik.
				if(erabiltzaile==0)
				{
					write(s,"ER7\r\n",5);
					continue;
				}
				buf[n-2] = 0; // EOL ezabatu.
				sprintf(file_path,"%s/%s",FILES_PATH,buf+4);	// Fitxategiak dauden karpeta eta fitxategiaren izena kateatu.
				if(unlink(file_path) < 0)		// Ezabatu fitxategia.
					write(s,"ER11\r\n",6);
				else
					write(s,"OK\r\n",4);
				break;
			//case COM_DDEL:
				
			case COM_EXIT:
				if(n > 6)	// Egiaztatu ez dela parametrorik jaso.
				{
					ustegabekoa(s);
					continue;
				}
				write(s,"OK\r\n",4);
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

/*
* 'string' parametroko karaktere katea bilatzen du 'string_zerr' parametroan. 'string_zerr' bektoreko azkeneko elementua NULL izan behar da.
* 'string' katearen lehen agerpenaren indizea itzuliko du, edo balio negatibo bat ez bada agerpenik aurkitu.
*/

int bilatu_string(char *string, char **string_zerr)
{
	int i=0;
	while(string_zerr[i] != NULL)
	{
		if(!strcmp(string,string_zerr[i]))
			return i;
		i++;
	}
	return -1;
}

/*
* 'string' parametroa 'string_zerr' bektoreko karaktere kateetako batetik hasten den egiaztatzen du. 'string_zerr' bektoreko azkeneko elementua NULL izan behar da.
* 'string' parametro karaktere katearen hasierarekin bat egiten duen 'string_zerr' bektoreko lehenego karaktere katearen indizea itzuliko du. Ez bada bat-egiterik egon balio negatibo bat itzuliko du.
*/
int bilatu_substring(char *string, char **string_zerr)
{
	int i=0;
	while(string_zerr[i] != NULL)
	{
		if(!strncmp(string,string_zerr[i],strlen(string_zerr[i])))
			return i;
		i++;
	}
	return -1;
}

/*
* Ustekabeko zerbait gertatzen denean egin beharrekoa: dagokion errorea bidali bezeroari eta egoera eguneratu.
*/
void ustegabekoa(int s)
{
	write(s,"ER1\r\n",5);
	if(egoera == ST_AUTH)
	{
		egoera = ST_INIT;
	}
	else if(egoera == ST_DOWN || egoera == ST_UP)
	{
		egoera = ST_MAIN;
	}
}

/*
* 's' streamaren bitartez eskuragarri dauden fitxategien zerrenda bidaltzen du. Ez bada posible fitxategiak dauden katalogoa aztertzea balio negatibo bat itzuliko du eta bestela, zerrendako fitxategi kopurua.
*/
int bidali_zerrenda(int s)
{
	struct dirent ** fitxategi_izenak;
	int i,fitxategi_kop;
	long fitxategi_tam;
	char buf[MAX_BUF], fitxategi_path[MAX_BUF];
	struct stat file_info;

	fitxategi_kop = scandir(FILES_PATH, &fitxategi_izenak, ez_ezkutua, alphasort);
	if(fitxategi_kop < 0)
		return -1;
	if(write(s,"OK\r\n",4) < 4)
		return -1;
		
	for(i=0;i<fitxategi_kop;i++)
	{
		sprintf(fitxategi_path,"%s/%s",FILES_PATH, fitxategi_izenak[i]->d_name);
		if(stat(fitxategi_path,&file_info) < 0)
			fitxategi_tam = -1L;
		else
			fitxategi_tam = file_info.st_size;
		sprintf(buf,"%s?%ld\r\n",fitxategi_izenak[i]->d_name, fitxategi_tam);
		if(write(s, buf, strlen(buf)) < strlen(buf))
			return i;
	}
	write(s,"\r\n",2);
	return fitxategi_kop;
}

/*
* Eskuragarri dauden fitxategiak kokatuta dauden diskoan eskuragarri dagoen tokia itzultzen du bytetan. Erroreren bat gertatuz gero balio negatibo bat itzuliko du.
*/
unsigned long toki_librea()
{
	struct statvfs info;

	if(statvfs(FILES_PATH,&info)<0)
		return -1;
	return info.f_bsize * info.f_bavail;
}

/*
* Katalogo bat eskaneatzerakoan fitxategi ezkutuak kontuan ez hartzeko eginiko iragazkia. Fitxategia '.' karaktereaz hasten bada 0 itzuliko du eta bestela 1. Kontuan izan fitxategi ezkutuak adierazteko modu hau UNIX eta antzeko sistemetan erabiltzen dela, beraz Windows sistemetan ez du esperotako portaera izango.
*/
int ez_ezkutua(const struct dirent *entry)
{
   if (entry->d_name[0]== '.')
     return (0);
   else
     return (1);
}


