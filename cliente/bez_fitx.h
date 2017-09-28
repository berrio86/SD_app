#define MAX_BUF 1024
#define SERVER "localhost"
#define PORT 6012

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

#define OP_LIST	1
#define OP_DOWN	2
#define OP_UP	3
#define OP_DEL	4
#define OP_EXIT	5
#define OP_MKDR 6
#define OP_DDEL 7

#define EVENT_SIZE  ( sizeof (struct inotify_event) )
//#define EVENT_SIZE  (3*sizeof(uint32_t)+sizeof(int))
#define EVENT_BUF_LEN     ( 1024 * ( EVENT_SIZE + 16 ) )


char * KOMANDOAK[] = {"USER","PASS","LIST","DOWN","DOW2","UPLO","UPL2","DELE", "MKDR", "DDEL", "EXIT",NULL};
char * ER_MEZUAK[] =
{
	"0 Dena ondo. Errorerik ez.\n",
	"1 Komando ezezaguna edo ustegabekoa.\n",
	"2 Erabiltzaile ezezaguna.\n",
	"3 Pasahitz okerra.\n",
	"4 Arazoa fitxategi zerrenda sortzen.\n",
	"5 Fitxategia ez da existizen.\n",
	"6 Arazoa fitxategia jeistean.\n",
	"7 Erabiltzaile anonimoak ez dauka honetarako baimenik.\n",
	"8 Fitxategiaren tamaina haundiegia da.\n",
	"9 Arazoa fitxategia igotzeko prestatzean.\n",
	"10 Arazoa fitxategia igotzean.\n",
	"11 Arazoa fitxategia ezabatzean.\n",
	"12 Error al crear directorio nuevo.\n"
};

int parse(char *status);
int readline(int stream, char *buf, int tam);
int menua();
