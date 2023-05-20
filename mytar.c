/************************
 * INCLUDES
 ************************/

#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>


/***********************
 * MACROS
************************/

/* Minimun number of arguments for tar command */
#define MIN_NUM_OF_ARGUMENTS  2

/* Message preffix */
#define MSG_PREFFIX           		"mytar:"

/* Options of tar command */
#define OPT_FILENAME          		"-f"
#define OPT_LIST_FILES        		"-t"

/* Block size of blocks of an archive */
#define BLOCKSIZE_BYTES       		512

/* Maximun size of file name */
#define SIZE_NAME_MAX        		100

/* Maximum number of files not found */
#define MAX_FILES_NOT_FOUND			100

/* Octal base */
#define OCTAL_BASE					8

/* Error codes */
#define ERROR_CODE_TWO				2

/* Values for typeflag field */
#define REGTYPE						'0'  // Regular file
#define AREGTYPE					'\0' // Regular file
#define LNKTYPE						'1'  // Link
#define SYMTYPE						'2'  // Reserved
#define CHRTYPE						'3'  // Character special
#define BLKTYPE						'4'  // Block special
#define DIRTYPE						'5'  // Directory
#define FIFOTYPE					'6'  // FIFO special
#define CONTTYPE					'7'  // Reserved
#define XHDTYPE						'x'  // Extended header referring to the next file in the archive
#define XGLTYPE						'g'  // Global extended header



/***********************
 * GLOBAL VARIABLES
 ***********************/
