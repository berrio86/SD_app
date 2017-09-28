#define MAX_BUF 1024
#define SERVER "localhost"
#define FILES_PATH "./files/"
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
#define OP_UP		3
#define OP_DEL	4
#define OP_EXIT	5
#define OP_MKDR 6
#define OP_DDEL 7


char * KOMANDOAK[] = {"USER","PASS","LIST","DOWN","DOW2","UPLO","UPL2","DELE","MKDR", "DDEL","EXIT",NULL};
char * ER_MEZUAK[] =
{
	"Todo bien. Sin errores.\n",
	"Comando desconocido o inesperado.\n",
	"Usuario desconocido.\n",
	"Password incorrecto.\n",
	"Problemas al crear la lista de ficheros.\n",
	"El fichero no existe.\n",
	"Error al descargar el fichero.\n",
	"El usuario anonimo no tiene permisos para esa acción.\n",
	"El tamaño del fichero es demasiado grande.\n",
	"Error en el preparativo de subida del fichero.\n",
	"Error al subir el fichero.\n",
	"Error al borrar el fichero.\n"
};

int parse(char *status);
int readline(int stream, char *buf, int tam);
