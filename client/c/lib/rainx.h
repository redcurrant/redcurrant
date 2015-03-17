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

#ifndef __RAINX_H_CLIENT__
#define __RAINX_H_CLIENT__

#include "./gs_internals.h"

#define RAINX_UPLOAD "PUT"
#define RAINX_DOWNLOAD "GET"

gboolean stg_pol_is_rainx(namespace_info_t *ni, const gchar *stgpol);
gboolean stg_pol_rainx_get_param(namespace_info_t *ni, const gchar *stgpol, const gchar *param, gint64 *p_val);

GSList* rainx_get_spare_chunks(gs_container_t *container, gchar *content_path, gint64 count,
		gint64 distance, GSList *notin_list, GSList *broken_rawx_list, gs_error_t **err);

addr_info_t* get_rainx_from_conscience(const gchar *nsname, GError **error);


gboolean rainx_ask_reconstruct(struct dl_status_s *dl_status, gs_content_t *content, GSList *aggregated_chunks,
		GSList *filtered, GSList *beans, GSList *broken_rawx_list, GHashTable *failed_chunks,
		const gchar *storage_policy, gs_error_t **err);

#endif /* __RAINX_H_CLIENT__ */
