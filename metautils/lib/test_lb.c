#ifndef G_LOG_DOMAIN
# define G_LOG_DOMAIN "grid.lb.test"
#endif

#include <stddef.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <math.h>

#include "metautils.h"

#define SRVTYPE "sqlx"

#define ADDR_GOOD "127.0.0.1"
#define ADDR_BAD  "127.0.0.2"

static guint max_feed = 37;
static guint max_get = 5001;

static struct service_info_s *
_build_si2(const gchar *a, guint i, gint score)
{
	struct service_info_s *si;

	si = g_malloc0(sizeof(*si));
	g_strlcpy(si->ns_name, "NS", sizeof(si->ns_name));
	g_strlcpy(si->type, SRVTYPE, sizeof(si->type));
	si->addr.addr.v4 = inet_addr(a);
	si->addr.type = TADDR_V4;
	si->addr.port = htons(i+2);
	si->score.value = score;
	si->score.timestamp = time(0);
	return si;
}

static struct service_info_s *
_build_si(const gchar *a, guint i)
{
	return _build_si2(a, i, i);
}

static guint
_fill(struct grid_lb_s *lb, guint max, gboolean same_score)
{
	guint i, count;
	int dump;

	gboolean provide(struct service_info_s **p_si) {
		g_assert(p_si != NULL);
		if (i >= max)
			return FALSE;
		*p_si = _build_si2(ADDR_GOOD, i, (same_score && i)? 100 : i);
		if (dump) {
			gchar *str = service_info_to_string(*p_si);
			g_free(str);
		}
		i++;
		count ++;
		return TRUE;
	}

	dump = 0;
	i = count = 0;
	grid_lb_reload(lb, &provide);

	dump = 1;
	i = count = 0;
	grid_lb_reload(lb, &provide);

	g_debug("%d services loaded", count);
	g_assert_cmpuint(count, ==, max);
	return count;
}

static struct grid_lb_s *
_build(gboolean same_score)
{
	struct grid_lb_s *lb;

	lb = grid_lb_init("NS", SRVTYPE);
	g_assert(lb != NULL);

	grid_lb_set_SD_shortening(lb, FALSE);
	grid_lb_set_shorten_ratio(lb, 1.001);
	_fill(lb, max_feed, same_score);
	return lb;
}

static void
check_presence(gboolean expected, struct grid_lb_iterator_s *iter,
		struct service_info_s *si)
{
	gboolean available;

	available = grid_lb_iterator_is_srv_available(iter, si);
	available = (available != 0);
	expected = (expected != 0);

	g_assert(available == expected);
}

static void
check_not_found(struct grid_lb_iterator_s *iter)
{
	struct service_info_s *si;

	si = _build_si(ADDR_GOOD, 1);
	check_presence(TRUE, iter, si);
	service_info_clean(si);

	si = _build_si(ADDR_BAD, 1);
	check_presence(FALSE, iter, si);
	service_info_clean(si);

	si = _build_si(ADDR_BAD, 1);
	si->addr.port = htons(1U);
	check_presence(FALSE, iter, si);
	service_info_clean(si);


	si = _build_si(ADDR_GOOD, max_feed);
	check_presence(FALSE, iter, si);
	service_info_clean(si);

	si = _build_si(ADDR_GOOD, 1);
	si->addr.port = htons(1U);
	check_presence(FALSE, iter, si);
	service_info_clean(si);

	si = _build_si(ADDR_GOOD, max_feed);
	check_presence(FALSE, iter, si);
	service_info_clean(si);

	si = _build_si(ADDR_GOOD, 0);
	check_presence(FALSE, iter, si);
	service_info_clean(si);
}

static gint
cmp_addr(gconstpointer a, gconstpointer b, gpointer user_data)
{
	(void) user_data;
	return addr_info_compare(a, b);
}

static void
_keep_for_repartition(GTree *used, struct service_info_s *si)
{
	guint *pi = g_tree_lookup(used, &(si->addr));
	if (!pi) {
		pi = g_malloc0(sizeof(guint));
		*pi = 1;
		g_tree_insert(used, g_memdup(&(si->addr), sizeof(struct addr_info_s)), pi);
	}
	else {
		++ *pi;
	}
}

static void
_compute_repartition(GTree *used, struct service_info_s **siv)
{
	while (*siv)
		_keep_for_repartition(used, *(siv++));
}

