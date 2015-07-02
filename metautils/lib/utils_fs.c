#ifndef G_LOG_DOMAIN
#define G_LOG_DOMAIN "metautils.fs"
#endif

#include <errno.h>
#include <glib.h>

#include "metautils.h"
#include "metautils_fs.h"
#include "metautils_internals.h"

static GHashTable *majmin_to_stats = NULL;
static GHashTable *path_to_majmin = NULL;


static guint64
_token(GMatchInfo *match_info, gint group)
{
	gchar *str = g_match_info_fetch(match_info, group);
	if (!str)
		return 0;
	guint64 v = g_ascii_strtoull(str, NULL, 10);
	g_free(str);
	return v;
}

static int
_token_int(GMatchInfo *match_info, gint group)
{
	gchar *str = g_match_info_fetch(match_info, group);
	if (!str)
		return 0;
	int v = atoi(str);
	g_free(str);
	return v;
}

static void
_diskstat_line(GMatchInfo *match_info)
{
	struct majmin_s key;
	key.major = _token_int(match_info, 1);
	key.minor = _token_int(match_info, 2);

	struct disk_stat_s dstat;
	dstat.reads = _token(match_info, 3);
	dstat.read_merged = _token(match_info, 4);
	dstat.read_sectors = _token(match_info, 5);
	dstat.read_time = _token(match_info, 6);
	dstat.writes = _token(match_info, 7);
	dstat.write_merged = _token(match_info, 8);
	dstat.write_sectors = _token(match_info, 9);
	dstat.write_time = _token(match_info, 10);
	dstat.io_in_progress = _token(match_info, 11);
	dstat.io_time = _token(match_info, 12);
	dstat.w_io_time = _token(match_info, 13);

	struct io_stat_s *s = g_hash_table_lookup(majmin_to_stats, &key);
	if (NULL == s) {
		s = g_malloc0(sizeof(struct io_stat_s));
		g_hash_table_insert(majmin_to_stats, g_memdup(&key, sizeof(key)), s);
	}

	memcpy(&(s->previous), &(s->current), sizeof(struct disk_stat_s));
	memcpy(&(s->previous_time), &(s->current_time), sizeof(struct timeval));
	memcpy(&(s->current), &dstat, sizeof(struct disk_stat_s));
	gettimeofday(&(s->current_time), NULL);
}

static guint
majmin_hash(struct majmin_s *p)
{
	return makedev(p->major, p->minor);
}

static gboolean
majmin_equal(struct majmin_s *p0, struct majmin_s *p1)
{
	return p0->major == p1->major && p0->minor == p1->minor;
}

gboolean
metautils_fs_init_stats()
{
	if (!path_to_majmin) {
		path_to_majmin = g_hash_table_new_full(
				g_str_hash, g_str_equal,
				g_free, g_free);
	}

	if (!majmin_to_stats) {
		majmin_to_stats = g_hash_table_new_full(
				(GHashFunc)majmin_hash, (GEqualFunc)majmin_equal,
				g_free, g_free);
	}
	return TRUE;
}

void
metautils_fs_clear_stats()
{
	if (path_to_majmin) {
		g_hash_table_unref(path_to_majmin);
		path_to_majmin = NULL;
	}
	if (majmin_to_stats) {
		g_hash_table_unref(majmin_to_stats);
		majmin_to_stats = NULL;
	}
}

