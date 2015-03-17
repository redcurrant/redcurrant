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

#ifndef HC__SQLX_REMOTE_EX_H
# define HC__SQLX_REMOTE_EX_H 1

#include <metautils/lib/metautils.h>
#include <sqliterepo/sqlx_remote.h>

/**
 * Destroy an SQLX database.
 *
 * @param target One of the services managing the database.
 * @param sid Unused
 * @param name The name of the database
 * @param local TRUE to destroy only the database local to the service
 */
GError* sqlx_remote_execute_DESTROY(const gchar *target, GByteArray *sid,
		struct sqlxsrv_name_s *name, gboolean local);

/**
 * Locally destroy an SQLX database on several services.
 *
 * @param targets An array of services managing the database.
 * @param sid Unused
 * @param name The name of the database
 * @param local
 */
GError* sqlx_remote_execute_DESTROY_many(gchar **targets, GByteArray *sid,
		struct sqlxsrv_name_s *name);

/**
 * Get a value from 'admin' table.
 *
 * @param k The key
 * @param[out] v A pointer where to store the value
 */
GError* sqlx_remote_execute_ADMGET(const gchar *target, GByteArray *sid,
		struct sqlx_name_s *name, const gchar *k, gchar **v);

/**
 * Set a value in 'admin' table.
 *
 * @param k The key
 * @param v The value to set
 */
GError* sqlx_remote_execute_ADMSET(const gchar *target, GByteArray *sid,
		struct sqlx_name_s *name, const gchar *k, const gchar *v);

/**
 * Get 'admin' status of a database.
 */
GError* sqlx_get_admin_status(const gchar *target, struct sqlx_name_s *name,
		guint32 *status);

/**
 * Set 'admin' status of a database.
 *
 * @param status One of ADMIN_STATUS_ENABLED, ADMIN_STATUS_FROZEN,
 *     ADMIN_STATUS_DISABLED
 */
GError* sqlx_set_admin_status(const gchar *target, struct sqlx_name_s *name,
		guint32 status);

#endif
