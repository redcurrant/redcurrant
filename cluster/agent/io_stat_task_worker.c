#ifndef G_LOG_DOMAIN
# define G_LOG_DOMAIN "gridcluster.agent.io_stat_task_worker"
#endif

#include <sys/time.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <mntent.h>

#include <metautils/lib/metautils.h>

#include "io_stat_task_worker.h"
#include "task_scheduler.h"
#include "agent.h"
#include "worker.h"

#define TASK_ID "io_stat_task"

static int
io_stat_task_worker(gpointer p, GError ** error)
{
	TRACE_POSITION();
	(void)p;
	int result = 0;

	result = (int) metautils_fs_parse_stats(error);

	task_done(TASK_ID);
	return result;
}

int
start_io_stat_task(GError **err)
{
	metautils_fs_init_stats();

	task_t *task = create_task(2, TASK_ID);
	task->task_handler = io_stat_task_worker;

	if (!add_task_to_schedule(task, err)) {
		GSETERROR(err, "Failed to add io_stat task to scheduler");
		g_free(task);
		return 0;
	}

	return 1;
}

