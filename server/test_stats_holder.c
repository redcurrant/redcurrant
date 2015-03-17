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
# define G_LOG_DOMAIN "test"
#endif

#include "metautils/lib/metautils.h"
#include "server/stats_holder.h"
#include <glib.h>

static void
test_rrd (void)
{
	struct grid_single_rrd_s *rrd = grid_single_rrd_create(2, 60);
	grid_single_rrd_push(rrd, 3, 0);
	grid_single_rrd_push(rrd, 1000, 0);
	grid_single_rrd_push(rrd, 2, 0);
	grid_single_rrd_destroy(rrd);
}

int
main (int argc, char **argv)
{
	HC_TEST_INIT(argc,argv);
	g_test_add_func("/server/rrd", test_rrd);
	return g_test_run();
}

