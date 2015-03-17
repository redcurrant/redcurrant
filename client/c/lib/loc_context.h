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
 * @file loc_context.h
 * Client loc_context library
 */

#ifndef __LOC_CONTEXT__H__
# define __LOC_CONTEXT__H__ 1

/**
 * Forward declaration
 */
struct loc_context_s;
struct hc_url_s;

/**
 * @param hc
 * @param url
 * @return
 */
struct loc_context_s * loc_context_init(gs_grid_storage_t *hc,
		struct hc_url_s *url, gs_error_t **p_e);

struct loc_context_s * loc_context_init_retry(gs_grid_storage_t *hc,
		struct hc_url_s *url, gs_error_t **p_e);

void loc_context_clean(struct loc_context_s *lc);

char * loc_context_to_string(const struct loc_context_s *lc, int xml);

char* loc_context_getstgpol_to_string(const struct loc_context_s *lc, gboolean bContent);

#endif /* __LOC_CONTEXT__H__ */
