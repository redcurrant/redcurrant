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

#ifndef __CRAWLER_BUSCMD_H
#define __CRAWLER_BUSCMD_H






GError* tlc_init_connection(TCrawlerBus** conn, char* service_name, char* service_path,
                    char* bus_address, TCrawlerBusObjectInfo* object_info);


/*************/
/* A-SYNCHRO**/

GError* tlc_Send_DataTripEx_noreply(TCrawlerReq* req, void *user_data, char* addSender, char* alldata);

GError* tlc_Send_CmdProc(TCrawlerReq* req, int timeout,
        void (*_notify_callback)(TCrawlerReq* req, GError* error, char* msgReceived, void *user_data),
        void *user_data, char* cmd, char* alldata);


GError* tlc_Send_CmdProcEx(TCrawlerReq* req, int timeout,
        void (*_notify_callback)(TCrawlerReq* req, GError* error, char* msgReceived, void *user_data),
        void *user_data, char* cmd, const char* sender, char* alldata);

GError* tlc_Send_Ack_noreply(TCrawlerReq* req, void *user_data, char* cmd, char* alldata);


#endif   //__CRAWLER_BUSCMD_H

