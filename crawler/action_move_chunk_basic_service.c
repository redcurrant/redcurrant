/* THIS FILE IS NO MORE MAINTAINED */

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>

#include <glib.h>
#include <dbus/dbus.h>

#include <metautils/lib/metautils.h>

#include "crawler_constants.h"
#include "crawler_common_tools.h"

static DBusConnection* conn;

static gboolean stop_thread;

static gchar* action_name;
static gchar* source_cmd_opt_name;
static gchar* destination_cmd_opt_name;
static gchar* deletion_cmd_opt_name;

static int service_pid;

static const gchar* occur_type_string;

/*
 * This method is listening to the system D-Bus action interface for action signals
 **/
static void
listening_action() {
	DBusError error;
	DBusMessage* msg = NULL;
        DBusMessageIter iter;
	GVariantType* param_type = NULL;
	const char* param_print = NULL;
	GVariant* param = NULL;
	GVariant* ack_parameters = NULL;

	/* Signal parsed parameters */
        int argc = -1;
        char** argv = NULL;

	guint64 context_id = 0;
	guint64 service_uid = 0;
	GVariant* occur = NULL;
	const gchar* source_path = NULL;
	gchar* destination_directory_path = NULL;
	gchar* destination_path = NULL;
	/* ------- */

	dbus_error_init(&error);

	param_type = g_variant_type_new(gvariant_action_param_type_string); /* Initializing the GVariant param type value */

        while ( FALSE == stop_thread ) {
                dbus_connection_read_write(conn, DBUS_LISTENING_TIMEOUT);
                msg = dbus_connection_pop_message(conn);

                if (NULL == msg)
                        continue;

                if (dbus_message_is_signal(msg, SERVICE_IFACE_ACTION, action_name)) { /* Is the signal name corresponding to the service name */
                        if (!dbus_message_iter_init(msg, &iter)) /* Is the signal containing at least one parameter ? */
                                continue;
                        else {
				if (DBUS_TYPE_STRING != dbus_message_iter_get_arg_type(&iter)) /* Is the parameter corresponding to a string value ? */
                               		continue;
                                else {
                                	dbus_message_iter_get_basic(&iter, &param_print); /* Getting the string parameter */

					if (NULL == (param = g_variant_parse(param_type, param_print, NULL, NULL, NULL))) {
						dbus_message_unref(msg);

						continue;
					}

					if (EXIT_FAILURE == disassemble_context_occur_argc_argv_uid(param, &context_id, &occur, &argc, &argv, &service_uid)) {
						g_variant_unref(param);
						dbus_message_unref(msg);

						continue;
					}

					/* End type signal management (last occurence for the specific service_uid value) */
                                        gboolean ending_signal = FALSE;
                                        if (0 == context_id) {
                                                GVariantType* occurt = g_variant_type_new("s");
                                                if (TRUE == g_variant_is_of_type(occur, occurt)) {
                                                        const gchar* occur_tile = g_variant_get_string(occur, NULL);
                                                        if (!g_strcmp0(end_signal_tile, occur_tile))
                                                                ending_signal = TRUE;
                                                }
                                                g_variant_type_free(occurt);
                                        }
                                        /* ------- */

					/* ACTION SPECIFIC AREA */

					if (FALSE == ending_signal) {
                                        	/* Destination directory path extraction */
                                        	if (NULL == (destination_directory_path = get_argv_value(argc, argv, action_name, destination_cmd_opt_name))) {
							g_free(argv);
                                                	g_variant_unref(param);
                                                	dbus_message_unref(msg);

                                                	continue;
                                        	}
                                        	/* ------- */

						/* Preparing the complete pathes */
						GVariantType* gvt = g_variant_type_new(occur_type_string);
                                        	if (NULL == occur || FALSE == g_variant_is_of_type(occur, gvt)) {
                                                	g_free(argv);
                                                	g_variant_unref(param);
                                                	dbus_message_unref(msg);
                                                	g_variant_type_free(gvt);
							g_free(destination_directory_path);

                                                	continue;
                                        	}
                                        	source_path = g_variant_get_string(g_variant_get_child_value(occur, 0), NULL);
                                        	g_variant_type_free(gvt);
						gchar* temp_basename = g_path_get_basename(source_path);
						destination_path = g_strconcat(destination_directory_path, G_DIR_SEPARATOR_S, temp_basename, NULL);
						g_free(temp_basename);
						/* ------- */

						/* Moving the file and sending the ACK signal */
						char* temp_msg = (char*)g_malloc0((SHORT_BUFFER_SIZE * sizeof(char)) + sizeof(guint64));
						if (EXIT_FAILURE == move_file(source_path, (const gchar*)destination_path, TRUE)) {
                                			sprintf(temp_msg, "%s on %s for the context %llu and the file %s", ACK_KO, action_name, (long long unsigned)context_id, source_path);

                                			GRID_INFO("%s (%d) : %s", action_name, service_pid, temp_msg);

							GVariant* temp_msg_gv = g_variant_new_string(temp_msg);

                                			ack_parameters = g_variant_new(gvariant_ack_param_type_string, context_id, temp_msg_gv);

                                			if (EXIT_FAILURE == send_signal(conn, SERVICE_OBJECT_NAME, SERVICE_IFACE_ACK, ACK_KO, ack_parameters))
                                        			GRID_ERROR("%s (%d) : System D-Bus signal sending failed %s %s", action_name, service_pid, error.name, error.message);
                        			}
						else {
                                                	sprintf(temp_msg, "%s on %s for the context %llu and the file %s", ACK_OK, action_name, (long long unsigned)context_id, source_path);

                                                	GRID_INFO("%s (%d) : %s", action_name, service_pid, temp_msg);

							GVariant* temp_msg_gv = g_variant_new_string(temp_msg);

                                                	ack_parameters = g_variant_new(gvariant_ack_param_type_string, context_id, temp_msg_gv);

                                                	if (EXIT_FAILURE == send_signal(conn, SERVICE_OBJECT_NAME, SERVICE_IFACE_ACK, ACK_OK, ack_parameters))
                                                	        GRID_ERROR("%s (%d) : System D-Bus signal sending failed %s %s", action_name, service_pid, error.name, error.message);
						}
						g_variant_unref(ack_parameters);
                                        	g_free(temp_msg);
						/* ------- */
					
						g_free(destination_path);
						g_free(destination_directory_path);
					
						/* XXXXXXX */
					}

					g_free(argv);
					g_variant_unref(param);
				}
			}
		}

		dbus_message_unref(msg);
	}

	g_variant_type_free(param_type);
}

