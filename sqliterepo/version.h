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

/**
 * @file version.h
 */

#ifndef HC__SQLX_VERSION__H
# define HC__SQLX_VERSION__H 1
# include <glib.h>

/**
 * @defgroup sqliterepo_version Databases versioning
 * @ingroup sqliterepo
 * @brief
 * @details
 *
 * @{
 */

struct TableSequence;

/**
 *
 */
struct object_version_s
{
	gint64 version; /**<  */
	gint64 when;    /**<  */
};

struct sqlx_sqlite3_s;

/** Wraps version_extract_from_admin_tree() called on the admin table
 * cache. */
GTree* version_extract_from_admin(struct sqlx_sqlite3_s *sq3);

/** For testing purposes, prefer version_extract_from_admin()
 * for production code.
 * @see version_extract_from_admin() */
GTree* version_extract_from_admin_tree(GTree *t);

/**
 * @param t
 * @return
 */
gchar* version_dump(GTree *t);

/**
 * @param tag
 * @param versions
 */
void version_debug(const gchar *tag, GTree *sq3);

/**
 * @param t
 */
void version_increment_all(GTree *t);

/**
 * Computes what would be the version if the 'changes' were applied to a
 * base with the 'current' version.
 *
 * @param current
 * @param changes
 * @return
 */
GTree* version_extract_expected(GTree *current, struct TableSequence *changes);

/**
 * Compute the diff between both versions, and returns an error if the worst
 * version is > 1 in basolute value.
 *
 * @param src
 * @param dst
 * @param worst the worst difference matched, with the considering 'src - dst'
 * @return the error that occured
 */
GError* version_validate_diff(GTree *src, GTree *dst, gint64 *worst);

GTree* version_empty(void);

/**
 * @param t
 * @return
 */
GByteArray* version_encode(GTree *t);

/**
 * @param raw
 * @param rawsize
 * @return
 */
GTree* version_decode(guint8 *raw, gsize rawsize);

/**
 * @param version
 * @return
 */
GTree* version_dup(GTree *version);

/** @} */

#endif
