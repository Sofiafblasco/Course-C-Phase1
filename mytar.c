/*
 * INCLUDES
 */

#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

/*
 * MACROS
 */

/* Minimun number of arguments for tar command */
#define	MIN_NUM_OF_ARGUMENTS		2

/* Message preffix */
#define	MSG_PREFFIX					"mytar:"

/* Options of tar command */
#define	OPT_FILENAME				"-f"
#define	OPT_LIST_FILES				"-t"

/* Block size of blocks of an archive */
#define	BLOCKSIZE_BYTES				512

/* Maximun size of file name */
#define	SIZE_NAME_MAX				100

/* Maximum number of files not found */
#define	MAX_FILES_NOT_FOUND			100

/* Octal base */
#define	OCTAL_BASE					8

/* Error codes */
#define	ERROR_CODE_TWO				2

/* Values for typeflag field */
/* Regular file */
#define	REGTYPE						'0'
/* Regular file */
#define	AREGTYPE					'\0'
/* Link */
#define	LNKTYPE						'1'
/* Reserved */
#define	SYMTYPE						'2'
/* Character special */
#define	CHRTYPE						'3'
/* Block special */
#define	BLKTYPE						'4'
/* Directory */
#define	DIRTYPE						'5'
/* FIFO special */
#define	FIFOTYPE					'6'
/* Reserved */
#define	CONTTYPE					'7'
/* Extended header referring to the next file in the archive */
#define	XHDTYPE						'x'
/* Global extended header */
#define	XGLTYPE						'g'

/*
 * GLOBAL VARIABLES
 */
typedef struct header
{
    union
	{
		struct
		{
			char name[100];				// file name
			char mode[8];				// permissions
			char uid[8];				// user id (octal)
			char gid[8];				// group id (octal)
			char size[12];				// size (octal)
			char mtime[12];				// modification time (octal)
			char chksum[8];				// sum of unsigned characters in block, with spaces in the check field while calculation is done (octal)
			char typeflag;				// link indicator (one caracter)
			char linkname[100];			// name of linked file
			char magic[6];
			char version[2];
			char uname[32];
			char gname[32];
			char devmajor[8];
			char devminor[8];
			char prefix[155];
		};
		char block[BLOCKSIZE_BYTES];
	};
	struct header * next;
} header_t;

typedef struct file
{
    char * name;
    struct file * next;
} file_t;

/*
 * FUNCTIONS PROTOTYPES
 */
bool isUnknownOptionFound(char *inputArgs[], int numInputArgs);
header_t *createHeader();
file_t *createFile(char *file_name);
header_t *addHeader(header_t *list, header_t *new);
file_t *addFile(file_t *listFiles, file_t *new);
void printNameHeaders(header_t *list);
void printNameFiles(file_t *listFiles, int filesNotFoundCount);
bool isZeroBlock(header_t *header);
int countBytesToSkip(header_t *header);
int findFile(char *file_name, header_t *list);
void sortFileList(file_t **ptrFilesFound);
void orderFilesIndex(char *argv[], int argc);
int checkTruncatedFile(FILE *tarArchive);
void freeList(header_t *list);
void freeFileList(file_t *list);

/*
 * FUNCTIONS
 */

header_t *createHeader() {
	header_t *new = NULL;
	new = (header_t *) malloc(sizeof (header_t));
	new->next = NULL;
	return (new);
}

file_t *createFile(char *file_name) {
	file_t * new = NULL;
	new = (file_t *) malloc(sizeof (file_t));
	new->name = (char *)malloc(strlen(file_name) + 1);
	strcpy(new->name, file_name);
	new->next = NULL;
	return (new);
}

header_t *addHeader(header_t *list, header_t *new) {
	if (list == NULL)
		list = new;
	else
	{
		header_t *aux = list;
		while (aux->next != NULL)
		{
			aux = aux -> next;
		}
		aux->next = new;
	}
	return (list);
}

file_t *addFile(file_t *listFiles, file_t *new) {
	if (listFiles == NULL)
		listFiles = new;
	else
	{
		file_t *aux = listFiles;
		while (aux->next != NULL)
		{
			aux = aux->next;
		}
		aux->next = new;
	}
	return (listFiles);
}

void printNameHeaders(header_t *list) {
	header_t *aux = list;
	while (aux != NULL)
	{
		printf("%s\n", aux->name);
		aux = aux->next;
	}
}

