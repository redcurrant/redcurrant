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

#ifndef GRID__tash_h
# define GRID__tash_h 1
# include <glib.h>

// Object style API

typedef guint task_period_t;

struct grid_task_queue_s;

struct grid_task_queue_s* grid_task_queue_create(const gchar *name);

void grid_task_queue_stop(struct grid_task_queue_s *gtq);

void grid_task_queue_destroy(struct grid_task_queue_s *gtq);

guint grid_task_queue_sleepticks(struct grid_task_queue_s *gtq);

void grid_task_queue_fire(struct grid_task_queue_s *gtq);

void grid_task_queue_register(struct grid_task_queue_s *gtq,
		task_period_t period, GDestroyNotify run, GDestroyNotify cleanup,
		gpointer udata);

/**
 * When the thread is joined, 'gtq' is returned.
 *
 * @param gtq
 * @param err
 * @return
 */
GThread* grid_task_queue_run(struct grid_task_queue_s *gtq, GError **err);

// Static API suitable in most cases

#endif // GRID__tash_h
