/*
 * Copyright (C) 2015 Worldline
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as
 * published by the Free Software Foundation, either version 3 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef __REDCURRANT_metatype_metadata__h
#define __REDCURRANT_metatype_metadata__h 1
#include <glib/gtypes.h>

/**
 * @defgroup metautils_metadata Metadata
 * @ingroup metautils_utils
 * These functions handle GHashTable of (gchar*,gchar*)
 * @{
 */

/**
 * @return
 */
GHashTable* metadata_create_empty(void);


/**
 * @param gba
 * @param error
 * @return
 */
GHashTable* metadata_unpack_gba(GByteArray *gba, GError **error);


/**
 * @param data
 * @param size
 * @param error
 * @return
 */
GHashTable* metadata_unpack_buffer(const guint8 *data, gsize size, GError **error);


/**
 * @param data
 * @param error
 * @return
 */
GHashTable* metadata_unpack_string(const gchar *data, GError **error);


/**
 * @param unpacked
 * @param error
 * @return
 */
GByteArray* metadata_pack(GHashTable *unpacked, GError **error);


/**
 * Returns if given metadata strings contain the same key/values.
 *
 * @param md1 first metadata to compare
 * @param md2 second metadata to compare
 * @param diff if not NULL, contains keys for which the value differs
 * @return TRUE if both metadata are equal, FALSE otherwise
 */
gboolean metadata_equal(const gchar *md1, const gchar *md2, GSList **diff);


/**
 * @param unpacked
 * @param prefix
 * @param error
 * @return
 */
GHashTable* metadata_remove_prefixed(GHashTable *unpacked, const gchar *prefix, GError **error);


/**
 * @param unpacked
 * @param prefix
 * @param error
 * @return
 */
GHashTable* metadata_extract_prefixed(GHashTable *unpacked, const gchar *prefix, GError **error);


/**
 * @param base
 * @param complement
 */
void metadata_merge(GHashTable *base, GHashTable *complement);


/**
 * @param md
 * @param key
 * @param t
 */
void metadata_add_time(GHashTable *md, const gchar *key, GTimeVal *t); 


/**
 * @param md
 * @param key
 * @param fmt
 * @param ...
 */
void metadata_add_printf(GHashTable *md, const gchar *key, const gchar *fmt, ...);

/** @} */

#endif // __REDCURRANT_metatype_metadata__h
