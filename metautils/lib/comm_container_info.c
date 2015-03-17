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
#define G_LOG_DOMAIN "metacomm.container_info"
#endif

#include "./metautils_internals.h"

#include "./asn_ContainerInfo.h"

#include "./ContainerInfo.h"
#include "./ContainerInfoSequence.h"

static struct abstract_sequence_handler_s seq_descriptor = {
	sizeof(ContainerInfo_t),
	sizeof(container_info_t),
	&asn_DEF_ContainerInfoSequence,
	(abstract_converter_f) container_info_ASN2API,
	(abstract_converter_f) container_info_API2ASN,
	(abstract_asn_cleaner_f) container_info_cleanASN,
	(abstract_api_cleaner_f) g_free,
	"container_info"
};

DEFINE_MARSHALLER(container_info_marshall);
DEFINE_MARSHALLER_GBA(container_info_marshall_gba);
DEFINE_UNMARSHALLER(container_info_unmarshall);
DEFINE_BODY_MANAGER(container_info_concat, container_info_unmarshall);
