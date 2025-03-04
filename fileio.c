/* fileio.c: Simple file/directory access functions with error-handling.
 *
 * Copyright (C) 2001-2017 by Brian Raiter and Eric Schmidt, under the
 * GNU General Public License. No warranty. See COPYING for details.
 */

#include	<stdio.h>
#include	<stdlib.h>
#include	<string.h>
#include	<errno.h>
#include	<dirent.h>
#include	<fcntl.h>
#include	<sys/types.h>
#include	<sys/stat.h>
#include	"err.h"
#include	"fileio.h"

/* Determine the proper directory delimiter and mkdir() arguments.
 */
#ifdef _WIN32
#define	DIRSEP_CHAR	'\\'
#define	createdir(name)	(mkdir(name) == 0)
#else
#define	DIRSEP_CHAR	'/'
#define	createdir(name)	(mkdir(name, 0700) == 0)
#endif

/* Determine a compile-time number to use as the maximum length of a
 * path. Use a value of 1023 if we can't get anything usable from the
 * header files.
 */
#include <limits.h>
#if !defined(PATH_MAX) || PATH_MAX <= 0
#  if defined(MAXPATHLEN) && MAXPATHLEN > 0
#    define PATH_MAX MAXPATHLEN
#  else
#    include <sys/param.h>
#    if !defined(PATH_MAX) || PATH_MAX <= 0
#      if defined(MAXPATHLEN) && MAXPATHLEN > 0
#        define PATH_MAX MAXPATHLEN
#      else
#        define PATH_MAX 1023
#      endif
#    endif
#  endif
#endif

#ifdef _WIN32
#include <shlobj.h>
#elif defined(__APPLE__) && defined(__MACH__)
#include <glob.h>
#include <sysdir.h>
#endif

/* The function used to display error messages relating to file I/O.
 */
int fileerr_(char const *cfile, unsigned long lineno,
	     fileinfo *file, char const *msg)
{
    if (msg) {
	err_cfile_ = cfile;
	err_lineno_ = lineno;
	errmsg_(file->name ? file->name : "file error",
		errno ? strerror(errno) : msg);
    }
    return FALSE;
}

/*
 * File-handling functions.
 */

/* Clear the fields of the fileinfo struct.
 */
void clearfileinfo(fileinfo *file)
{
    file->name = NULL;
    file->fp = NULL;
    file->alloc = FALSE;
}

/* The 'x' modifier (C11) in fopen() is not widely supported as of 2020.
 * This hack enables its use regardless of the underlying libc.
 */
static FILE *FOPEN(char const *name, char const *mode)
{
    FILE * file = NULL;
    if (!strcmp(mode, "wx")) {
	int fd = open(name, O_WRONLY | O_CREAT | O_EXCL, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH);
	if (fd != -1)
	    file = fdopen(fd, "w");
    }
    else
	file = fopen(name, mode);

    return file;
}

/* Open a file. If the fileinfo structure does not already have a
 * filename assigned to it, use name (after making an independent
 * copy).
 */
int fileopen(fileinfo *file, char const *name, char const *mode,
	     char const *msg)
{
    int	n;

    if (!file->name) {
	n = strlen(name) + 1;
	if ((file->name = malloc(n))) {
	    memcpy(file->name, name, n);
	    file->alloc = TRUE;
	} else {
	    file->name = (char*)name;
	    file->alloc = FALSE;
	}
    }
    errno = 0;
    file->fp = FOPEN(name, mode);
    if (file->fp)
	return TRUE;
    if (file->alloc) {
	free(file->name);
	file->name = NULL;
	file->alloc = FALSE;
    }
    return fileerr(file, msg);
}

/* Close the file, clear the file pointer, and free the name buffer if
 * necessary.
 */
void fileclose(fileinfo *file, char const *msg)
{
    errno = 0;
    if (file->fp) {
	if (fclose(file->fp) == EOF)
	    fileerr(file, msg);
	file->fp = NULL;
    }
    if (file->alloc) {
	free(file->name);
	file->name = NULL;
	file->alloc = FALSE;
    }
}

/* rewind().
 */
int filerewind(fileinfo *file, char const *msg)
{
    (void)msg;
    rewind(file->fp);
    return TRUE;
}

/* fseek().
 */
