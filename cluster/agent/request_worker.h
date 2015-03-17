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

#ifndef _MESSAGE_WORKER_H
#define _MESSAGE_WORKER_H

#include <cluster/agent/worker.h>

/**
  *	Initialize the message_worker
  *
 */
int init_request_worker(GError **error);

/**
  *	The default worker for handling message
  *
 */
int request_worker(worker_t *worker, GError **error);

#endif		/* _MESSAGE_WORKER_H */
