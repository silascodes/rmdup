/* rmdup.c -- duplicate file removal tool
  version beta-2, October 7th, 2012

  Silas Normanton
  silas.normanton@gmail.com

*/

/* PORTING NOTES
 *	Theres minimal Win32 specific code for scanning files and directories, everything else should be fine.
 *	Search WIN32 to find the Win32 specific code.
 */

#include <stdio.h>
#include <stdarg.h>
#include <windows.h>	/* WIN32 specific */

#include "md5.h"

#define RMDUP_VERSION		"beta-2"
#define RMDUP_MAXPATH		1024
#define RMDUP_BUFSIZE		2048        /* After some profiling this seemed most optimal, on my AMD FX box hitting RAID0 SATA3 drives */

#define RMDUP_FLAG_RECURSE	(1 << 0)
#define RMDUP_FLAG_SAFE		(1 << 1)
#define RMDUP_FLAG_QUIET	(1 << 2)

typedef struct hashlist_entry_s {
	unsigned char md5[16];
	struct hashlist_entry_s *next;
} hashlist_entry;

hashlist_entry *listbase = NULL;

unsigned int dirsProcessed = 0;
unsigned int filesProcessed = 0;
unsigned int dupsFound = 0;

int flags = 0;

void rmdup_print(char *str, ...) {
	va_list vl;
	char buf[RMDUP_BUFSIZE];

	memset(buf, 0, RMDUP_BUFSIZE);

	va_start(vl, str);
	vsprintf(buf, str, vl);
	va_end(vl);

	if(!(flags & RMDUP_FLAG_QUIET)) {
		printf("%s", buf);
	}
}

void title() {
	rmdup_print("rmdup - remove duplicate files - version %s\nCopyright (C) 2012 Silas Normanton (silas.normanton@gmail.com)\n\n", RMDUP_VERSION);
}

void usage() {
	title();
	rmdup_print("Usage: rmdup [-rsqh] path\n\t-r\trecurse through subdirectories\n\t-s\tsafe mode, don't actually remove any files\n\t-q\tquiet mode, don't output anything\n\t-h\tshow this help screen\n\n");
}

void results() {
	rmdup_print("\nProcessed %d directories containing %d files\n%d duplicate files found\n", dirsProcessed, filesProcessed, dupsFound);
}

int md5file(char *path, unsigned char *md5) {
	FILE *fp;
	unsigned int len;
	unsigned char buf[RMDUP_BUFSIZE];
	MD5_CTX ctx;

	MD5_Init(&ctx);

	fp = fopen(path, "rb");
	if(!fp) {
		return 1;
	}

	while(!feof(fp)) {
		len = fread(buf, 1, RMDUP_BUFSIZE, fp);
		MD5_Update(&ctx, buf, len);
	}

	fclose(fp);
	MD5_Final(md5, &ctx);

	return 0;
}

int containsmd5(unsigned char *md5) {
	hashlist_entry *list = listbase;

	if(list != NULL) {
		do {
			if(memcmp(list->md5, md5, 16) == 0) {
				return 1;
			}
			list = list->next;
		} while(list != NULL);
	}

	return 0;
}

void addmd5(unsigned char *md5) {
	hashlist_entry *list = listbase;

	if(listbase == NULL) {
		listbase = (hashlist_entry*)malloc(sizeof(hashlist_entry));
		if(listbase == NULL) {
			rmdup_print("Error: memory allocation failure\n");
		}

		memcpy(listbase->md5, md5, 16);
		listbase->next = NULL;
	}
	else {
		while(list->next != NULL) {
			list = list->next;
		}

		list->next = (hashlist_entry*)malloc(sizeof(hashlist_entry));
		if(list->next == NULL) {
			rmdup_print("Error: memory allocation failure\n");
		}

		list = list->next;
		memcpy(list->md5, md5, 16);
		list->next = NULL;
	}
}

void freelist() {
	hashlist_entry *next, *list;

	list = listbase;
	while(list != NULL) {
		next = list->next;
		free(list);
		list = next;
	}
}

