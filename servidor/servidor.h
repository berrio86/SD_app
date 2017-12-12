#define MAX_BUF 1024
#define PORT 6012
#define PORT_SERVIDORES 6013
#define FILES_PATH	"files"

#define ST_INIT	0
#define ST_AUTH	1
#define ST_MAIN	2
#define ST_DOWN	3
#define ST_UP		4

#define COM_USER	0
#define COM_PASS	1
#define COM_LIST	2
#define COM_DOWN	3
#define COM_DOW2	4
#define COM_UPLO	5
#define COM_UPL2	6
#define COM_DELE	7
#define COM_MKDR	8
#define COM_DDEL	9
#define COM_EXIT	10

#define MAX_UPLOAD_SIZE	10*1024*1024	// 10 MB
#define SPACE_MARGIN		50*1024*1024	// 50 MB

char * KOMANDOAK[] = {"USER","PASS","LIST","DOWN","DOW2","UPLO","UPL2","DELE","MKDR", "DDEL","EXIT",NULL};
char * erab_zer[] = {"sd",NULL};
char * pass_zer[] = {"sd"};
int egoera;

struct mensaje {
    int cont;
    char valor[];
};

void sesioa(int s);
int readline(int stream, char *buf, int tam);
int bilatu_string(char *string, char **string_zerr);
int bilatu_substring(char *string, char **string_zerr);
void ustegabekoa(int s);
int bidali_zerrenda(int s);
unsigned long toki_librea();
void sig_chld(int signal);
int ez_ezkutua(const struct dirent *entry);

/*GUK GEHITUTAKO KODEA*/
void establecerPrimario();
void establecerSecundario();
void waitFor(unsigned int);
void *establecerSocketServidores(void *a);
void *establecerSocketClientes (void *a);
int chequear_mensaje(struct mensaje);
void enviar(struct sockaddr_in, char[]);
void r_difundir(int [], struct mensaje);
void recibir();
void r_entregar( char[]);
void actualizarListaServidores(struct sockaddr_in);
void enviarListaDeServidores();
void * recibirListaDeServidores(void *a);