int fileskip(fileinfo *file, int offset, char const *msg)
{
    errno = 0;
    if (!fseek(file->fp, offset, SEEK_CUR))
	return TRUE;
    return fileerr(file, msg);
}

/* feof().
 */
int filetestend(fileinfo *file)
{
    int	ch;

    if (feof(file->fp))
	return TRUE;
    ch = fgetc(file->fp);
    if (ch == EOF)
	return TRUE;
    ungetc(ch, file->fp);
    return FALSE;
}

/* read().
 */
int fileread(fileinfo *file, void *data, unsigned long size, char const *msg)
{
    if (!size)
	return TRUE;
    errno = 0;
    if (fread(data, size, 1, file->fp) == 1)
	return TRUE;
    return fileerr(file, msg);
}

/* Read size bytes from the given file into a newly allocated buffer.
 */
void *filereadbuf(fileinfo *file, unsigned long size, char const *msg)
{
    void       *buf;

    if (size == 0) {
        return NULL;
    }

    if (!(buf = malloc(size))) {
	fileerr(file, msg);
	return NULL;
    }
    errno = 0;
    if (fread(buf, size, 1, file->fp) != 1) {
	fileerr(file, msg);
	free(buf);
	return NULL;
    }
    return buf;
}

/* Read one full line from fp and store the first len characters,
 * including any trailing newline.
 */
int filegetline(fileinfo *file, char *buf, int *len, char const *msg)
{
    int	n, ch;

    if (!*len) {
	*buf = '\0';
	return TRUE;
    }
    errno = 0;
    if (!fgets(buf, *len, file->fp))
	return fileerr(file, msg);
    n = strlen(buf);
    if (n == *len - 1 && buf[n] != '\n') {
	do
	    ch = fgetc(file->fp);
	while (ch != EOF && ch != '\n');
    } else
	buf[n--] = '\0';
    *len = n;
    return TRUE;
}

/* write().
 */
int filewrite(fileinfo *file, void const *data, unsigned long size,
	      char const *msg)
{
    if (!size)
	return TRUE;
    errno = 0;
    if (fwrite(data, size, 1, file->fp) == 1)
	return TRUE;
    return fileerr(file, msg);
}

/* Read one byte as an unsigned integer value.
 */
int filereadint8(fileinfo *file, unsigned char *val8, char const *msg)
{
    int	byte;

    errno = 0;
    if ((byte = fgetc(file->fp)) == EOF)
	return fileerr(file, msg);
    *val8 = (unsigned char)byte;
    return TRUE;
}

/* Write one byte as an unsigned integer value.
 */
int filewriteint8(fileinfo *file, unsigned char val8, char const *msg)
{
    errno = 0;
    if (fputc(val8, file->fp) != EOF)
	return TRUE;
    return fileerr(file, msg);
}

/* Read two bytes as an unsigned integer value stored in little-endian.
 */
int filereadint16(fileinfo *file, unsigned short *val16, char const *msg)
{
    int	byte;

    errno = 0;
    if ((byte = fgetc(file->fp)) != EOF) {
	*val16 = (unsigned char)byte;
	if ((byte = fgetc(file->fp)) != EOF) {
	    *val16 |= (unsigned char)byte << 8;
	    return TRUE;
	}
    }
    return fileerr(file, msg);
}

/* Write two bytes as an unsigned integer value in little-endian.
 */
int filewriteint16(fileinfo *file, unsigned short val16, char const *msg)
{
    errno = 0;
    if (fputc(val16 & 0xFF, file->fp) != EOF
			&& fputc((val16 >> 8) & 0xFF, file->fp) != EOF)
	return TRUE;
    return fileerr(file, msg);
}

/* Read four bytes as an unsigned integer value stored in little-endian.
 */
int filereadint32(fileinfo *file, unsigned long *val32, char const *msg)
{
    int	byte;

    errno = 0;
    if ((byte = fgetc(file->fp)) != EOF) {
	*val32 = (unsigned int)byte;
	if ((byte = fgetc(file->fp)) != EOF) {
	    *val32 |= (unsigned int)byte << 8;
	    if ((byte = fgetc(file->fp)) != EOF) {
		*val32 |= (unsigned int)byte << 16;
		if ((byte = fgetc(file->fp)) != EOF) {
		    *val32 |= (unsigned int)byte << 24;
		    return TRUE;
		}
	    }
	}
    }
    return fileerr(file, msg);
}

