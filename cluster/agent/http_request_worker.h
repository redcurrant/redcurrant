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

#ifndef HTTP_REQUEST_WORKER_H
#define HTTP_REQUEST_WORKER_H

#include <metautils/lib/metatypes.h>
#include <cluster/agent/worker.h>

typedef struct http_session_s {
	addr_info_t *addr;
	enum {E_GET, E_POST} method;
	char *url;
	char *body;
	worker_t *worker;
	worker_func_f response_handler;
	worker_func_f error_handler;
} http_session_t;

int http_request_worker(worker_t *worker, GError **error);

#endif	/* HTTP_REQUEST_WORKER_H */
