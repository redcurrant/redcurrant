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

#ifndef _TASK_SCHEDULER_H
#define _TASK_SCHEDULER_H

#include <glib.h>
#include <cluster/agent/task.h>

void init_task_scheduler(void);

void stop_task_scheduler(void);

void clean_task_scheduler(void);

int add_task_to_schedule(task_t * task, GError ** error);

void remove_task(const char *task_id);

void exec_tasks(void);

long get_time_to_next_task_schedule(void);

void task_done(const char *id);

void task_stop(const char *id);

gboolean is_task_scheduled(const char *id);

task_t* create_task(long frequency, const gchar *id);

task_t* set_task_callbacks(task_t *task, task_handler_f handle,
		GDestroyNotify clean, gpointer udata);

/* ------------------------------------------------------------------------- */

int list_tasks_worker(worker_t * worker, GError ** error);

#endif /* _TASK_SCHEDULER_H */
