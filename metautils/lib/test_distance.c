#ifndef G_LOG_DOMAIN
# define G_LOG_DOMAIN "metautils.url"
#endif

#include <stdlib.h>
#include <glib.h>
#include "./metautils.h"

static void
test_distance(void)
{
	g_assert(0 == distance_between_location("room1.server1.1", "room1.server1.1"));
	g_assert(1 == distance_between_location("room1.server1.1", "room1.server1.2"));
	g_assert(2 == distance_between_location("room1.server1.1", "room1.server2.1"));
	g_assert(4 == distance_between_location("room1.server1.1", "room2.server1.1"));

	g_assert(1 == distance_between_location("", ""));
	g_assert(1 == distance_between_location(NULL, ""));
	g_assert(1 == distance_between_location("", NULL));
	g_assert(1 == distance_between_location(NULL, NULL));

	g_assert(8 == distance_between_location(NULL, "room1.server1.1"));
	g_assert(8 == distance_between_location("room1.server1.1", NULL));
}

int
main(int argc, char **argv)
{
	HC_TEST_INIT(argc, argv);
	g_test_add_func("/metautils/dist", test_distance);
	return g_test_run();
}
