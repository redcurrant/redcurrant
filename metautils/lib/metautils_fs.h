#ifndef __REDCURRANT__metautils_fs__h
# define __REDCURRANT__metautils_fs__h 1

#include <glib.h>

#define PROC_DISKSTAT "/proc/diskstats"

struct majmin_s
{
	int major;
	int minor;
};

struct dated_majmin_s
{
	struct majmin_s majmin;
	time_t last_update;
};

struct disk_stat_s
{
	unsigned long reads;        // # of reads issued
	unsigned long read_merged;  // # of reads merged
	unsigned long read_sectors; // # of sectors read
	unsigned long read_time;    // # of milliseconds spent reading
	unsigned long writes;       // # of writes completed
	unsigned long write_merged; // # of writes merged
	unsigned long write_sectors;    // # of sectors written
	unsigned long write_time;   // # of milliseconds spent writing
	unsigned long io_in_progress;   // # of I/Os currently in progress
	unsigned long io_time;      // # of milliseconds spent doing I/Os
	unsigned long w_io_time;    // weighted # of milliseconds spent doing I/Os
};

struct io_stat_s
{
	struct timeval previous_time;
	struct disk_stat_s previous;
	struct timeval current_time;
	struct disk_stat_s current;
};


/**
 * Initialize the cache of FS stats.
 *
 * @warning NOT THREAD-SAFE.
 */
gboolean metautils_fs_init_stats();

/**
 * Clear the cache of FS stats.
 *
 * @warning NOT THREAD-SAFE.
 */
void metautils_fs_clear_stats();

/**
 * Compute statistics for all known storage devices.
 *
 * @return TRUE on success
 * @warning NOT THREAD-SAFE.
 */
gboolean metautils_fs_parse_stats(GError **err);

/**
 * Get the percentage of idle IO for the devide holding `path`.
 *
 * @warning NOT THREAD-SAFE.
 */
gboolean metautils_fs_get_io_idle_for_path(const char *path, int *idle, GError **err);

GString* metautils_fs_dump_io_idle();
#endif