static void
_check_repartition_uniform(GTree *used, gdouble ratio)
{
	gint64 count = 0, total = 0, sq_total = 0, min = 0, max = 0;
	gdouble average = 0.0, std_dev = 0.0;

	gboolean hook_sum(gpointer ai, guint *pi, gpointer ignored) {
		(void) ai, (void) ignored;
		total += *pi;
		sq_total += *pi * *pi;
		++ count;
		return FALSE;
	}
	gboolean hook_check(gpointer ai, guint *pi, gpointer ignored) {
		(void) ignored;
		gchar tmp[64];
		grid_addrinfo_to_string(ai, tmp, 64);
		gint64 current = *pi;
		g_debug("%s -> count=%ld average=%f min=%ld max=%ld",
				tmp, current, average, min, max);
		g_assert_cmpint(current, <=, max);
		g_assert_cmpint(current, >=, min);
		return FALSE;
	}

	g_tree_foreach(used, (GTraverseFunc)hook_sum, NULL);
	if (count > 0) {
		average = (gdouble)total / (gdouble)count;
		min = (gint64) floor(average * (1.0 - ratio));
		max = (gint64) ceil(average * (1.0 + ratio));
		std_dev = sqrt(sq_total / (gdouble)count - average * average);
	}
	g_tree_foreach(used, (GTraverseFunc)hook_check, NULL);
	g_debug("relative standard deviation: %f%%", (std_dev * 100.0)/average);
	g_assert_cmpuint((guint)g_tree_nnodes(used), ==, (max_feed - 1));
}

static void
_check_repartition_decreasing(GTree *used, gdouble ratio)
{
	gint64 count = 0, total = 0, score_total = 0, min = 0, max = 0;
	gint64 deviation = 0;
	gdouble average = 0.0, variance = 0.0;

	gboolean hook_sum(gpointer ai, guint *pi, gpointer ignored) {
		(void) ai, (void) ignored;
		total += *pi;
		score_total += ((addr_info_t*)ai)->port - 2;
		++ count;
		return FALSE;
	}
	gboolean hook_check(gpointer ai, guint *pi, gpointer ignored) {
		(void) ignored;
		gchar tmp[64];
		grid_addrinfo_to_string(ai, tmp, 64);
		gint64 current = *pi;
		gint64 expected = (((addr_info_t*)ai)->port - 2) * total / score_total;
		min = (gint64) floor(expected * (1.0 - ratio));
		max = (gint64) ceil(expected * (1.0 + ratio));
		deviation = abs(current - expected);
		variance += deviation * deviation;
		g_debug("%s -> count=%ld ideal=%ld min=%ld max=%ld deviation=%ld",
				tmp, current, expected, min, max, deviation);
		g_assert_cmpint(current, <=, max);
		g_assert_cmpint(current, >=, min);
		return FALSE;
	}

	g_tree_foreach(used, (GTraverseFunc)hook_sum, NULL);
	if (count > 0) {
		average = (gdouble)total / (gdouble)count;
	}
	g_tree_foreach(used, (GTraverseFunc)hook_check, NULL);
	variance /= (gdouble) count;
	g_debug("relative standard deviation: %f%%", sqrt(variance)/average * 100.0);
}

static void
generate_set_and_check_uniform_repartition(struct grid_lb_iterator_s *iter,
		gdouble ratio)
{
	g_debug("By set...");
	struct service_info_s **siv = NULL;
	GTree *used = g_tree_new_full(cmp_addr, NULL, g_free, g_free);

	struct lb_next_opt_s opt;
	memset(&opt, 0, sizeof(opt));
	opt.req.max = max_get;
	opt.req.distance = 1;
	opt.req.duplicates = TRUE;
	gboolean rc = grid_lb_iterator_next_set(iter, &siv, &opt);
	g_assert(rc != FALSE);

	_compute_repartition(used, siv);
	_check_repartition_uniform(used, ratio);
	g_tree_destroy(used);
	service_info_cleanv(siv, FALSE);
}

static void
generate_1by1_and_check_uniform_repartition(struct grid_lb_iterator_s *iter,
		gdouble ratio)
{
	g_debug("One by one (uniform)...");
	GTree *used = g_tree_new_full(cmp_addr, NULL, g_free, g_free);
	for (guint i=0; i<max_get; ++i) {
		struct service_info_s *si = NULL;
		if (!grid_lb_iterator_next(iter, &si))
			break;
		if (!si)
			break;
		_keep_for_repartition(used, si);
		service_info_clean(si);
	}
	_check_repartition_uniform(used, ratio);
	g_tree_destroy(used);
}

static void
generate_1by1_and_check_decreasing_repartition(struct grid_lb_iterator_s *iter,
		gdouble margin)
{
	g_debug("One by one (decreasing)...");
	GTree *used = g_tree_new_full(cmp_addr, NULL, g_free, g_free);
	for (guint i=0; i<max_get; ++i) {
		struct service_info_s *si = NULL;
		if (!grid_lb_iterator_next(iter, &si))
			break;
		if (!si)
			break;
		_keep_for_repartition(used, si);
		service_info_clean(si);
	}
	_check_repartition_decreasing(used, margin);
	g_tree_destroy(used);
}

