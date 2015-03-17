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

#ifndef META_BACKEND_COMMON__H
# define META_BACKEND_COMMON__H 1
# include <glib.h>
# include <metautils/lib/metautils.h>

# include <sqliterepo/sqliterepo.h>

struct meta_backend_common_s {
	gchar ns_name[LIMIT_LENGTH_NSNAME];
	namespace_info_t ns_info;
	GMutex *ns_info_lock;
	const gchar *type;
	struct sqlx_repository_s *repo;

	// Managed by sqlx_service_extra, do not allocate/free
	struct grid_lbpool_s *lb;
	struct event_config_repo_s *evt_repo;
};

#endif