/* Write four bytes as an unsigned integer value in little-endian.
 */
int filewriteint32(fileinfo *file, unsigned long val32, char const *msg)
{
    errno = 0;
    if (fputc(val32 & 0xFF, file->fp) != EOF
			&& fputc((val32 >> 8) & 0xFF, file->fp) != EOF
			&& fputc((val32 >> 16) & 0xFF, file->fp) != EOF
			&& fputc((val32 >> 24) & 0xFF, file->fp) != EOF)
	return TRUE;
    return fileerr(file, msg);
}

/*
 * Directory-handling functions.
 */

/* Return the size of a buffer big enough to hold a pathname.
 */
int getpathbufferlen(void)
{
    return PATH_MAX;
}

/* Return a buffer big enough to hold a pathname.
 */
char *getpathbuffer(void)
{
    char       *buf;

    if (!(buf = calloc(PATH_MAX + 1, 1)))
	memerrexit();
    return buf;
}

/* Return TRUE if name contains a path but is not a directory itself.
 */
int haspathname(char const *name)
{
    struct stat	st;

    if (!strchr(name, DIRSEP_CHAR))
	return FALSE;
    if (stat(name, &st) || S_ISDIR(st.st_mode))
	return FALSE;
    return TRUE;
}

/* Return a pointer to the filename, skipping over any directories in
 * the front.
 */
char *skippathname(char const *name)
{
    char const *p;

    p = strrchr(name, DIRSEP_CHAR);
    return (char*)(p ? p + 1 : name);
}

/* Append the path and/or file contained in path to dir. If path is
 * an absolute path, the contents of dir are ignored.
 */
int combinepath(char *dest, char const *dir, char const *path)
{
    int	m, n;

    if (path[0] == DIRSEP_CHAR) {
	n = strlen(path);
	if (n > PATH_MAX) {
	    errno = ENAMETOOLONG;
	    return FALSE;
	}
	strcpy(dest, path);
	return TRUE;
    }
    n = strlen(dir);
    if (n >= PATH_MAX) {
	errno = ENAMETOOLONG;
	return FALSE;
    }
    if (dest != dir)
	memcpy(dest, dir, n);
    if (dest[n - 1] != DIRSEP_CHAR)
	dest[n++] = DIRSEP_CHAR;
    m = strlen(path);
    if (m + n + 1 > PATH_MAX) {
	errno = ENAMETOOLONG;
	return FALSE;
    }
    memcpy(dest + n, path, m + 1);
    return TRUE;
}

/* Verify that the given directory exists, or create it if it doesn't
 * and create is set to TRUE.
 */
int finddir(char const *dir, int create)
{
    struct stat	st;

    return stat(dir, &st) ? (create && createdir(dir))
			: S_ISDIR(st.st_mode);
}

/* Obtain save/settings directory paths and place in savedir,
 * settingsdir. root is the root directory of Tile World, and
 * savedir/settingsdir must be buffers of size getpathbufferlen().
 * The old save directory path is used if a directory exists at
 * that location. Otherwise the function looks for system-specific
 * paths. If system-specific paths cannot be found, root/save is
 * used, if it fits in the buffers. If this fails, FALSE is
 * returned.
 */
int get_userdirs(char const *root, char *savedir, char *settingsdir) {
    int founddir = FALSE;
    char *home;

    // Use old path if it still exists
    if ((home = getenv("HOME")) && *home)
        founddir = combinepath(savedir, home, ".tworld") && finddir(savedir, FALSE);
    else
        founddir = combinepath(savedir, root, "save") && finddir(savedir, FALSE);

    if (founddir) {
	strcpy(settingsdir, savedir);
	return TRUE;
    }

#ifdef _WIN32
    // Use Saved Games folder on Windows
    wchar_t *savedgamesdir;
    char path[PATH_MAX];
    if (SHGetKnownFolderPath(&FOLDERID_SavedGames, KF_FLAG_CREATE, NULL, &savedgamesdir) == S_OK) {
	// Convert from UTF-16 to UTF-8
	if (WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS, savedgamesdir, -1, path, sizeof(path), NULL, NULL)) {
	    founddir = combinepath(savedir, path, "Tile World");
	    if (founddir)
		strcpy(settingsdir, savedir);
	}
	CoTaskMemFree(savedgamesdir);
    }
