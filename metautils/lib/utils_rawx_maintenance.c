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

#ifndef G_LOG_DOMAIN
# define G_LOG_DOMAIN "metautils.rawx_maintenance"
#endif

#include "metautils.h"

void
chunk_textinfo_free_content(struct chunk_textinfo_s *cti)
{
	if (!cti)
		return;
	g_free(cti->id);
	g_free(cti->path);
	g_free(cti->size);
	g_free(cti->hash);
	g_free(cti->position);
	g_free(cti->metadata);
	g_free(cti->container_id);
	memset(cti, 0x00, sizeof(struct chunk_textinfo_s));
}


void
content_textinfo_free_content(struct content_textinfo_s *cti)
{
	if (!cti)
		return;
	g_free(cti->path);
	g_free(cti->size);
	g_free(cti->metadata);
	g_free(cti->system_metadata);
	g_free(cti->chunk_nb);
	g_free(cti->container_id);
	g_free(cti->storage_policy);
	g_free(cti->rawx_list);
	g_free(cti->spare_rawx_list);
	g_free(cti->version);
	memset(cti, 0x00, sizeof(struct content_textinfo_s));
}


int
chunk_is_last(struct chunk_textinfo_s *chunk, struct content_textinfo_s *content)
{
	gint32 i_pos, i_nbr;

	if (!chunk || !content)
		return 0;
	if (!chunk->position || !content->chunk_nb)
		return 0;
	i_pos = g_ascii_strtoll(chunk->position, NULL, 10);
	i_nbr = g_ascii_strtoll(content->chunk_nb, NULL, 10);
	return (i_pos + 1 >= i_nbr);
}