static guint
_count_set(struct grid_lb_iterator_s *iter, guint max)
{
	struct service_info_s **siv = NULL;
	gboolean rc;

	struct lb_next_opt_s opt;
	memset(&opt, 0, sizeof(opt));
	opt.req.max = max;
	opt.req.distance = 1;
	opt.req.duplicates = TRUE;

	rc = grid_lb_iterator_next_set(iter, &siv, &opt);
	g_assert(rc != FALSE);

	guint count = g_strv_length((gchar**)siv);
	service_info_cleanv(siv, FALSE);
	return count;
}

static guint
_count_single(struct grid_lb_iterator_s *iter, guint max)
{
	guint count = 0;

	while ((max--) > 0) {
		struct service_info_s *si = NULL;
		if (!grid_lb_iterator_next(iter, &si))
			break;
		service_info_clean(si);
		count ++;
	}

	return count;
}

static void
check_service_count(struct grid_lb_iterator_s *iter)
{
	g_assert(iter != NULL);
	check_not_found(iter);
	g_assert(max_get == _count_single(iter, max_get));
	g_assert((max_feed + 10) == _count_set(iter, max_feed + 10));
	g_assert(max_get == _count_set(iter, max_get));
}

static void
test_lb_RR(void)
{
	struct grid_lb_s *lb;
	struct grid_lb_iterator_s *iter;

	lb = _build(FALSE);
	iter = grid_lb_iterator_round_robin(lb);

	check_service_count(iter);
	generate_1by1_and_check_uniform_repartition(iter, 0.01);
	generate_set_and_check_uniform_repartition(iter, 0.01);

	grid_lb_iterator_clean(iter);
	grid_lb_clean(lb);
}

static void
test_lb_WRR(void)
{
	struct grid_lb_s *lb;
	struct grid_lb_iterator_s *iter;

	lb = _build(TRUE);
	iter = grid_lb_iterator_weighted_round_robin(lb);
	check_service_count(iter);
	generate_1by1_and_check_uniform_repartition(iter, 0.01);
	generate_set_and_check_uniform_repartition(iter, 0.01);
	grid_lb_iterator_clean(iter);
	grid_lb_clean(lb);

	lb = _build(FALSE);
	iter = grid_lb_iterator_weighted_round_robin(lb);
	check_service_count(iter);
	generate_1by1_and_check_decreasing_repartition(iter, 0.7);
	grid_lb_iterator_clean(iter);
	grid_lb_clean(lb);
}

static void
test_lb_RAND(void)
{
	struct grid_lb_s *lb;
	struct grid_lb_iterator_s *iter;

	lb = _build(FALSE);
	iter = grid_lb_iterator_random(lb);
	check_service_count(iter);
	generate_1by1_and_check_uniform_repartition(iter, 0.3);
	generate_set_and_check_uniform_repartition(iter, 0.3);

	grid_lb_iterator_clean(iter);
	grid_lb_clean(lb);
}

static void
test_lb_WRAND(void)
{
	struct grid_lb_s *lb;
	struct grid_lb_iterator_s *iter;

	lb = _build(TRUE);
	iter = grid_lb_iterator_weighted_random(lb);
	check_service_count(iter);
	generate_1by1_and_check_uniform_repartition(iter, 0.3);
	generate_set_and_check_uniform_repartition(iter, 0.3);
	grid_lb_iterator_clean(iter);
	grid_lb_clean(lb);

	lb = _build(FALSE);
	iter = grid_lb_iterator_weighted_random(lb);
	check_service_count(iter);
	generate_1by1_and_check_decreasing_repartition(iter, 0.7);
	grid_lb_iterator_clean(iter);
	grid_lb_clean(lb);}

/* -------------------------------------------------------------------------- */

static void
test_pool_create_destroy(void)
{
	struct grid_lbpool_s *glp = grid_lbpool_create("NS");
	g_assert(glp != NULL);
	grid_lbpool_destroy(glp);
}

/* -------------------------------------------------------------------------- */

int
main(int argc, char **argv)
{
	HC_TEST_INIT(argc,argv);
	g_test_add_func("/grid/lb/RR", test_lb_RR);
	g_test_add_func("/grid/lb/WRR", test_lb_WRR);
	g_test_add_func("/grid/lb/RAND", test_lb_RAND);
	g_test_add_func("/grid/lb/WRAND", test_lb_WRAND);
	g_test_add_func("/grid/pool/create_destroy", test_pool_create_destroy);
	srand(time(NULL));
	return g_test_run();
}