/* GRID COMMON MAIN */
static struct grid_main_option_s *
main_get_options(void) {
        static struct grid_main_option_s options[] = {
                { NULL, 0, {.b=NULL}, NULL }
        };

        return options;
}

static void
main_action(void) {
        gchar* match_pattern = NULL;
        DBusError error;

        dbus_error_init(&error);

        /* DBus connexion */
        if (EXIT_FAILURE == init_dbus_connection(&conn)) {
                GRID_ERROR("%s (%d) : System D-Bus connection failed %s %s", action_name, service_pid, error.name, error.message);

                exit(EXIT_FAILURE);
        }
        /* ------- */

        /* Signal subscription */
        match_pattern = g_strconcat("type='signal',interface='", SERVICE_IFACE_ACTION, "'", NULL);
        dbus_bus_add_match(conn, match_pattern, &error);
        dbus_connection_flush(conn);
        if (dbus_error_is_set(&error)) {
                GRID_ERROR("%s (%d) : Subscription to the system D-Bus action signals on the action interface failed %s %s", action_name, service_pid, error.name, error.message);

                g_free(match_pattern);

                exit(EXIT_FAILURE);
        }

        g_free(match_pattern);
        /* ------- */

        GRID_INFO("%s (%d) : System D-Bus %s action signal listening thread started...", action_name, service_pid, action_name);
        listening_action();

        exit(EXIT_SUCCESS);
}

static void
main_set_defaults(void) {
	conn = NULL;
	stop_thread = FALSE;
	action_name = "action_move_chunk_basic";
	source_cmd_opt_name = "s";
	destination_cmd_opt_name = "f";
	deletion_cmd_opt_name = "d";
	service_pid = getpid();
	occur_type_string = "(ss)";
}

static void
main_specific_fini(void) { }

static gboolean
main_configure(int argc, char **args) {
	argc = argc;
	args = args;

	return TRUE;
}

static const gchar*
main_usage(void) { return ""; }

static void
main_specific_stop(void) {
	stop_thread = TRUE;
	GRID_INFO("%s (%d) : System D-Bus %s action signal listening thread stopped...", action_name, service_pid, service_name);
}

static struct grid_main_callbacks cb = {
	.options = main_get_options,
	.action = main_action,
	.set_defaults = main_set_defaults,
	.specific_fini = main_specific_fini,
	.configure = main_configure,
	.usage = main_usage,
	.specific_stop = main_specific_stop
};

int
main(int argc, char **argv) {
	return grid_main(argc, argv, &cb);
}
/* ------- */