/* WIN32 specific stuff in here, enjoy... */
void rmdup(char *path) {
	HANDLE h;
	WIN32_FIND_DATA findData;
	char findPath[RMDUP_MAXPATH], dirPath[RMDUP_MAXPATH];
	unsigned char md5[16];

	/* Make working copy of path, append \* */
	memcpy(findPath, path, RMDUP_MAXPATH);
	strncat(findPath, "\\*", 2);

	rmdup_print("Processing directory %s\n", path);
	dirsProcessed += 1;

	/* Process all entries in the directory specified in path */
	h = FindFirstFileEx(findPath, FindExInfoStandard, &findData, FindExSearchNameMatch, NULL, 0);/*FIND_FIRST_EX_LARGE_FETCH);*/
	if(h == INVALID_HANDLE_VALUE) {
		rmdup_print("Error: failed to scan path %s\n", path);
		return;
	}
	do {
		if(findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
			/* Process directories, recursing if we need to */
			if(strcmp(findData.cFileName, ".") == 0 || strcmp(findData.cFileName, "..") == 0) {
				/* Ignore . (current) and .. (1 level up) */
				continue;
			}
			if(flags & RMDUP_FLAG_RECURSE) {
				/* Prepare the new path then recurse to it */
				memcpy(dirPath, path, RMDUP_MAXPATH);
				strncat(dirPath, "\\", 1);
				strcat(dirPath, findData.cFileName);

				rmdup(dirPath);
			}
		}
		else if(!(findData.dwFileAttributes & FILE_ATTRIBUTE_HIDDEN) && !(findData.dwFileAttributes & FILE_ATTRIBUTE_SYSTEM)) {
			/* Get the full absolute path to the file */
			memcpy(dirPath, path, RMDUP_MAXPATH);
			strncat(dirPath, "\\", 1);
			strcat(dirPath, findData.cFileName);

			filesProcessed += 1;

			/* Calculate the CRC32 checksum for the file, if it's in our list delete the file, otherwise add the CRC to our list */
			md5file(dirPath, md5);
			if(containsmd5(md5) == 0) {
				addmd5(md5);
			}
			else {
				if(!(flags & RMDUP_FLAG_SAFE)) {
					DeleteFile(dirPath);
				}
				rmdup_print("Duplicate: %s\n", findData.cFileName);
				dupsFound += 1;
			}
		}
	} while(FindNextFile(h, &findData) != FALSE);
	FindClose(h);
}

int main(int argc, char **argv) {
	int i, j;
	char path[RMDUP_MAXPATH];

	memset(path, 0, RMDUP_MAXPATH);

	if(argc < 2) {
		usage();
	}
	else {
		memset(path, 0, RMDUP_MAXPATH);

		/* Process command line */
		for(i = 1; i < argc; i++) {
			if(argv[i][0] == '-') {
				for(j = 1; j < strlen(argv[i]); j++) {
					if(argv[i][j] == 'r') {
						flags |= RMDUP_FLAG_RECURSE;
					}
					else if(argv[i][j] == 'h') {
						usage();
						return;
					}
					else if(argv[i][j] == 's') {
						flags |= RMDUP_FLAG_SAFE;
					}
					else if(argv[i][j] == 'q') {
						flags |= RMDUP_FLAG_QUIET;
					}
				}
			}
			else {
				strncpy(path, argv[i], RMDUP_MAXPATH);
			}
		}

		title();

		/* Check we actually got a path */
		i = strlen(path);
		if(strlen(path) == 0) {
			rmdup_print("Error: no path specified\n");
			return -1;
		}

		/* Strip trailing slashes from path */
		i -= 1;
		if(path[i] == '\\' || path[i] == '/') {
			path[i] = 0;
		}

		/* Validate path */
		i = GetFileAttributes(path); /* WIN32 specific */
		if(i == INVALID_FILE_ATTRIBUTES || !(i & FILE_ATTRIBUTE_DIRECTORY)) {
			rmdup_print("Error: specified path is not a valid directory\n");
			return -1;
		}

		/* Process the specified directory */
		rmdup(path);

		results();
		freelist();
	}

	return 0;
}
