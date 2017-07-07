
#define USE_COMPRESSION

#include "pgen.h"
#ifdef USE_COMPRESSION
#include <sys/stat.h>
#include <sys/types.h>
#endif

#ifdef WIN32
#include "..\..\sdk\zlib\unzip.h"
#else
#include "../zlib/contrib/minizip/unzip.h"
#endif

#define ZIP_FILE "compressed.zip"

#ifdef WIN32
#ifdef _DEBUG
#pragma comment(lib, "..\\..\\sdk\\zlib\\zlibstatd.lib")
#else
#pragma comment(lib, "..\\..\\sdk\\zlib\\zlibstat.lib")
#endif
#endif

char _load_compressed(unzFile fin, char** bytes, int* size)
{
	unz_file_info fi;
	unzGetCurrentFileInfo(fin,&fi,0,0,0,0,0,0);
	if (!fi.uncompressed_size)
		return TRUE;
	*bytes = new char[fi.uncompressed_size];
	if (unzOpenCurrentFile(fin) != UNZ_OK)
		return FALSE;
	*size = fi.uncompressed_size;
	unzReadCurrentFile(fin, *bytes, *size);
	unzCloseCurrentFile(fin);
	return TRUE;
}

char load_compressed(const char* filename, char** bytes, int* size)
{
#ifdef USE_COMPRESSION
	*bytes = 0; *size = 0;
	unzFile fin = unzOpen(app_path);
	if (!fin) return FALSE;
	if (unzLocateFile(fin, filename, 2) != UNZ_OK) {
		unzClose(fin);
		return FALSE;
	}
	char retval = _load_compressed(fin, bytes, size);
	unzClose(fin);
	if (!retval && *bytes) {
		delete[] *bytes;
		*bytes=0;
	}
	return retval;
#else
	struct _stat st;
	FILE* fin;
	if (_stat(filename, &st))
		return FALSE;
	*bytes = new char[st.st_size];
	*size = st.st_size;
	fin = fopen(filename, "rb");
	fread(*bytes, st.st_size, 1, fin);
	fclose(fin);
	return TRUE;
#endif
}

char dir_compressed(compressed_cb cbfunc)
{
	unzFile fin = unzOpen(app_path);
	if (!fin) return FALSE;

	if (unzGoToFirstFile(fin) == UNZ_OK)
	{
		char fn[256];
		do {
			unzGetCurrentFileInfo(fin, 0, fn, 256, 0,0,0,0);
			if (!cbfunc(fn))
				break;
		} while(unzGoToNextFile(fin) == UNZ_OK);
	}
	unzClose(fin);
	return TRUE;
}