void printNameFiles(file_t *listFiles, int filesNotFoundCount) {
    file_t *aux = listFiles;
    while (aux != NULL)
	{
		if (filesNotFoundCount > 0)
			fprintf(stderr, "%s\n", aux->name);
		else
			printf("%s\n", aux->name);
		aux = aux->next;
	}
}

bool isZeroBlock(header_t *header) {
	header_t allZeroHeader;
	memset(&allZeroHeader, 0, BLOCKSIZE_BYTES);

	if (memcmp(&allZeroHeader, header, BLOCKSIZE_BYTES) == 0)
		return (true);
	return (false);
}

int countBytesToSkip(header_t *header) {
	long int contentSize = strtol(header->size, NULL, OCTAL_BASE);
	long int bytesToSkip = 0;

	if (contentSize > 0)
	{
		if (contentSize > BLOCKSIZE_BYTES)
		{
			bytesToSkip = (contentSize/BLOCKSIZE_BYTES) * BLOCKSIZE_BYTES;
			if ((contentSize % BLOCKSIZE_BYTES) != 0)
				bytesToSkip = ((contentSize/BLOCKSIZE_BYTES) * BLOCKSIZE_BYTES) + BLOCKSIZE_BYTES;
		} else if (contentSize <= BLOCKSIZE_BYTES)
			bytesToSkip = BLOCKSIZE_BYTES;
	}
	return (bytesToSkip);
}

int findFile(char *file_name, header_t *list) {
	header_t *aux = list;
	while (aux != NULL)
	{
		if (strcmp(file_name, aux->name) == 0)
			return (1);
		aux = aux->next;
	}
	return (0);
}

void sortFileList(file_t **ptrFilesFound) {
    if (*ptrFilesFound == NULL || (*ptrFilesFound)->next == NULL)
		return;

    file_t *current = *ptrFilesFound;
    while (current != NULL)
	{
		file_t *minNode = current;
		file_t *temp = current->next;

		while (temp != NULL)
		{
			if (strcmp(temp->name, minNode->name) < 0)
				minNode = temp;
			temp = temp->next;
		}

		if (minNode != current)
		{
			char *tempName = current->name;
			current->name = minNode->name;
			minNode->name = tempName;
		}
		current = current->next;
	}
}

int checkTruncatedFile(FILE *tarArchive) {
	long int currentPosition = ftell(tarArchive);
    fseek(tarArchive, 0, SEEK_END);
    long int fileSize = ftell(tarArchive);
    fseek(tarArchive, currentPosition, SEEK_SET);

    if (currentPosition > fileSize)
		return (-1);
	return (0);
}

void freeList(header_t *list) {
    header_t *current = list;
    while (current != NULL)
	{
		header_t *next = current->next;
		free(current);
		current = next;
	}
}

void freeFileList(file_t *list) {
    file_t *current = list;
    while (current != NULL)
	{
		file_t *next = current->next;
		free(current);
		current = next;
	}
}

int openTarArchive(FILE *tarArchive, char *tarArchiveName) {
	tarArchive = fopen(tarArchiveName, "r");
	if (tarArchive == NULL)
	{
		return (-1);
	}
	return (0);
}

int readTarArchive(FILE *tarArchive,
					header_t *list,
					int *posZeroBlock,
					bool *isLoneZeroBlock) {
	while (1)
	{
		header_t *new_header = createHeader();
		size_t element_read = fread((new_header->block),
								sizeof (new_header->block),
								1, tarArchive);
		if (element_read != 1)
			break;

		if (isZeroBlock(new_header) == true)
		{
			if (fread((new_header->block),
						sizeof (new_header->block),
						1, tarArchive) != 1)
			{
				(*posZeroBlock)++;
				(*isLoneZeroBlock) = true;
			}
			break;
		}

		if ((new_header->typeflag != REGTYPE) &&
			new_header->typeflag != AREGTYPE)
		{
			printf(MSG_PREFFIX " Unsupported header type: %d\n",
									new_header->typeflag);
			return (-1);
		}

		list = addHeader(list, new_header);

		long int bytesToSkip = countBytesToSkip(new_header);
		fseek(tarArchive, bytesToSkip, SEEK_CUR);

		while (bytesToSkip > 0)
		{
			bytesToSkip -= BLOCKSIZE_BYTES;
			(*posZeroBlock)++;
		}

		if (checkTruncatedFile(tarArchive) == -1)
		{
			printf("%s\n", new_header->name);
			printf(MSG_PREFFIX " Unexpected EOF in archive\n");
			printf(MSG_PREFFIX " Error is not recoverable: exiting now\n");
			return (-2);
		}
	}
	fclose(tarArchive);
	return (0);
}