struct header
{
    union
    {
        struct
		{
			char name[100];             // file name
			char mode[8];               // permissions
			char uid[8];                // user id (octal)
			char gid[8];                // group id (octal)
			char size[12];              // size (octal)
			char mtime[12];             // modification time (octal)
			char chksum[8];             // sum of unsigned characters in block, with spaces in the check field while calculation is done (octal)
			char typeflag;        	    // link indicator (one caracter)
			char linkname[100];         // name of linked file
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
};

struct header header;
struct header arrayHeader[100];


/***********************
 * FUNCTIONS PROTOTYPES
************************/
bool isUnknownOptionFound(char *inputArgs[], int numInputArgs);
void orderFilesIndex(char *argv[], int argc);
int checkTruncatedFile(FILE *tarArchive);

/***********************
 * FUNCTIONS
************************/
bool isUnknownOptionFound(char *inputArgs[], int numInputArgs)
{
	for (int i = 0; i < numInputArgs; i++)
	{
		if ((inputArgs[i][0] == '-') &&
			(strcmp(inputArgs[1], OPT_FILENAME) != 0) &&
			(strcmp(inputArgs[1], OPT_LIST_FILES) != 0))
		{
			printf(MSG_PREFFIX " Unknown option: %s\n", inputArgs[1]);
			return true;
		}
	}
	return false;
}

void orderFilesIndex(char *inputArgs[], int numInputArgs)
{
	char* aux = NULL;
	for(int i = 0; i < numInputArgs; i++)
	{
		for(int j = 0; j < numInputArgs - 1; j++)
		{
			if(strcmp(inputArgs[j], inputArgs[j+1]) > 0)
			{
				aux = inputArgs[j];
				inputArgs[j] = inputArgs[j+1];
				inputArgs[j+1] = aux;
			}
		}
	}
}

int checkTruncatedFile(FILE *tarArchive)
{
	long int currentPosition = ftell(tarArchive);
    fseek(tarArchive, 0, SEEK_END);
    long int fileSize = ftell(tarArchive);
    fseek(tarArchive, currentPosition, SEEK_SET);

    if (currentPosition > fileSize) {
        return -1;
    }
	return 0;
}


int main(int argc, char *argv[])
{
	if (isUnknownOptionFound(argv, argc))
	{
		exit(ERROR_CODE_TWO);
	}

	if (argc < MIN_NUM_OF_ARGUMENTS)
	{
		exit(ERROR_CODE_TWO);
	}

	else if (argc == MIN_NUM_OF_ARGUMENTS)
	{
		if(argv[1][0] == '-')
		{
			// Command executed: ./mytar -f
			if (strcmp(argv[1], OPT_FILENAME) == 0)
			{
				printf(MSG_PREFFIX " option requires an argument -- 'f'\n"
					"Try './mytar --help' or './mytar --usage' for more information.\n");
				exit(ERROR_CODE_TWO);
			}

			//Command executed: ./mytar -t
			else if (strcmp(argv[1], OPT_LIST_FILES) == 0)
			{
				printf(MSG_PREFFIX " Refusing to read archive contents from terminal (missing -f option?)\n"
					MSG_PREFFIX " Error is not recoverable: exiting now");
				exit(ERROR_CODE_TWO);
			}
		}
		else
		{
			printf(MSG_PREFFIX " You must specify one of the '-Acdtrux', '--delete' or '--test-label' options\n"
				"Try 'tar --help' or 'tar --usage' for more information.\n");
			exit(ERROR_CODE_TWO);
		}
	}

	else if (argc == 3)
	{
		// Command executed: ./mytar -f <archive-filename>
		if (strcmp(argv[1], OPT_FILENAME) == 0)
		{
			printf(MSG_PREFFIX " option requires an argument -- 'f'\n"
				"Try './mytar --help' or './mytar --usage' for more information.\n");
			exit(ERROR_CODE_TWO);
		}

		// Command executed: ./mytar -t <archive-filename>
		// Command executed: ./mytar <archive-filename> -t
		else if(strcmp(argv[1], OPT_LIST_FILES) == 0 || 
				strcmp(argv[2], OPT_LIST_FILES))
		{
			printf(MSG_PREFFIX " Refusing to read archive contents from terminal (missing -f option?)\n"
				MSG_PREFFIX " Error is not recoverable: exiting now");
			exit(ERROR_CODE_TWO);
		}
	}

	else if (argc == 4)
	{
		FILE * tarArchive = NULL;

        // Command executed: ./mytar -f <archive-filename> -t
		if (strcmp(argv[1], OPT_FILENAME) == 0 &&
			(strcmp(argv[3], OPT_LIST_FILES) == 0))
		{
			tarArchive = fopen(argv[2], "r");

			if (tarArchive == NULL)
			{
				printf(MSG_PREFFIX " %s file does not exist in current directory", argv[2]);
				exit(ERROR_CODE_TWO);
			}
	    }

		// Command executed: ./mytar -t -f <archive-filename>
		else if ((strcmp(argv[2], OPT_FILENAME) == 0) &&
			(strcmp(argv[1], OPT_LIST_FILES) == 0))
		{
			tarArchive = fopen(argv[3], "r");

			if (tarArchive == NULL)
			{
				printf(MSG_PREFFIX " %s file does not exist in current directory", argv[3]);
				exit(ERROR_CODE_TWO);
			}
		}

		int count = 0;
		int posZeroBlock = 1;
		bool isLoneZeroBlock = false;

		while (1)
		{
			if(fread(&arrayHeader[count], sizeof(arrayHeader[count]), 1, tarArchive) != 1)
			{
        		break;
			}

			
			struct header allZeroHeader;
			memset(&allZeroHeader, 0, sizeof(allZeroHeader));

			if(memcmp(&allZeroHeader, &arrayHeader[count], sizeof(allZeroHeader)) == 0)
			{
				if(fread(&arrayHeader[count], sizeof(arrayHeader[count]), 1, tarArchive) != 1)
				{
					posZeroBlock++;
					isLoneZeroBlock = true;
				}
				break;
			}


			if ((arrayHeader[count].typeflag != REGTYPE) &&
				(arrayHeader[count].typeflag != AREGTYPE))
			{
				printf(MSG_PREFFIX " Unsupported header type: %d\n", arrayHeader[count].typeflag);
				exit(ERROR_CODE_TWO);
			}


			long int contentSize = strtol(arrayHeader[count].size, NULL, OCTAL_BASE);
			long int bytesToSkip = 0;

			if(contentSize > 0)
			{
				if(contentSize > BLOCKSIZE_BYTES)
				{
					bytesToSkip = (contentSize/BLOCKSIZE_BYTES) * BLOCKSIZE_BYTES;
					if((contentSize % BLOCKSIZE_BYTES) != 0)
						bytesToSkip = ((contentSize/BLOCKSIZE_BYTES) * BLOCKSIZE_BYTES) + BLOCKSIZE_BYTES;
				}
				else if(contentSize <= BLOCKSIZE_BYTES)
					bytesToSkip = BLOCKSIZE_BYTES;
			}

			fseek(tarArchive, bytesToSkip, SEEK_CUR);

			while(bytesToSkip > 0)
			{
				bytesToSkip -= BLOCKSIZE_BYTES;
				posZeroBlock++;
			}


			if (checkTruncatedFile(tarArchive) == -1)
			{
				printf("%s\n", arrayHeader[count].name);
				printf(MSG_PREFFIX " Unexpected EOF in archive\n");
				printf(MSG_PREFFIX " Error is not recoverable: exiting now\n");
				exit(ERROR_CODE_TWO);
			}
			count++;
		}
		fclose(tarArchive);

		for (int i = 0; i < count; i++)
		{
			printf("%s\n", arrayHeader[i].name);
		}
		
		if(isLoneZeroBlock == true)
			printf(MSG_PREFFIX " A lone zero block at %d\n", posZeroBlock);
	}

	else
	{
		FILE * tarArchive = NULL;
		int filesIndexStart = 0;
		int filesIndexEnd = 0;

		// Command executed: ./mytar  -t -f <archive-filename> [filenames...]
		if((strcmp(argv[2], OPT_FILENAME) == 0) &&
			(strcmp(argv[1], OPT_LIST_FILES)) == 0)
		{
			tarArchive = fopen(argv[3], "r");

			if (tarArchive == NULL)
			{
				printf(MSG_PREFFIX " %s file does not exist in current directory\n", argv[3]);
				exit(ERROR_CODE_TWO);
			}

			filesIndexStart = 4;
			filesIndexEnd = argc - 1;
		}

		// Command executed: ./mytar -f <archive-filename> -t [filenames...]
		else if((strcmp(argv[1], OPT_FILENAME) == 0) &&
			(strcmp(argv[3], OPT_LIST_FILES)) == 0)
		{
			tarArchive = fopen(argv[2], "r");

			if (tarArchive == NULL)
			{
				printf(MSG_PREFFIX " %s file does not exist in current directory\n", argv[2]);
				exit(ERROR_CODE_TWO);
			}

			filesIndexStart = 4;
			filesIndexEnd = argc - 1;			
		}

		// Command executed: ./mytar -t [filenames...] -f <archive-filename>
		else if((strcmp(argv[1], OPT_FILENAME) == 0) &&
			(strcmp(argv[3], OPT_LIST_FILES)) == 0)
		{
			tarArchive = fopen(argv[4], "r");

			if (tarArchive == NULL)
			{
				printf(MSG_PREFFIX " %s file does not exist in current directory\n", argv[argc-1]);
				exit(ERROR_CODE_TWO);
			}

			filesIndexStart = 2;
			filesIndexEnd = argc - 3;
		}	

		//Command executed: ./mytar -f <archive-filename> [filenames...] -t
		else if ((strcmp(argv[1], OPT_FILENAME) == 0) &&
			(strcmp(argv[argc - 1], OPT_LIST_FILES) == 0))
		{
			tarArchive = fopen(argv[2], "r");

			if (tarArchive == NULL)
			{
				printf(MSG_PREFFIX " %s file does not exist in current directory\n", argv[2]);
				exit(ERROR_CODE_TWO);
			}

			filesIndexStart = 3;
			filesIndexEnd = argc - 2;
		}

		else
		{
			printf(MSG_PREFFIX " You must specify one of the '-Acdtrux', '--delete' or '--test-label' options\n"
				"Try 'tar --help' or 'tar --usage' for more information.\n");
			exit(ERROR_CODE_TWO);
		}

		int count = 0;
		int posZeroBlock = 1;
		bool isLoneZeroBlock = false;

		while (1)
		{
			if(fread(&arrayHeader[count], sizeof(arrayHeader[count]), 1, tarArchive) != 1)
			{
        		break;
			}

			
			struct header allZeroHeader;
			memset(&allZeroHeader, 0, sizeof(allZeroHeader));

			if(memcmp(&allZeroHeader, &arrayHeader[count], sizeof(allZeroHeader)) == 0)
			{
				if(fread(&arrayHeader[count], sizeof(arrayHeader[count]), 1, tarArchive) != 1)
				{
					posZeroBlock++;
					isLoneZeroBlock = true;
				}
				break;
			}


			if ((arrayHeader[count].typeflag != REGTYPE) &&
				(arrayHeader[count].typeflag != AREGTYPE))
			{
				printf(MSG_PREFFIX " Unsupported header type: %d\n", arrayHeader[count].typeflag);
				exit(ERROR_CODE_TWO);
			}


			long int contentSize = strtol(arrayHeader[count].size, NULL, OCTAL_BASE);
			long int bytesToSkip = 0;

			if(contentSize > 0)
			{
				if(contentSize > BLOCKSIZE_BYTES)
				{
					bytesToSkip = (contentSize/BLOCKSIZE_BYTES) * BLOCKSIZE_BYTES;
					if((contentSize % BLOCKSIZE_BYTES) != 0)
						bytesToSkip = ((contentSize/BLOCKSIZE_BYTES) * BLOCKSIZE_BYTES) + BLOCKSIZE_BYTES;
				}
				else if(contentSize <= BLOCKSIZE_BYTES)
					bytesToSkip = BLOCKSIZE_BYTES;
			}

			fseek(tarArchive, bytesToSkip, SEEK_CUR);

			while(bytesToSkip > 0)
			{
				bytesToSkip -= BLOCKSIZE_BYTES;
				posZeroBlock++;
			}


			if (checkTruncatedFile(tarArchive) == -1)
			{
				printf("%s\n", arrayHeader[count].name);
				printf(MSG_PREFFIX " Unexpected EOF in archive\n");
				printf(MSG_PREFFIX " Error is not recoverable: exiting now\n");
				exit(ERROR_CODE_TWO);
			}
			count++;
		}
		fclose(tarArchive);

		int filesFoundCount = 0;
		int filesNotFoundCount = 0;
		char * filesFound[100];
		char * filesNotFound[100];
		
		for(int i = filesIndexStart; i <= filesIndexEnd; i++)
		{
			for(int j = 0; j < count; j++)
			{
				if(strcmp(argv[i], arrayHeader[j].name) == 0)
				{
					filesFound[filesFoundCount] = argv[i];
					filesFoundCount++;
					break;
				}
				else if((strcmp(argv[i], arrayHeader[j].name) != 0) &&
						(j == (count - 1))) //No se ha encontrado en todo el array
				{
					filesNotFound[filesNotFoundCount] = argv[i];
					filesNotFoundCount++;
				}
			}
		}
		if(filesFoundCount > 0)
		{
			orderFilesIndex(filesFound, filesFoundCount);

			for (int i = 0; i < filesFoundCount; i++)
			{
				if(filesNotFoundCount > 0)
				{
					fprintf(stderr, "%s\n", filesFound[i]);
				}
				else
				{
					printf("%s\n", filesFound[i]);
				}
			}
		}

		if(filesNotFoundCount > 0)
		{
			for(int i = 0; i < filesNotFoundCount; i++)
			{
				fprintf(stderr, MSG_PREFFIX " %s: Not found in archive\n", filesNotFound[i]);
			}
			fprintf(stderr, MSG_PREFFIX " Exiting with failure status due to previous errors\n");
			exit(ERROR_CODE_TWO);
		}

		if(isLoneZeroBlock == true)
			printf(MSG_PREFFIX " A lone zero block at %d\n", posZeroBlock);
	}
	return 0;
}