#elif defined(__APPLE__) && defined(__MACH__)
    // Use Application Support directory on macOS
    char path[PATH_MAX];
    sysdir_search_path_enumeration_state state = sysdir_start_search_path_enumeration(SYSDIR_DIRECTORY_APPLICATION_SUPPORT, SYSDIR_DOMAIN_MASK_USER);
    if (sysdir_get_next_search_path_enumeration(state, path)) {
	// Typically path will start with a tilde which must be expanded
	glob_t pglob;
	if (!glob(path, GLOB_TILDE, NULL, &pglob)) {
	    founddir = combinepath(savedir, pglob.gl_pathv[0], "Tile World");
	    if (founddir)
		strcpy(settingsdir, savedir);
	}
	globfree(&pglob);
    }
#elif defined(__unix__)
    // On all other *nix systems, follow XDG base directory spec
    char tmp[PATH_MAX + 1];
    char *xdg_data = getenv("XDG_DATA_HOME");
    char *xdg_config = getenv("XDG_CONFIG_HOME");
    if (xdg_data && xdg_data[0] == DIRSEP_CHAR)
	founddir = combinepath(tmp, xdg_data, "tworld");
    else if (home && *home)
	founddir = combinepath(tmp, home, ".local/share/tworld");
    if (!founddir)
	    goto done;
    if (xdg_config && xdg_config[0] == DIRSEP_CHAR)
	founddir = combinepath(settingsdir, xdg_config, "tworld");
    else if (home && *home)
	founddir = combinepath(settingsdir, home, ".config/tworld");
    else
	founddir = FALSE;
    if (founddir)
	strcpy(savedir, tmp);
#endif

done:
    if (!founddir) {
	// Fallback path
	if (combinepath(savedir, root, "save")) {
	    strcpy(settingsdir, savedir);
	    founddir = TRUE;
	}
    }
    return founddir;
}

/* Return the pathname for a directory and/or filename, using the
 * same algorithm to construct the path as openfileindir().
 */
char *getpathforfileindir(char const *dir, char const *filename)
{
    char       *path;
    int		m, n;

    m = strlen(filename);
    if (!dir || !*dir || strchr(filename, DIRSEP_CHAR)) {
	if (m > PATH_MAX) {
	    errno = ENAMETOOLONG;
	    return NULL;
	}
	path = getpathbuffer();
	strcpy(path, filename);
    } else {
	n = strlen(dir);
	if (m + n + 1 > PATH_MAX) {
	    errno = ENAMETOOLONG;
	    return NULL;
	}
	path = getpathbuffer();
	memcpy(path, dir, n);
	path[n++] = DIRSEP_CHAR;
	memcpy(path + n, filename, m + 1);
    }
    return path;
}

/* Open a file, using dir as the directory if filename is not a path.
 */
int openfileindir(fileinfo *file, char const *dir, char const *filename,
		  char const *mode, char const *msg)
{
    char	buf[PATH_MAX + 1];
    int		m, n;

    if (!dir || !*dir || strchr(filename, DIRSEP_CHAR))
	return fileopen(file, filename, mode, msg);

    n = strlen(dir);
    m = strlen(filename);
    if (m + n + 1 > PATH_MAX) {
	errno = ENAMETOOLONG;
	return fileerr(file, NULL);
    }
    memcpy(buf, dir, n);
    buf[n++] = DIRSEP_CHAR;
    memcpy(buf + n, filename, m + 1);
    return fileopen(file, buf, mode, msg);
}

/* Read the given directory and call filecallback once for each file
 * contained in it.
 */
int findfiles(char const *dir, void *data,
	      int (*filecallback)(char const*, void*))
{
    char	       *filename = NULL;
    DIR		       *dp;
    struct dirent      *dent;
    int			r;

    if (!(dp = opendir(dir))) {
	fileinfo tmp;
	tmp.name = (char*)dir;
	return fileerr(&tmp, "couldn't access directory");
    }

    while ((dent = readdir(dp))) {
	if (dent->d_name[0] == '.')
	    continue;
	x_alloc(filename, strlen(dent->d_name) + 1);
	strcpy(filename, dent->d_name);
	r = (*filecallback)(filename, data);
	if (r < 0)
	    break;
	else if (r > 0)
	    filename = NULL;
    }

    if (filename)
	free(filename);
    closedir(dp);
    return TRUE;
}
