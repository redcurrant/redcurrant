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
# define G_LOG_DOMAIN "metautils.url"
#endif

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "metautils_loggers.h"
#include "metautils_resolv.h"
#include "metautils_bits.h"
#include "common_main.h"
#include "test_addr.h"

static void
test_bad_connect_address(void)
{
	static const gchar *pProc = __FUNCTION__;
	void test(const gchar *url) {
		gboolean rc = metautils_url_valid_for_connect(url);
		URL_ASSERT(rc == FALSE);
	}
	test_on_urlv(bad_urls, test);
}

static void
test_good_connect_address(void)
{
	static const gchar *pProc = __FUNCTION__;
	void test(const gchar *url) {
		gboolean rc = metautils_url_valid_for_connect(url);
		URL_ASSERT(rc != FALSE);
	}
	test_on_urlv(good_urls, test);
}

int
main(int argc, char **argv)
{
	HC_TEST_INIT(argc,argv);
	g_test_add_func("/metautils/addr/bad_connect",
			test_bad_connect_address);
	g_test_add_func("/metautils/gridd_client/good_address",
			test_good_connect_address);
	return g_test_run();
}

