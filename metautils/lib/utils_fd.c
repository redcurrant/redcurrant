#ifndef G_LOG_DOMAIN
#define G_LOG_DOMAIN "metautils.fd"
#endif

#include <errno.h>
#include <glib.h>

#include "metautils.h"

#define METAUTILS_FD_BUFFER_SIZE 32768

GError*
metautils_read_fd(int fd, guint64 size, GByteArray *gba)
{
	ssize_t r;
	guint64 tot = 0;
	guint8 *d;
	GError *err = NULL;

	d = g_malloc(METAUTILS_FD_BUFFER_SIZE);

	do {
		r = read(fd, d, MIN(size - tot, METAUTILS_FD_BUFFER_SIZE));
		if (r < 0) {
			err = NEWERROR(CODE_INTERNAL_ERROR, "read error: (%d) %s",
					errno, strerror(errno));
		} else if (r > 0) {
			tot += r;
			g_byte_array_append(gba, d, r);
		}
	} while (r > 0 && tot < size && !err);

	g_free(d);
	return err;
}

GError*
metautils_read_fd_full(int fd, GByteArray *gba)
{
	int rc;
	struct stat st;
	GError *err = NULL;

	rc = fstat(fd, &st);
	if (0 > rc)
		return NEWERROR(CODE_INTERNAL_ERROR,
				"failed to stat the input file descriptor: (%d) %s",
				errno, strerror(errno));

	g_byte_array_set_size(gba, st.st_size);
	g_byte_array_set_size(gba, 0);

	err = metautils_read_fd(fd, (guint64)st.st_size, gba);
	return err;
}