int main(int argc, char *argv[]) {
	if (argc < MIN_NUM_OF_ARGUMENTS)
		exit(ERROR_CODE_TWO);
    int f = 0;
    int t = 0;
	char *tarArchiveName = NULL;
	char **fileNames = NULL;
	int numFileNames = 0;

    for (int i = 1; i < argc; i++)
	{
		if (argv[i][0] == '-')
		{
			switch (argv[i][1])
			{
			case 'f':
				f = 1;
				if (strncmp(argv[i+1], "-", 1) != 0)
				{
					tarArchiveName = argv[i+1];
					i++;
				}
				else
				{
					printf(MSG_PREFFIX " option requires an argument -- 'f'\n"
						"Try './mytar --help' or './mytar --usage' for more information.\n");
					exit(ERROR_CODE_TWO);
				}
				break;
			case 't':
				t = 1;
				break;
			default:
				printf(MSG_PREFFIX " Unknown option: %s\n", argv[i]);
				exit(ERROR_CODE_TWO);
			}
		} else if (f || t)
		{
			fileNames = realloc(fileNames, (numFileNames + 1) * sizeof (char *));
			fileNames[numFileNames] = argv[i];
			numFileNames++;
		}
		else
		{
			printf(MSG_PREFFIX " You must specify one of the '-Acdtrux', '--delete' or '--test-label' options\n"
					"Try 'tar --help' or 'tar --usage' for more information.\n");
			exit(ERROR_CODE_TWO);
		}
	}

	FILE * tarArchive = NULL;
	int filesFoundCount = 0;
	int filesNotFoundCount = 0;
	file_t * filesFound = NULL;
	file_t * filesNotFound = NULL;

	if (f && t)
	{
		header_t * list = NULL;
		int posZeroBlock = 1;
		bool isLoneZeroBlock = false;

		if (openTarArchive(tarArchive, tarArchiveName) == -1)
		{
			printf(MSG_PREFFIX " %s file does not exist in current directory\n", argv[2]);
			exit(ERROR_CODE_TWO);
		}

		int readResult = readTarArchive(tarArchive, list, &posZeroBlock, &isLoneZeroBlock);
		if (readResult == -1)
			exit(ERROR_CODE_TWO);
		else if (readResult == -2)
			exit(ERROR_CODE_TWO);

		if (numFileNames == 0) printNameHeaders(list);
		else
		{
			for (int i = 0; i < numFileNames; i++)
			{
				char *fileName = fileNames[i];
				if (findFile(fileName, list) == 1)
				{
					file_t *new_file = createFile(fileName);
					filesFound = addFile(filesFound, new_file);
					filesFoundCount++;
				}
				else
				{
					file_t *new_file = createFile(fileName);
					filesNotFound = addFile(filesNotFound, new_file);
					filesNotFoundCount++;
				}
			}

			if (filesFoundCount > 0)
			{
				file_t **ptrFilesFound = &filesFound;
				sortFileList(ptrFilesFound);
				printNameFiles(filesFound, filesNotFoundCount);
			}

			if (filesNotFoundCount > 0)
			{
				file_t *current = filesNotFound;
				while (current != NULL)
				{
					fprintf(stderr, MSG_PREFFIX " %s: Not found in archive\n", current->name);
					current = current->next;
				}
				fprintf(stderr, MSG_PREFFIX " Exiting with failure status due to previous errors\n");
				exit(ERROR_CODE_TWO);
			}
		}

		if (isLoneZeroBlock == true)
			printf(MSG_PREFFIX " A lone zero block at %d\n", posZeroBlock);

		freeList(list);
		freeFileList(filesFound);
		freeFileList(filesNotFound);
	} else if (f && !t)
	{
		printf(MSG_PREFFIX " option requires an argument -- 'f'\n"
				"Try './mytar --help' or './mytar --usage' for more information.\n");
		exit(ERROR_CODE_TWO);
	} else if (!f && t)
	{
		printf(MSG_PREFFIX " Refusing to read archive contents from terminal (missing -f option?)\n"
				MSG_PREFFIX " Error is not recoverable: exiting now\n");
		exit(ERROR_CODE_TWO);
	}

	return (0);
}
