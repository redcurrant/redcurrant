#ifndef G_LOG_DOMAIN
# define G_LOG_DOMAIN "grid.tools.uncompress"
#endif

#include <assert.h>
#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <signal.h>

#include <metautils/lib/metautils.h>
#include "gs_rawx_tools.h"

#define DECOMPRESSION_MAX_BUFSIZE 512000

char *optarg;
int optind, opterr, optopt;

int flag_verbose = 0;
int flag_quiet = 0;
int flag_help = 0;

gchar *algo = NULL;
int blocksize = DEFAULT_COMPRESSION_BLOCKSIZE;


struct compression_ctx_s* comp_ctx = NULL;
struct compressed_chunk_s *cp_chunk = NULL;

guint32 checksum;
guint32 compressed_size;
static gboolean preserve = FALSE;
static gboolean keep_pending = FALSE;

static void
help(int argc, char **args)
{
	(void) argc;
	g_printerr("gs_uncompress utility:\n");
	g_printerr("This binary compress a chunk and set all informations\n");
	g_printerr("needed by an httpd rawx to uncompress it\n");
	g_printerr("Usage: %s [OPTION]... chunk_path...\n", args[0]);
	g_printerr("OPTIONS::\n");
	g_printerr("\t -h : displays this help section;\n");
	g_printerr("\t -k : keep pending file in case of error;\n");
	g_printerr("\t -l <path> : use log4c configuration file;\n");
	g_printerr("\t -p : preserve mode (recommanded);\n");
	g_printerr("\t -v : verbose mode, increases debug output;\n");
}

static int
parse_opt(int argc, char **args)
{
	int opt;

	while ((opt = getopt(argc, args, "hvpqkl:")) != -1) {
		switch (opt) {
		case 'h':
			flag_help = ~0;
			break;
		case 'k':
			PRINT_DEBUG("Keep pending file\n");
			keep_pending = TRUE;
			break;
		case 'l':
			if (log4c_load(optarg) != 0) {
				PRINT_ERROR("Failed to load %s\n", optarg);
			}
			break;
		case 'v':
			flag_verbose++;
			break;
		case 'p':
			PRINT_DEBUG("Preserve mode activated\n");
			preserve = TRUE;
			break;
		case 'q':
			flag_quiet = ~0;
			break;
		case '?':
		default:
			PRINT_ERROR("unexpected %c (%s)\n", optopt, strerror(opterr));
			return 0;
		}
	}

	return 1;
}

int
main(int argc, char** args)
{
	int rc = -1;
	log4c_init();

	if (argc <= 1) {
		help(argc, args);
		return 1;
	}
	if (!parse_opt(argc, args)) {
		help(argc, args);
		return 1;
	}
	if (flag_help) {
		help(argc, args);
		return 0;
	}

	if (optind < argc) {
		GError *local_error = NULL;
		int i;
		for (i = optind; i < argc; i++) {
			/* Sanity check */
			PRINT_DEBUG("Going to work with chunk file [%s]\n", args[i]);
			/* Run decompression */
			if(uncompress_chunk2(args[i], preserve, keep_pending, &local_error) != 1) {
				if(local_error)
					PRINT_ERROR("Failed to uncompress chunk [%s] :\n %s", args[i], local_error->message);
				else
					PRINT_ERROR("Failed to uncompress chunk [%s] : no error\n",args[i]);
				if (keep_pending)
					PRINT_DEBUG("%s.pending file kept\n", args[i]);
			} else {
				PRINT_DEBUG("Chunk [%s] uncompressed\n",args[i]);
			}
		}
	}
	return rc;
}
