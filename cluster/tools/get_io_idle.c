#include <stdlib.h>
#include <glib.h>

#include <metautils/lib/metautils.h>


static int delay = 5;
static char *path = NULL;

static void
usage(int argc, char **argv)
{
	(void) argc;
	g_print("usage: %s [-v] [-d DELAY] [-p PATH]\n", argv[0]);
	exit(0);
}

int
main(int argc, char **argv)
{
	int idle = -1, rc = 0;
	GError *err = NULL;

	g_log_set_handler(NULL,
			G_LOG_LEVEL_ERROR | G_LOG_FLAG_FATAL | G_LOG_FLAG_RECURSION,
			g_log_default_handler,
			NULL);

	for (;;) {
		int c = getopt(argc, argv, "d:p:hv");
		if (c == -1)
			break;
		switch (c) {
		case 'd':
			delay = atoi(optarg);
			break;
		case 'p':
			path = optarg;
			break;
		case 'h':
			usage(argc, argv);
			break;
		case 'v':
			g_log_set_handler(NULL,
					G_LOG_LEVEL_MASK | G_LOG_FLAG_FATAL | G_LOG_FLAG_RECURSION,
					g_log_default_handler,
					NULL);
			break;
		default:
			g_printerr("Unrecognized option '%c'\n", optopt);
			break;
		}
	}

	if (!metautils_fs_parse_stats(&err)) {
		rc = 1;
		goto end;
	}

	g_print("FS statistics parsed, sleeping %ds\n", delay);
	sleep(delay);

	if (!metautils_fs_parse_stats(&err)) {
		rc = 1;
		goto end;
	}
	g_print("FS statistics parsed again\n");

	if (path != NULL) {
		if (!metautils_fs_get_io_idle_for_path(path, &idle, &err)) {
			rc = 2;
			goto end;
		}
		g_print("Idle IO percentage for [%s]: %d\n", path, idle);
	} else {
		GString *all_dev = metautils_fs_dump_io_idle();
		g_print("%s", all_dev->str);
		g_string_free(all_dev, TRUE);
	}

end:
	if (err)
		g_printerr("%s\n", err->message);
	g_clear_error(&err);
	metautils_fs_clear_stats();
	return rc;
}

