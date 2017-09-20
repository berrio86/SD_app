#define MAX_BUF 1024
#define PORT 6012
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
#define COM_EXIT	8

#define MAX_UPLOAD_SIZE	10*1024*1024	// 10 MB
#define SPACE_MARGIN		50*1024*1024	// 50 MB

char * KOMANDOAK[] = {"USER","PASS","LIST","DOWN","DOW2","UPLO","UPL2","DELE","EXIT",NULL};
char * erab_zer[] = {"anonimous","sar","sza",NULL};
char * pass_zer[] = {"","sar","sza"};
int egoera;

void sesioa(int s);
int readline(int stream, char *buf, int tam);
int bilatu_string(char *string, char **string_zerr);
int bilatu_substring(char *string, char **string_zerr);
void ustegabekoa(int s);
int bidali_zerrenda(int s);
unsigned long toki_librea();
void sig_chld(int signal);
int ez_ezkutua(const struct dirent *entry);
