/*
   Copyright (c) 2021 Sibi Siddharthan

   Distributed under the MIT license.
   Refer to the LICENSE file at the root directory of the parent project
   for details.
*/


#include <dirent.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <wchar.h>
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

_WDIR *_wopendir(const wchar_t *name)
{
	if (name == NULL)
	{
		errno = ENOENT;
		return NULL;
	}

	int length = wcslen(name);
	wchar_t last = name[length - 1];
	wchar_t *name_proper = (wchar_t *)malloc(sizeof(wchar_t) * (length + 3)); // '/' + '*' + '\0'
	wcscpy(name_proper, name);

	// Append "/*" or "*" as needed
	if (last == L'/' || last == L'\\')
	{
		wcscat(name_proper, L"*");
	}
	else
	{
		wcscat(name_proper, L"/*");
	}

	HANDLE directory_handle;
	WIN32_FIND_DATAW find_file;
	_WDIR *dirp = NULL;

	directory_handle = FindFirstFileExW(name_proper, FindExInfoStandard, &find_file, FindExSearchNameMatch, NULL, FIND_FIRST_EX_LARGE_FETCH);

	if (directory_handle == INVALID_HANDLE_VALUE)
	{
		errno = ENOENT;
		return NULL;
	}

	dirp = (_WDIR *)malloc(sizeof(_WDIR));
	dirp->d_handle = directory_handle;
	dirp->data = (WIN32_FIND_DATAW *)malloc(sizeof(WIN32_FIND_DATAW));
	memcpy(dirp->data, &find_file, sizeof(WIN32_FIND_DATAW));
	dirp->_dirent = (struct _wdirent *)malloc(sizeof(struct _wdirent));
	dirp->offset = 0;

	free(name_proper);
	return dirp;
}

struct _wdirent *_wreaddir(_WDIR *dirp)
{
	if (dirp == NULL)
	{
		errno = EBADF;
		return NULL;
	}

	if (dirp->offset != 0)
	{
		if (!FindNextFileW(dirp->d_handle, dirp->data))
		{
			return NULL;
		}
	}

	dirp->_dirent->d_ino = 0;
	dirp->_dirent->d_off = dirp->offset;
	dirp->_dirent->d_reclen = sizeof(struct _wdirent);
	wcscpy(dirp->_dirent->d_name, dirp->data->cFileName);

	DWORD attributes = dirp->data->dwFileAttributes;
	if (attributes & FILE_ATTRIBUTE_REPARSE_POINT)
	{
		dirp->_dirent->d_type = DT_LNK;
	}
	else if (attributes & FILE_ATTRIBUTE_DIRECTORY)
	{
		dirp->_dirent->d_type = DT_DIR;
	}
	else if ((attributes & ~(FILE_ATTRIBUTE_READONLY | FILE_ATTRIBUTE_HIDDEN | FILE_ATTRIBUTE_SYSTEM | FILE_ATTRIBUTE_ARCHIVE |
							 FILE_ATTRIBUTE_NORMAL | FILE_ATTRIBUTE_TEMPORARY | FILE_ATTRIBUTE_SPARSE_FILE | FILE_ATTRIBUTE_COMPRESSED |
							 FILE_ATTRIBUTE_NOT_CONTENT_INDEXED | FILE_ATTRIBUTE_ENCRYPTED)) == 0)
	{
		dirp->_dirent->d_type = DT_REG;
	}
	else
	{
		dirp->_dirent->d_type = DT_UNKNOWN;
	}

	dirp->offset++;
	return dirp->_dirent;
}

int _wclosedir(_WDIR *dirp)
{
	if (dirp == NULL)
	{
		errno = EBADF;
		return -1;
	}

	if (FindClose(dirp->d_handle))
	{
		// Free the memory of DIR and it's components
		free(dirp->_dirent);
		free(dirp->data);
		free(dirp);
		return 0;
	}
	return -1;
}

DIR *opendir(const char *name)
{
	if (name == NULL)
	{
		errno = ENOENT;
		return NULL;
	}

	int length = strlen(name);
	char last = name[length - 1];
	char *name_proper = (char *)malloc(sizeof(char) * (length + 3)); // '/' + '*' + '\0'
	strcpy(name_proper, name);

	// Append "/*" or "*" as needed
	if (last == '/' || last == '\\')
	{
		strcat(name_proper, "*");
	}
	else
	{
		strcat(name_proper, "/*");
	}

	HANDLE directory_handle;
	WIN32_FIND_DATAA find_file;
	DIR *dirp = NULL;

	directory_handle = FindFirstFileExA(name_proper, FindExInfoStandard, &find_file, FindExSearchNameMatch, NULL, FIND_FIRST_EX_LARGE_FETCH);

	if (directory_handle == INVALID_HANDLE_VALUE)
	{
		errno = ENOENT;
		return NULL;
	}

	dirp = (DIR *)malloc(sizeof(DIR));
	dirp->d_handle = directory_handle;
	dirp->data = (WIN32_FIND_DATAA *)malloc(sizeof(WIN32_FIND_DATAA));
	memcpy(dirp->data, &find_file, sizeof(WIN32_FIND_DATAA));
	dirp->_dirent = (struct dirent *)malloc(sizeof(struct dirent));
	dirp->offset = 0;

	free(name_proper);
	return dirp;
}

struct dirent *readdir(DIR *dirp)
{
	if (dirp == NULL)
	{
		errno = EBADF;
		return NULL;
	}

	if (dirp->offset != 0)
	{
		if (!FindNextFileA(dirp->d_handle, dirp->data))
		{
			return NULL;
		}
	}

	dirp->_dirent->d_ino = 0;
	dirp->_dirent->d_off = dirp->offset;
	dirp->_dirent->d_reclen = sizeof(struct dirent);
	strcpy(dirp->_dirent->d_name, dirp->data->cFileName);

	DWORD attributes = dirp->data->dwFileAttributes;
	if (attributes & FILE_ATTRIBUTE_REPARSE_POINT)
	{
		dirp->_dirent->d_type = DT_LNK;
	}
	else if (attributes & FILE_ATTRIBUTE_DIRECTORY)
	{
		dirp->_dirent->d_type = DT_DIR;
	}
	else if ((attributes & ~(FILE_ATTRIBUTE_READONLY | FILE_ATTRIBUTE_HIDDEN | FILE_ATTRIBUTE_SYSTEM | FILE_ATTRIBUTE_ARCHIVE |
							 FILE_ATTRIBUTE_NORMAL | FILE_ATTRIBUTE_TEMPORARY | FILE_ATTRIBUTE_SPARSE_FILE | FILE_ATTRIBUTE_COMPRESSED |
							 FILE_ATTRIBUTE_NOT_CONTENT_INDEXED | FILE_ATTRIBUTE_ENCRYPTED)) == 0)
	{
		dirp->_dirent->d_type = DT_REG;
	}
	else
	{
		dirp->_dirent->d_type = DT_UNKNOWN;
	}

	dirp->offset++;
	return dirp->_dirent;
}

int closedir(DIR *dirp)
{
	if (dirp == NULL)
	{
		errno = EBADF;
		return -1;
	}

	if (FindClose(dirp->d_handle))
	{
		// Free the memory of DIR and it's components
		free(dirp->_dirent);
		free(dirp->data);
		free(dirp);
		return 0;
	}
	return -1;
}
