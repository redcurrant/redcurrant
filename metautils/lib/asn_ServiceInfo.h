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

#ifndef __ASN_SRVINFO_H__
#define __ASN_SRVINFO_H__

#include "./metatypes.h"
#include "./ServiceInfo.h"
#include "./ServiceInfoSequence.h"

gboolean service_info_ASN2API(ServiceInfo_t * asn, service_info_t * api);
gboolean service_info_API2ASN(service_info_t * api, ServiceInfo_t * asn);
void service_info_cleanASN(ServiceInfo_t * asn, gboolean only_content);

#endif