gboolean
metautils_fs_parse_stats(GError **err)
{
	char *current_line = NULL;
	char *next_new_line = NULL;
	char *diskstat = NULL;
	GRegex *regex = NULL;

	metautils_fs_init_stats();

	if (!g_file_get_contents(PROC_DISKSTAT, &diskstat, NULL, err)) {
		GSETERROR(err, "Failed to get the statistics");
		return 0;
	}

	regex = g_regex_new(
			"^\\s*([0-9]+)\\s+([0-9]+)"
			"\\s+\\S+"
			"\\s+([0-9]+)\\s+([0-9]+)\\s+([0-9]+)\\s+([0-9]+)"
			"\\s+([0-9]+)\\s+([0-9]+)\\s+([0-9]+)\\s+([0-9]+)"
			"\\s+([0-9]+)\\s+([0-9]+)\\s+([0-9]+)",
			0, 0, err);

	if (regex == NULL) {
		GSETERROR(err, "Failed to build regex for parsing %s", PROC_DISKSTAT);
		g_free(diskstat);
		return 0;
	}

	current_line = diskstat;
	next_new_line = strchr(current_line, '\n');
	while (next_new_line) {
		GMatchInfo *match_info = NULL;
		if (g_regex_match(regex, current_line, 0, &match_info)) {
			if (match_info) {
				_diskstat_line(match_info);
			}
		}
		if (match_info)
			g_match_info_free(match_info);
		match_info = NULL;
		current_line = next_new_line + 1;
		next_new_line = strchr(current_line, '\n');
	}

	g_regex_unref(regex);
	g_free(diskstat);
	return TRUE;

}

int
metautils_fs_get_major_minor(const gchar *path, int *pmaj, int *pmin,
		GError **err)
{
	*pmaj = 0;
	*pmin = 0;

	metautils_fs_init_stats();

	struct dated_majmin_s *v = g_hash_table_lookup(path_to_majmin, path);
	if (NULL == v) {
		v = g_malloc0(sizeof(struct dated_majmin_s));
		g_hash_table_insert(path_to_majmin, g_strdup(path), v);
	}

	time_t now = time(NULL);
	if (!v->last_update || v->last_update > now || v->last_update < now - 30) {
		struct stat file_stat;
		memset(&file_stat, 0, sizeof(file_stat));
		if (0 > stat(path, &file_stat)) {
			GSETERROR(err, "stat(%s): errno=%d %s", path, errno, strerror(errno));
			return 0;
		}
		v->majmin.major = major(file_stat.st_dev);
		v->majmin.minor = minor(file_stat.st_dev);
		v->last_update = now;
	}

	*pmaj = v->majmin.major;
	*pmin = v->majmin.minor;
	GRID_TRACE("Device [%s] major=%d minor=%d", path, *pmaj, *pmin);
	return 1;
}

int
metautils_fs_get_io_idle(int major, int minor, int *idle, GError **err)
{
	struct majmin_s key;
	struct io_stat_s *s = NULL;
	struct timeval elapsed;
	double sec_d, usec_d, prev_d, cur_d, percent_used_d, time_spent_d, result_d;

	metautils_fs_init_stats();

	key.major = major;
	key.minor = minor;
	if (!(s = g_hash_table_lookup(majmin_to_stats, &key))) {
		GSETERROR(err, "Device not found major=%d minor=%d", major, minor);
		return 0;
	}

	timersub(&(s->current_time), &(s->previous_time), &(elapsed));

	/*convert working values in floating point numbers */
	cur_d = s->current.io_time;
	prev_d = s->previous.io_time;
	sec_d = elapsed.tv_sec;
	usec_d = elapsed.tv_usec;

	percent_used_d = 100.0 * (cur_d > prev_d ? cur_d - prev_d : 0.0);
	time_spent_d = sec_d * 1000.0 + usec_d / 1000.0;

	result_d = 100.0 - (percent_used_d / (time_spent_d > 0.0 ? time_spent_d : 1.0));

	*idle = result_d; // implicit conversion
	return 1;
}

gboolean
metautils_fs_get_io_idle_for_path(const char *path, int *idle, GError **err)
{
	int major, minor;

	if (metautils_fs_get_major_minor(path, &major, &minor, err) &&
			metautils_fs_get_io_idle(major, minor, idle, err))
		return TRUE;

	GSETERROR(err, "No stat for [%s]", path);
	return FALSE;
}

GString*
metautils_fs_dump_io_idle()
{
	GString *output = g_string_sized_new(256);
	GList *keys = g_hash_table_get_keys(majmin_to_stats);
	for (GList *l = keys; l; l = l->next) {
		struct majmin_s *majmin = l->data;
		int idle = -1;
		metautils_fs_get_io_idle(majmin->major, majmin->minor, &idle, NULL);
		g_string_append_printf(output, "major=%d minor=%d idle=%d\n",
				majmin->major, majmin->minor, idle);
	}
	g_list_free(keys);
	return output;
}

