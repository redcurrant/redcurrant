#ifndef __REDCURRANT__metautils_fd__h
# define __REDCURRANT__metautils_fd__h 1

#include <glib.h>

/**
 * Read part of an open file into a GByteArray.
 */
GError* metautils_read_fd(int fd, guint64 size, GByteArray *gba);

/**
 * Read the whole content of an open file into a GByteArray.
 */
GError* metautils_read_fd_full(int fd, GByteArray *gba);

#endif
