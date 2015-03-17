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

#ifndef __VNS_AGENT_INTERNALS_H__
# define __VNS_AGENT_INTERNALS_H__

# include <stdio.h>
# include <stdlib.h>
# include <unistd.h>
# include <stdarg.h>
# include <string.h>
# include <errno.h>
# include <signal.h>
# include <time.h>
# include <netdb.h>
# include <sys/types.h>
# include <sys/time.h>

# include <math.h>

# include <metautils/lib/metautils.h>
# include <metautils/lib/metacomm.h>
# include <cluster/lib/gridcluster.h>

# include <remote/vns_agent_remote.h>
# include <lib/vns_agent.h>

struct vns_agent_handle_s
{
	namespace_info_t ns_info;
	GHashTable *vns_space_used;
	get_namespace_info_f get_namespace_info;
};

typedef struct vns_agent_handle_s vns_agent_handle_t;

extern struct vns_agent_handle_s *vns_agent_handle;

#endif /*__VNS_AGENT_INTERNALS_H__*/
