#undef PACKAGE_BUGREPORT
#undef PACKAGE_NAME
#undef PACKAGE_STRING
#undef PACKAGE_TARNAME
#undef PACKAGE_VERSION

#ifdef APR_HAVE_STDIO_H
#include <stdio.h>              /* for sprintf() */
#endif

#include <unistd.h>
#include <assert.h>
#include <sys/stat.h>

#include <apr.h>
#include <apr_file_io.h>
#include <apr_strings.h>
#include <apr_buckets.h>

#include <apr_general.h>
#include <apr_thread_proc.h>

#include <httpd.h>
#include <http_log.h>
#include <http_config.h>
#include <http_protocol.h>      /* for ap_set_* (in dav_rainx_set_headers) */
#include <http_request.h>       /* for ap_update_mtime() */

#include <mod_dav.h>

// TODO FIXME replace by the APR equivalent
#include <openssl/md5.h>

#include <librain.h>

#include <metautils/lib/metautils.h>
#include <metautils/lib/metacomm.h>
#include <cluster/lib/gridcluster.h>
#include <rawx-lib/src/rawx.h>

#include <glib.h>

#include "mod_dav_rainx.h"
#include "rainx_internals.h"
#include "rainx_repository.h"
#include "rainx_http_tools.h"

/* pull this in from the other source file */
/*extern const dav_hooks_locks dav_hooks_locks_fs; */

#define POINTER_TO_REQPARAMSSTORE(p) ((struct req_params_store*)p)
#define REQPARAMSSTORE_TO_POINTER(rps) ((void*)rps)

/* forward-declare the hook structures */
static const dav_hooks_repository dav_hooks_repository_rainx;

static const dav_hooks_liveprop dav_hooks_liveprop_rainx;

/*
 ** The namespace URIs that we use. This list and the enumeration must
 ** stay in sync.
 */
static const char * const dav_rainx_namespace_uris[] =
{
	"DAV:",
	"http://apache.org/dav/props/",
	NULL        /* sentinel */
};

enum {
	DAV_FS_URI_DAV,            /* the DAV: namespace URI */
	DAV_FS_URI_MYPROPS         /* the namespace URI for our custom props */
};

static const char *const rain_algorithm_name[] = {
	"unknown",
	"liber8tion",
	"crs"
};

static apr_status_t
apr_storage_policy_clean(void *p)
{
	struct storage_policy_s *sp = (struct storage_policy_s *) p;
	storage_policy_clean(sp);
	return APR_SUCCESS;
}

static const dav_liveprop_spec dav_rainx_props[] =
{
	/* standard DAV properties */
	{
		DAV_FS_URI_DAV,
		"creationdate",
		DAV_PROPID_creationdate,
		0
	},
	{
		DAV_FS_URI_DAV,
		"getcontentlength",
		DAV_PROPID_getcontentlength,
		0
	},
	{
		DAV_FS_URI_DAV,
		"getetag",
		DAV_PROPID_getetag,
		0
	},
	{
		DAV_FS_URI_DAV,
		"getlastmodified",
		DAV_PROPID_getlastmodified,
		0
	},

	/* our custom properties */
	{
		DAV_FS_URI_MYPROPS,
		"executable",
		DAV_PROPID_FS_executable,
		0       /* handled special in dav_rainx_is_writable */
	},

	{ 0, 0, 0, 0 }        /* sentinel */
};

static const dav_liveprop_group dav_rainx_liveprop_group =
{
	dav_rainx_props,
	dav_rainx_namespace_uris,
	&dav_hooks_liveprop_rainx
};

static void* APR_THREAD_FUNC
_put_to_rawx(apr_thread_t *thd, void* params)
{
	struct req_params_store* rps = POINTER_TO_REQPARAMSSTORE(params);

	rps->req_status = rainx_http_req(rps);

	apr_thread_exit(thd, APR_SUCCESS);
	return NULL;
}

static dav_error *
rainx_repo_check_request(request_rec *req, const char *root_dir,
		const char *label, int use_checked_in, dav_resource_private *ctx,
		dav_resource **result_resource)
{

	(void) ctx;
	/* Ensure the chunkid in the URL has the approriated format and
	 * increment the request counters */
	const char *src;

	src = strrchr(req->uri, '/');
	src = src ? src + 1 : req->uri;

	if (0 == apr_strnatcasecmp(src, "info")) {
		return dav_rainx_info_get_resource(req, root_dir, label,
				use_checked_in, result_resource);
	}

	if (0 == apr_strnatcasecmp(src, "stat")) {
		return dav_rainx_stat_get_resource(req, root_dir, label,
				use_checked_in, result_resource);
	}

	return NULL;
}

/* --------------------------------------------------------------------
 **
 ** REPOSITORY HOOK FUNCTIONS
 */

static void
__load_one_header(request_rec *request, apr_uint32_t headers,
		const char *name, char **dst)
{
	const char *value;

	*dst = NULL;

	if (headers & HEADER_SCHEME_V2) {
		char new_name[strlen(name) + sizeof(HEADER_PREFIX_GRID)];
		g_snprintf(new_name, sizeof(new_name), HEADER_PREFIX_GRID"%s", name);
		if (NULL != (value = apr_table_get(request->headers_in, new_name))) {
			*dst = apr_pstrdup(request->pool, value);
			DAV_XDEBUG_REQ(request, 0, "Header found [%s]:[%s]", new_name, *dst);
		}
	}

	if (!(*dst) && (headers & HEADER_SCHEME_V1)) {
		if (NULL != (value = apr_table_get(request->headers_in, name))) {
			*dst = apr_pstrdup(request->pool, value);
			DAV_XDEBUG_REQ(request, 0, "Header found [%s]:[%s]", name, *dst);
		}
	}
}

static void
request_load_chunk_info(request_rec *request, dav_resource *resource)
{
	dav_rainx_server_conf *conf = ap_get_module_config(
			resource->info->request->server->module_config, &dav_rainx_module);

	/* These headers are used by the Integrity loop */
	LOAD_HEADER(content, path,            "content_path");
	LOAD_HEADER(content, size,            "content_size");
	LOAD_HEADER(content, chunk_nb,        "content_chunksnb");
	LOAD_HEADER(content, metadata,        "content_metadata");
	LOAD_HEADER(content, system_metadata, "content_metadata-sys");
	LOAD_HEADER(content, container_id,    "content_containerid");

	LOAD_HEADER(chunk, id,           "chunk_id");
	LOAD_HEADER(chunk, path,         "chunk_path");
	LOAD_HEADER(chunk, size,         "chunk_size");
	LOAD_HEADER(chunk, hash,         "chunk_hash");
	LOAD_HEADER(chunk, position,     "chunk_position");
	LOAD_HEADER(chunk, metadata,     "chunk_metadata");
	LOAD_HEADER(chunk, container_id, "chunk_containerid");

	if (conf->headers_scheme & HEADER_SCHEME_V1) {
		/* There are the headers used by the common client.
		 * This is an ugly clue of history and entropy */
		LOAD_HEADER(chunk, id,           "chunkid");
		LOAD_HEADER(chunk, path,         "contentpath");
		LOAD_HEADER(chunk, size,         "chunksize");
		LOAD_HEADER(chunk, hash,         "chunkhash");
		LOAD_HEADER(chunk, position,     "chunkpos");
		LOAD_HEADER(chunk, metadata,     "chunkmetadata");
		LOAD_HEADER(chunk, container_id, "containerid");

		LOAD_HEADER(content, path,            "contentpath");
		LOAD_HEADER(content, size,            "contentsize");
		LOAD_HEADER(content, chunk_nb,        "chunknb");
		LOAD_HEADER(content, metadata,        "contentmetadata");
		LOAD_HEADER(content, system_metadata, "contentmetadata-sys");
		LOAD_HEADER(content, container_id,    "containerid");
		LOAD_HEADER(content, storage_policy,    "storagepolicy");
		LOAD_HEADER(content, rawx_list, "rawxlist");
		LOAD_HEADER(content, spare_rawx_list, "sparerawxlist");
	}

	resource->info->namespace = apr_pstrdup(request->pool, conf->ns_name);
	__load_one_header(request, conf->headers_scheme,
			"namespace", &(resource->info->namespace));
}

static dav_error *
dav_rainx_get_resource(request_rec *r, const char *root_dir, const char *label,
		int use_checked_in, dav_resource **result_resource)
{
	dav_resource_private ctx;
	dav_resource *resource;
	dav_rainx_server_conf *conf;
	dav_error *e = NULL;

	*result_resource = NULL;

	(void) use_checked_in;/* No way, we do not support versioning */
	conf = request_get_server_config(r);

	/* ACL */
	/* Check if client allowed to work with us */
	if (conf->enabled_acl) {
		apr_thread_mutex_lock(conf->rainx_conf_lock);
#if MODULE_MAGIC_COOKIE == 0x41503234UL /* "AP24" */
		gboolean apo = authorized_personal_only(r->connection->client_ip,
				conf->rainx_conf->acl);
#else
		gboolean apo = authorized_personal_only(r->connection->remote_ip,
				conf->rainx_conf->acl);
#endif
		apr_thread_mutex_unlock(conf->rainx_conf_lock);
		if (!apo) {
			return server_create_and_stat_error(conf, r->pool,
					HTTP_UNAUTHORIZED, 0, "Permission Denied (APO)");
		}
	}

	/* Create private resource context descriptor */
	memset(&ctx, 0x00, sizeof(ctx));
	ctx.pool = r->pool;
	ctx.request = r;
	ctx.on_the_fly = g_str_has_prefix(r->uri, "/on-the-fly");

	e = rainx_repo_check_request(r, root_dir, label, use_checked_in,
			&ctx, result_resource);
	/* Return in case we have an error or if result_resource != null
	 * because it was an info request */
	if (NULL != e || NULL != *result_resource) {
		return e;
	}

	/* All the checks on the URL have been passed, now build a resource */

	resource = apr_pcalloc(r->pool, sizeof(*resource));
	resource->exists = 1;
	resource->collection = 0;
	resource->type = DAV_RESOURCE_TYPE_REGULAR;
	resource->info = apr_pcalloc(r->pool, sizeof(ctx));;
	memcpy(resource->info, &ctx, sizeof(ctx));
	resource->hooks = &dav_hooks_repository_rainx;
	resource->pool = r->pool;

	request_load_chunk_info(r, resource);

	/* Check META-Chunk size not larger than namespace allowed chunk-size */
	apr_thread_mutex_lock(conf->rainx_conf_lock);
	gint64 ns_chunk_size = namespace_chunk_size(conf->rainx_conf->ni,
				resource->info->namespace);
	apr_thread_mutex_unlock(conf->rainx_conf_lock);
	gint64 subchunk_size = apr_atoi64(resource->info->chunk.size);
	if (r->method_number == M_PUT && ns_chunk_size < subchunk_size) {
		return server_create_and_stat_error(conf, r->pool, HTTP_BAD_REQUEST, 0,
				apr_psprintf(resource->pool,
				"Metachunk size exceeds namespace chunk size: %"
				G_GINT64_FORMAT" (chunk) > %"G_GINT64_FORMAT" (%s)",
				subchunk_size, ns_chunk_size, resource->info->namespace));
	}


	*result_resource = resource;

	return NULL;
}

static dav_error *
dav_rainx_get_parent_resource(const dav_resource *resource,
		dav_resource **result_parent)
{
	apr_pool_t *pool;
	dav_resource *parent;

	(void) resource;
	(void) result_parent;
	pool = resource->pool;

	DAV_XDEBUG_RES(resource, 0, "%s", __FUNCTION__);

	/* Build a fake root */
	parent = apr_pcalloc(resource->pool, sizeof(*resource));
	parent->exists = 1;
	parent->collection = 1;
	parent->uri = "/";
	parent->type = DAV_RESOURCE_TYPE_WORKING;
	parent->info = NULL;
	parent->hooks = &dav_hooks_repository_rainx;
	parent->pool = pool;

	*result_parent = parent;
	return NULL;
}

static int
dav_rainx_is_same_resource(const dav_resource *res1, const dav_resource *res2)
{
	(void) res1;
	(void) res2;

	DAV_XDEBUG_RES(res1, 0, "%s", __FUNCTION__);

	/* TODO */
	return 0;
}

static int
dav_rainx_is_parent_resource(const dav_resource *res1, const dav_resource *res2)
{
	(void) res1;
	(void) res2;

	DAV_XDEBUG_RES(res1, 0, "%s", __FUNCTION__);

	return 0;
}

static dav_error *
_init_rain_encoding(dav_rainx_server_conf *srv_conf,
		const dav_resource *resource)
{
	dav_error *err = NULL;
	char *err_msg = NULL;
	dav_resource_private *res_priv = resource->info;
	char* stgpol_str = res_priv->content.storage_policy;
	struct storage_policy_s *sp = NULL;
	const struct data_security_s *datasec = NULL;
	const char *k_str = NULL, *m_str = NULL, *algo = NULL;
	long k,m;
	apr_int64_t mc_size = -1;

	apr_thread_mutex_lock(srv_conf->rainx_conf_lock);
	if (!stgpol_str ||
			!(sp = storage_policy_init(srv_conf->rainx_conf->ni, stgpol_str))) {
		err_msg = apr_psprintf(res_priv->pool,
				"\"%s\" policy init failed for namespace \"%s\"",
				stgpol_str, srv_conf->rainx_conf->ni->name);
		apr_thread_mutex_unlock(srv_conf->rainx_conf_lock);
		goto end;
	}
	DAV_DEBUG_REQ(resource->info->request, 0,
			"[%s] policy init succeeded for namespace [%s]",
			stgpol_str, srv_conf->rainx_conf->ni->name);
	apr_thread_mutex_unlock(srv_conf->rainx_conf_lock);

	apr_pool_cleanup_register(res_priv->pool, sp,
			apr_storage_policy_clean, apr_pool_cleanup_null);

	datasec = storage_policy_get_data_security(sp);
	if (RAIN != data_security_get_type(datasec)) {
		err_msg = apr_psprintf(res_priv->pool,
				"Data security type for policy '%s' is not RAIN",
				stgpol_str);
		goto end;
	}

	if (!(k_str = data_security_get_param(datasec, "k"))
			|| !(m_str = data_security_get_param(datasec, "m"))
			|| !(algo = data_security_get_param(datasec, "algo"))) {
		err_msg = apr_psprintf(res_priv->pool,
				"Failed to get parameters of policy [%s]: k=%s, m=%s, algo=%s",
				stgpol_str, k_str, m_str, algo);
		goto end;
	}

	k = strtol(k_str, NULL, 10);
	m = strtol(m_str, NULL, 10);
	if (k < 1 || m < 1 || k < m) {
		err_msg = apr_psprintf(res_priv->pool,
				"Invalid RAIN parameters: k=%ld, m=%ld. "
				"Must verify m>0 and k>=m", k, m);
		goto end;
	}

	mc_size = apr_atoi64(res_priv->chunk.size);
	if (errno != 0) {
		err_msg = apr_psprintf(res_priv->pool,
				"Failed to parse chunk size: (%d) %s",
				errno, strerror(errno));
		goto end;
	} else if (mc_size < 0) {
		err_msg = apr_psprintf(res_priv->pool,
				"Chunk size must be positive (found %"APR_INT64_T_FMT,
				mc_size);
	}

	if (!rain_get_encoding(&(res_priv->rain_params), mc_size,
			k, m, algo) && errno != 0) {
		err_msg = apr_psprintf(res_priv->pool,
					"Failed to initialize RAIN encoding: (%d) %s",
					errno, strerror(errno));
	}
	DAV_DEBUG_REQ(resource->info->request, 0,
			"[%s] policy parameters are: k=%ld, m=%ld, algo=%s",
			stgpol_str, k, m, algo);
	DAV_DEBUG_REQ(resource->info->request, 0,
			"Encoding parameters are: block_size=%lu, packet_size=%lu, "
			"strip_size=%lu",
			res_priv->rain_params.block_size,
			res_priv->rain_params.packet_size,
			res_priv->rain_params.strip_size);

end:
	if (err_msg != NULL) {
		DAV_DEBUG_REQ(resource->info->request, 0, "%s", err_msg);
		err = server_create_and_stat_error(srv_conf, res_priv->pool,
				HTTP_INTERNAL_SERVER_ERROR, 0, err_msg);
	}

	return err;
}

#define RAIN_ENV_INIT(ENV,POOL) \
	void *__rain_malloc(size_t size) { return apr_palloc(subpool, size); }\
	void *__rain_calloc(size_t nmemb, size_t size) { return apr_pcalloc(subpool, nmemb*size); }\
	void __rain_free(void *ptr) { (void)ptr; }\
	ENV = (struct rain_env_s){ __rain_malloc, __rain_calloc, __rain_free };


static dav_error *
rainx_repo_stream_create(const dav_resource *resource, dav_stream **result)
{
	dav_error *err = NULL;
	struct rain_encoding_s *rain_params = NULL;

	DAV_DEBUG_REQ(resource->info->request, 0, "%s", __FUNCTION__);

	/* Build the stream */
	apr_pool_t *pool = resource->info->pool;
	dav_stream *ds = apr_pcalloc(pool, sizeof(*ds));

	apr_pool_create(&(ds->pool), pool);
	ds->r = resource;
	/* ------- */

	dav_rainx_server_conf *conf = ap_get_module_config(
			resource->info->request->server->module_config,
			&dav_rainx_module);

	/* Storage policy management */
	/* Getting policy parameters (k, m, algo) */
	err = _init_rain_encoding(conf, resource);
	if (err != NULL)
		return err;
	rain_params = &(resource->info->rain_params);
	/* ------- */

	/* Creating data buffer and infos */
	ds->original_data_size = rain_params->data_size;
	// FIXME: use streal pool instead of resource pool?
	ds->original_data = apr_pcalloc(pool, ds->original_data_size * sizeof(char));
	ds->chunk_start_ptr = ds->original_data;
	ds->chunk_end_ptr = ds->original_data;
	ds->original_data_stored = 0;
	/* ------ */

	/* Setting metachunk info */
	resource->info->current_rawx = 0;
	resource->info->current_chunk_remaining = rain_params->block_size;
	/* ------- */

	/* Getting the rawx addresses */
	char* rawx_list = resource->info->content.rawx_list;
	if (NULL == rawx_list) {
		DAV_DEBUG_REQ(resource->info->request, 0 , "rawx list is null");
		return server_create_and_stat_error(conf, pool, HTTP_BAD_REQUEST,
				0, "Bad rawx list parameter");
	}
	resource->info->rawx_list = (char**)apr_pcalloc(pool,
			(rain_params->k + rain_params->m) * sizeof(char*));
	char* last;
	// FIXME: this should be separated by ';', not '|'
	char* temp_tok = apr_strtok(rawx_list, RAWXLIST_SEPARATOR, &last);
	unsigned int i;
	for (i = 0; temp_tok && i < rain_params->k + rain_params->m; i++) {
		resource->info->rawx_list[i] = temp_tok;
		temp_tok = apr_strtok(NULL, RAWXLIST_SEPARATOR, &last);
	}
	if (i != rain_params->k + rain_params->m) {
		DAV_DEBUG_REQ(resource->info->request, 0 ,
				"missing one or more rawx address(es)");
		return server_create_and_stat_error(conf, pool, HTTP_BAD_REQUEST,
				0, "Missing one or more rawx address(es)");
	}
	/* ------- */

	resource->info->response_chunk_list = NULL;

	MD5_Init(&(ds->md5_ctx));
	*result = ds;

	return NULL;
}

static dav_error *
dav_rainx_open_stream(const dav_resource *resource, dav_stream_mode mode,
		dav_stream **stream)
{
	dav_stream *ds = NULL;
	dav_error *e = NULL;

	(void) mode;

	DAV_DEBUG_REQ(resource->info->request, 0, "%s", __FUNCTION__);

	e = rainx_repo_stream_create(resource, &ds);
	if (NULL != e) {
		DAV_DEBUG_REQ(resource->info->request, 0,
				"Dav stream initialization failure");

		return e;
	}

	/* Thread */
	ds->data_put_params = (struct req_params_store**)apr_pcalloc(
			resource->pool,
			resource->info->rain_params.k * sizeof(struct req_params_store*));
	/* ------- */

	*stream = ds;
	return NULL;
}

static gboolean
do_rollback_specific(dav_stream *stream, char* rawx_address)
{
	char* reply = apr_pcalloc(stream->r->info->request->pool,
			MAX_REPLY_HEADER_SIZE + REPLY_BUFFER_SIZE);
	struct req_params_store rps;
	memset(&rps, 0, sizeof(rps));
	rps.pool = stream->r->info->request->pool;
	rps.reply = reply;
	rps.req_type = "DELETE";
	rps.resource = stream->r;
	rps.service_address = rawx_address;
	rainx_http_req(&rps);
	if (strlen(reply) < 12 || !g_str_has_prefix(reply, "HTTP/1.1 20"))
		return TRUE;

	return FALSE;
}

static void
do_rollback(dav_stream *stream)
{
	struct rain_encoding_s *params = &(stream->r->info->rain_params);
	for (unsigned int i = 0; i < params->k + params->m; i++)
		do_rollback_specific(stream, stream->r->info->rawx_list[i]);
}

static gboolean
extract_code_message_reply(const dav_resource* resource, char* reply,
		char** code, char** message)
{
	if (!resource || !reply || !code || !message || strlen(reply) < 12)
		return FALSE;

	char* temp_reply = apr_pstrdup(resource->info->request->pool, reply);

	/* Isolating the first line */
	char* last;
	char* reply_tok = NULL;
	reply_tok = apr_strtok(temp_reply, "\r\n", &last);
	if (!reply_tok)
		return FALSE;
	/* ------- */

	/* Isolating the HTTP version */
	char* last2;
	char* reply_tok2 = apr_strtok(reply_tok, " ", &last2);
	if (!reply_tok2 || apr_strnatcmp(reply_tok2, "HTTP/1.1"))
		return FALSE;
	/* ------- */

	/* Isolating the returned code */
	reply_tok2 = apr_strtok(NULL, " ", &last2);
	if (!reply_tok2)
		return FALSE;
	memcpy(*code, reply_tok2, (int)strlen(reply_tok2));
	/* ------- */

	/* Isolating the returned message */
	reply_tok2 = apr_strtok(NULL, " ", &last2);
	if (!reply_tok2)
		return FALSE;
	memcpy(*message, reply_tok2, (int)strlen(reply_tok2));
	/* ------- */

	return TRUE;
}

static void
update_response_list(dav_stream *stream, char* rawx_entry,
		int stored_size, char* md5_digest)
{
	// FIXME: this should be separated by ';', not '|'
	char* response_entry = apr_psprintf(stream->r->info->request->pool,
			"%s%s%d%s%s", rawx_entry, RAWXLIST_SEPARATOR, stored_size,
			RAWXLIST_SEPARATOR, md5_digest);
	if (NULL == stream->r->info->response_chunk_list)
		stream->r->info->response_chunk_list = response_entry;
	else
		stream->r->info->response_chunk_list = apr_psprintf(
				stream->r->info->request->pool, "%s%s%s",
				stream->r->info->response_chunk_list,
				RAWXLIST_SEPARATOR2, response_entry);
}

static dav_error *
dav_rainx_close_stream(dav_stream *stream, int commit)
{
	dav_error *e = NULL;
	apr_status_t rv = APR_SUCCESS;
	unsigned int i;
	char* custom_chunkid = NULL;
	char* custom_chunkpos = NULL;
	char* custom_chunksize = NULL;
	char* custom_chunkhash = NULL;
	uint8_t **coding_metachunks = NULL;
	struct req_params_store** data_put_params = stream->data_put_params;
	struct req_params_store** coding_put_params = NULL;
	struct rain_encoding_s *rain_params = &(stream->r->info->rain_params);
	struct rain_env_s rain_env;
	apr_pool_t **coding_subpools = NULL, *subpool = NULL;

	DAV_DEBUG_REQ(stream->r->info->request, 0, "Closing (%s) the stream",
			(commit ? "commit" : "rollback"));

	dav_rainx_server_conf *conf = ap_get_module_config(
			stream->r->info->request->server->module_config, &dav_rainx_module);

	if (!commit) {
		e = server_create_and_stat_error(conf, stream->pool,
				HTTP_INTERNAL_SERVER_ERROR, 0, "Rain operation failed");
		goto close_stream_error_label;
	}

	int subchunk_size = rain_params->block_size;

	/* Preparing custom header */
	struct content_textinfo_s temp_content = stream->r->info->content;
	struct chunk_textinfo_s temp_chunk = stream->r->info->chunk;
	char* custom_header = apr_psprintf(stream->r->info->request->pool,
			"containerid: %s\nchunknb: %s\ncontentpath: %s\ncontentsize: %s",
			temp_content.container_id, temp_content.chunk_nb,
			temp_content.path, temp_content.size);
	if (temp_content.metadata)
		custom_header = apr_psprintf(stream->r->info->request->pool,
				"%s\ncontentmetadata: %s", custom_header,
				temp_content.metadata);
	if (temp_content.system_metadata)
        custom_header = apr_psprintf(stream->r->info->request->pool,
				"%s\ncontentmetadata-sys: %s", custom_header,
				temp_content.system_metadata);
	/* ------- */

	/* Finalizing custom header */
	int startid = strlen(
			stream->r->info->rawx_list[stream->r->info->current_rawx]) - 64;
	custom_chunkid = apr_pstrdup(stream->r->info->request->pool,
			stream->r->info->rawx_list[stream->r->info->current_rawx]
			+ startid);
	custom_chunkpos = apr_psprintf(stream->r->info->request->pool,
			"%s.%d", temp_chunk.position, stream->r->info->current_rawx);
	custom_chunksize = apr_itoa(stream->r->info->request->pool,
			subchunk_size - stream->r->info->current_chunk_remaining);
	custom_chunkhash = g_compute_checksum_for_data(G_CHECKSUM_MD5,
			(const guchar*)stream->chunk_start_ptr,
			subchunk_size - stream->r->info->current_chunk_remaining);
	/* ------- */

	apr_pool_create(&subpool, stream->pool);
	/* Flushing the last data metachunk (without the padding) */
	if (stream->original_data_stored > stream->original_data_size) {
		// FIXME: what?
		DAV_DEBUG_REQ(stream->r->info->request,
				0, "request entity too large");
		e = server_create_and_stat_error(conf, stream->pool,
				HTTP_BAD_REQUEST, 0, "Request entity too large");
		goto close_stream_error_label;
	}

	if (subchunk_size - stream->r->info->current_chunk_remaining > 0) {
		/* Initializing the PUT params structure */
		i = stream->r->info->current_rawx;
		data_put_params[i] = (struct req_params_store*)apr_pcalloc(
				subpool, sizeof(struct req_params_store));
		data_put_params[i]->service_address = \
				stream->r->info->rawx_list[stream->r->info->current_rawx];
		data_put_params[i]->data_to_send = \
				stream->chunk_start_ptr;
		data_put_params[i]->data_to_send_size = \
				subchunk_size - stream->r->info->current_chunk_remaining;
		data_put_params[i]->header = apr_psprintf(subpool,
				"%s\nchunkid: %s\nchunkpos: %s\nchunksize: %s\nchunkhash: %s",
				custom_header, custom_chunkid, custom_chunkpos,
				custom_chunksize, custom_chunkhash);
		data_put_params[i]->req_type = "PUT";
		data_put_params[i]->reply = apr_pcalloc(subpool,
				MAX_REPLY_HEADER_SIZE + REPLY_BUFFER_SIZE);
		data_put_params[i]->resource = stream->r;
		/* APR_SUCCESS will set it to 0 */
		data_put_params[i]->req_status = INIT_REQ_STATUS;
		data_put_params[i]->pool = subpool;
		/* ------- */

		/* Launching the PUT thread */
		apr_threadattr_create(&(data_put_params[i]->thd_attr), subpool);
		rv = apr_thread_create(&(data_put_params[i]->thd_arr),
				data_put_params[i]->thd_attr, _put_to_rawx,
				REQPARAMSSTORE_TO_POINTER(data_put_params[i]), subpool);
		if (rv != APR_SUCCESS) {
			data_put_params[i]->req_status = rv;
		}
		/* ------- */

		update_response_list(stream,
				stream->r->info->rawx_list[stream->r->info->current_rawx],
				subchunk_size - stream->r->info->current_chunk_remaining,
				custom_chunkhash);
	}

	g_free(custom_chunkhash);
	custom_chunkhash = NULL;
	/* ------- */

	/* Managing the end of put data threads */
	for (i = 0; i < rain_params->k; i++) {
		if (data_put_params[i] && data_put_params[i]->thd_arr) {
			apr_thread_join(&rv, data_put_params[i]->thd_arr);
			EXTRA_ASSERT(rv == APR_SUCCESS);
		}
	}

	/* ------- */

	/* Error management */
	char* reply_code = apr_pcalloc(stream->r->info->request->pool, 4);
	char* reply_message = apr_pcalloc(stream->r->info->request->pool,
			MAX_REPLY_MESSAGE_SIZE);
	char apr_error_message[256];
	for (i = 0; i < rain_params->k; i++) {
		if (data_put_params[i] == NULL) {
			DAV_DEBUG_REQ(stream->r->info->request, 0,
					"Nothing to put on rawx %d", i);
		} else if (data_put_params[i]->req_status != APR_SUCCESS) {
			apr_strerror(data_put_params[i]->req_status, apr_error_message, sizeof(apr_error_message));
			if (!extract_code_message_reply(stream->r,
					data_put_params[i]->reply,
					&reply_code, &reply_message)) {
				DAV_DEBUG_REQ(stream->r->info->request, 0,
						"error while putting the data to the rawx %d: (%d: %s) %s",
						i, data_put_params[i]->req_status,
						apr_error_message,
						data_put_params[i]->reply);
				e = server_create_and_stat_error(conf, stream->pool,
						(data_put_params[i]->req_status == APR_TIMEUP ?
						 HTTP_GATEWAY_TIME_OUT : HTTP_INTERNAL_SERVER_ERROR), 0,
						apr_psprintf(stream->pool,
						"Rain operation failed on put: %s", apr_error_message));
				goto close_stream_error_label;
			}
			if (!g_str_has_prefix(reply_code, "20")) {
				DAV_DEBUG_REQ(stream->r->info->request, 0,
						"error while putting the data to the rawx %d: (%d:%s) '%s'",
						i, data_put_params[i]->req_status,
						apr_error_message,
						data_put_params[i]->reply);
				e = server_create_and_stat_error(conf, stream->pool,
						atoi(reply_code), 0, reply_message);
				goto close_stream_error_label;
			}
		} else {
			DAV_DEBUG_REQ(stream->r->info->request, 0, "rawx %d filled", i);
		}
	}
	/* ------- */

	/* Rain calculation */
	stream->r->info->current_rawx = rain_params->k; /* Set there to rollback correctly in case of error */
	coding_metachunks = apr_pcalloc(stream->r->info->request->pool,
			rain_params->m * sizeof(uint8_t*));

	RAIN_ENV_INIT(rain_env, subpool)
	if (!rain_encode((uint8_t*)stream->original_data, stream->original_data_size,
			rain_params, &rain_env, (uint8_t**)coding_metachunks)) {
		DAV_DEBUG_REQ(stream->r->info->request, 0,
				"failed to calculate coding chunks");
		e = server_create_and_stat_error(conf, stream->pool,
				HTTP_INTERNAL_SERVER_ERROR, 0,
				"Coding chunks calculation failed");
		goto close_stream_error_label;
	} else {
		DAV_DEBUG_REQ(stream->r->info->request, 0,
				"coding metachunks calculation succeeded");

		/* List of thread references */
		coding_put_params = (struct req_params_store**)apr_pcalloc(
				stream->r->info->request->pool,
				rain_params->m * sizeof(struct req_params_store*));
		coding_subpools = (apr_pool_t**) apr_pcalloc(
				stream->r->info->request->pool,
				rain_params->m * sizeof(apr_pool_t*));
		/* ------- */

		/* Filling the stream->r->info->m coding metachunks */
		for (i = 0; i < rain_params->m; i++) {
			/* Set there to rollback correctly in case of error */
			stream->r->info->current_rawx = rain_params->k + i;

			/* Finalizing custom header values */
			startid = strlen(stream->r->info->rawx_list[stream->r->info->current_rawx]) - 64;
			custom_chunkid = apr_pstrdup(stream->r->info->request->pool,
					stream->r->info->rawx_list[stream->r->info->current_rawx]
					+ startid);
			custom_chunkpos = apr_psprintf(stream->r->info->request->pool,
					"%s.p%d", temp_chunk.position,
					stream->r->info->current_rawx - rain_params->k);
			custom_chunksize = apr_itoa(stream->r->info->request->pool,
					subchunk_size);
			custom_chunkhash = g_compute_checksum_for_data(G_CHECKSUM_MD5,
					(const guchar*)coding_metachunks[i], subchunk_size);
			apr_pool_create(&(coding_subpools[i]), stream->pool);
			/* ------- */

			/* Initializing the PUT params structure */
			coding_put_params[i] = (struct req_params_store*)apr_pcalloc(
					coding_subpools[i], sizeof(struct req_params_store));
            coding_put_params[i]->service_address = \
					stream->r->info->rawx_list[stream->r->info->current_rawx];
            coding_put_params[i]->data_to_send = (char*)coding_metachunks[i];
            coding_put_params[i]->data_to_send_size = subchunk_size;
			coding_put_params[i]->header = apr_psprintf(coding_subpools[i],
					"%s\nchunkid: %s\nchunkpos: %s\nchunksize: %s\nchunkhash: %s",
					custom_header, custom_chunkid, custom_chunkpos,
					custom_chunksize, custom_chunkhash);
            coding_put_params[i]->req_type = "PUT";
            coding_put_params[i]->reply = apr_pcalloc(coding_subpools[i],
					MAX_REPLY_HEADER_SIZE + REPLY_BUFFER_SIZE);
			coding_put_params[i]->resource = stream->r;
			/* APR_SUCCESS will set it to 0 */
			coding_put_params[i]->req_status = INIT_REQ_STATUS;
			coding_put_params[i]->pool = coding_subpools[i];
			/* ------- */

			/* Launching the PUT thread */
			apr_threadattr_create(&(coding_put_params[i]->thd_attr),
					coding_subpools[i]);
			rv = apr_thread_create(&(coding_put_params[i]->thd_arr),
					coding_put_params[i]->thd_attr, _put_to_rawx,
					REQPARAMSSTORE_TO_POINTER(coding_put_params[i]),
					coding_subpools[i]);
			if (rv != APR_SUCCESS) {
				coding_put_params[i]->req_status = rv;
			}
			/* ------- */

			update_response_list(stream,
					stream->r->info->rawx_list[stream->r->info->current_rawx],
					subchunk_size, custom_chunkhash);

			g_free(custom_chunkhash);
			custom_chunkhash = NULL;
		}

		for (i = 0; i < rain_params->m; i++) {
			if (coding_put_params[i] && coding_put_params[i]->thd_arr) {
				apr_thread_join(&rv, coding_put_params[i]->thd_arr);
				EXTRA_ASSERT(rv == APR_SUCCESS);
			}
		}
		/* ------- */

		/* Error management */
		for (i = 0; i < rain_params->m; i++) {
			if (coding_put_params[i]->req_status != APR_SUCCESS) {
				if (!extract_code_message_reply(stream->r,
						coding_put_params[i]->reply,
						&reply_code, &reply_message)) {
					DAV_DEBUG_REQ(stream->r->info->request, 0,
							"error while putting the coding to the rawx %d: (%d) %s",
							i, coding_put_params[i]->req_status,
							coding_put_params[i]->reply);
					e = server_create_and_stat_error(conf, stream->pool,
							HTTP_INTERNAL_SERVER_ERROR, 0,
							"Rain operation failed on put");
					goto close_stream_error_label;
				}
				if (!g_str_has_prefix(reply_code, "20")) {
					DAV_DEBUG_REQ(stream->r->info->request, 0,
							"error while putting the coding to the rawx %d: (%d) %s",
							i, coding_put_params[i]->req_status,
							coding_put_params[i]->reply);
					e = server_create_and_stat_error(conf, stream->pool,
							atoi(reply_code), 0, reply_message);
					goto close_stream_error_label;
				}
			}
			else
				DAV_DEBUG_REQ(stream->r->info->request, 0,
						"coding rawx %d filled", i);
		}
		/* ------- */
	}
	/* ------- */

	/* Adding the list of actually stored metachunks
	 * (ip:port/chunk_id|stored_size|md5_digest;...)
	 * in the response header to the client */
	apr_table_setn(stream->r->info->request->headers_out,
			apr_pstrdup(stream->r->info->request->pool, "chunklist"),
			stream->r->info->response_chunk_list);
	/* ------- */

	/* stats update */
	server_inc_request_stat(resource_get_server_config(stream->r),
			RAWX_STATNAME_REQ_CHUNKPUT,
			request_get_duration(stream->r->info->request));

close_stream_error_label:
	if (e)
		do_rollback(stream);
	g_free(custom_chunkhash);

	if (coding_subpools) {
		for (i = 0; i < rain_params->m; i++)
			apr_pool_destroy(coding_subpools[i]);
	}
	apr_pool_destroy(subpool);
	apr_pool_destroy(stream->pool);
	return e;
}

static dav_error *
dav_rainx_write_stream(dav_stream *stream, const void *buf, apr_size_t bufsize)
{
	(void) buf;

	apr_status_t rv = APR_SUCCESS;
	struct req_params_store** data_put_params = stream->data_put_params;
	struct rain_encoding_s *rain_params = &(stream->r->info->rain_params);
	dav_rainx_server_conf *conf = ap_get_module_config(
			stream->r->info->request->server->module_config, &dav_rainx_module);

	apr_pool_t **data_subpools = NULL;

	if (stream->original_data_stored + (int)bufsize > stream->original_data_size) {
		/* Rollback */
		DAV_DEBUG_REQ(stream->r->info->request, 0, "request entity too large");
		return server_create_and_stat_error(conf, stream->pool,
				HTTP_BAD_REQUEST, 0, "Request entity too large");
	}

	int subchunk_size = rain_params->block_size;

	/* Buf management */
	int to_read = bufsize;
	char* buf_ptr = (char*)buf;
	/* ------- */

	/* Preparing custom header */
	struct content_textinfo_s temp_content = stream->r->info->content;
	struct chunk_textinfo_s temp_chunk = stream->r->info->chunk;
	char* custom_header = apr_psprintf(stream->r->info->request->pool,
			"containerid: %s\nchunknb: %s\ncontentpath: %s\ncontentsize: %s",
			temp_content.container_id, temp_content.chunk_nb,
			temp_content.path, temp_content.size);
	if (temp_content.metadata)
		custom_header = apr_psprintf(stream->r->info->request->pool,
				"%s\ncontentmetadata: %s", custom_header, temp_content.metadata);
	if (temp_content.system_metadata)
		custom_header = apr_psprintf(stream->r->info->request->pool,
				"%s\ncontentmetadata-sys: %s", custom_header,
				temp_content.system_metadata);
	/* ------- */

	data_subpools = (apr_pool_t**) apr_pcalloc(stream->r->info->request->pool,
			rain_params->k * sizeof(apr_pool_t*));

	/* While buf is not completely read */
	while (to_read > 0) {
		if (stream->r->info->current_chunk_remaining < to_read) {
			memcpy(stream->chunk_end_ptr, buf_ptr,
					stream->r->info->current_chunk_remaining);

			/* Updating buf state */
			buf_ptr += stream->r->info->current_chunk_remaining;
			to_read -= stream->r->info->current_chunk_remaining;

			stream->chunk_end_ptr += \
					stream->r->info->current_chunk_remaining;
			stream->original_data_stored += \
					stream->r->info->current_chunk_remaining;
			/* ------- */

			/* Finalizing custom header */
			int startid = strlen(
					stream->r->info->rawx_list[stream->r->info->current_rawx]) - 64;
			char* custom_chunkid = apr_pstrdup(stream->r->info->request->pool,
					stream->r->info->rawx_list[stream->r->info->current_rawx]
					+ startid);
			char* custom_chunkpos = apr_psprintf(stream->r->info->request->pool,
					"%s.%d", temp_chunk.position, stream->r->info->current_rawx);
			char* custom_chunksize = apr_itoa(stream->r->info->request->pool,
					subchunk_size);
			char* custom_chunkhash = g_compute_checksum_for_data(G_CHECKSUM_MD5,
					(const guchar*)stream->chunk_start_ptr,
					subchunk_size);
			/* ------- */

			/* Initializing the PUT params structure */
			int i = stream->r->info->current_rawx;
			apr_pool_create(&(data_subpools[i]), stream->pool);
			data_put_params[i] = (struct req_params_store*)apr_pcalloc(
					data_subpools[i], sizeof(struct req_params_store));
			data_put_params[i]->service_address = \
					stream->r->info->rawx_list[stream->r->info->current_rawx];
			data_put_params[i]->data_to_send = \
					stream->chunk_start_ptr;
			data_put_params[i]->data_to_send_size = subchunk_size;
			data_put_params[i]->header = apr_psprintf(data_subpools[i],
					"%s\nchunkid: %s\nchunkpos: %s\nchunksize: %s\nchunkhash: %s",
					custom_header, custom_chunkid, custom_chunkpos,
					custom_chunksize, custom_chunkhash);
			data_put_params[i]->req_type = "PUT";
			data_put_params[i]->reply = apr_pcalloc(data_subpools[i],
					MAX_REPLY_HEADER_SIZE + REPLY_BUFFER_SIZE);
			data_put_params[i]->resource = stream->r;
			/* APR_SUCCESS will set it to 0 */
			data_put_params[i]->req_status = INIT_REQ_STATUS;
			data_put_params[i]->pool = data_subpools[i];
			/* ------- */

			/* Launching the PUT thread */
			apr_threadattr_create(&(data_put_params[i]->thd_attr),
					data_subpools[i]);
			rv = apr_thread_create(&(data_put_params[i]->thd_arr),
					data_put_params[i]->thd_attr, _put_to_rawx,
					REQPARAMSSTORE_TO_POINTER(data_put_params[i]),
					data_subpools[i]);
			assert(rv == APR_SUCCESS);
			/* ------- */

			update_response_list(stream,
					stream->r->info->rawx_list[stream->r->info->current_rawx],
					subchunk_size, custom_chunkhash);

			if (custom_chunkhash) {
				g_free(custom_chunkhash);
				custom_chunkhash = NULL;
			}

			/* Updating current metachunk buffer state */
			stream->r->info->current_chunk_remaining = subchunk_size;
			stream->chunk_start_ptr = \
					stream->chunk_end_ptr;
			/* ------- */

			/* Updating general context */
			stream->r->info->current_rawx++;
			/* ------- */
		}
		else {
			memcpy(stream->chunk_end_ptr, buf_ptr, to_read);

			/* Updating buf state */
			stream->chunk_end_ptr += to_read;
			stream->original_data_stored += to_read;

			stream->r->info->current_chunk_remaining -= to_read;

			buf_ptr += to_read;
			to_read = 0;
			/* ------- */
		}
	}

	for (unsigned int i = 0; i < rain_params->k; i++) {
		if (data_put_params[i] && data_put_params[i]->thd_arr) {
			apr_status_t retval = APR_SUCCESS;
			apr_thread_join(&retval, data_put_params[i]->thd_arr);
			assert(retval == APR_SUCCESS);
			data_put_params[i]->thd_arr = NULL;
			data_put_params[i] = NULL;
		}
		if (data_subpools[i]) {
			apr_pool_destroy(data_subpools[i]);
			data_subpools[i] = NULL;
		}
	}

	/* ------- */

	MD5_Update(&(stream->md5_ctx), buf, bufsize);
	server_add_stat(resource_get_server_config(stream->r),
			RAWX_STATNAME_REP_BWRITTEN, bufsize, 0);
	return NULL;
}

static dav_error *
dav_rainx_seek_stream(dav_stream *stream, apr_off_t abs_pos)
{
	(void) abs_pos, (void) stream;

	/* Do we really need this ? */
	DAV_XDEBUG_POOL(stream->pool, 0, "%s", __FUNCTION__);
	return NULL;
}

static dav_error *
dav_rainx_set_headers(request_rec *r, const dav_resource *resource)
{
	if (!resource->exists)
		return NULL;

	DAV_DEBUG_REQ(r, 0, "%s", __FUNCTION__);
	
	ap_set_content_length(r, strtoll(resource->info->chunk.size, NULL, 10));

	return NULL;
}

static int
extract_content_from_reply(char** chunk, char* reply,
		const dav_resource *resource)
{
	if (!reply || !chunk)
		return -1;

	char* ptr_start = strstr(reply, "Content-Length");
	ptr_start += 16;
	char* ptr_end = strchr(ptr_start, '\r');
	char* content_length_str = apr_pstrndup(resource->info->request->pool,
			ptr_start, ptr_end - ptr_start);
	size_t content_length = strtoll(content_length_str, NULL, 10);

	ptr_start = strstr(reply, "\r\n\r\n");
	ptr_start += 4;
	memcpy(*chunk, ptr_start, content_length);

	return content_length;
}

static dav_error *
parse_spare_rawx_list(const dav_resource *resource, int max,
		gboolean* failure_array, char **spare_rawx_list, char** spare_md5_list)
{
	dav_error *e = NULL;
	apr_pool_t *pool = resource_get_pool(resource);
	dav_rainx_server_conf *conf = resource_get_server_config(resource);
	char *last1 = NULL;
	char *tok1 = apr_strtok(resource->info->content.spare_rawx_list,
			RAWXLIST_SEPARATOR2, &last1);
	int spare_rawx_list_pos = 0;

	while (tok1 && spare_rawx_list_pos < max) {
		char* last2;
		char* tok2 = apr_strtok(tok1, RAWXLIST_SEPARATOR, &last2);
		if (!tok2) {
			e = server_create_and_stat_error(conf, pool, HTTP_BAD_REQUEST,
					0, "Bad spare rawx address(es) format");
			DAV_DEBUG_REQ(resource->info->request,
					0, e->desc);
			goto end;
		}
		spare_rawx_list[spare_rawx_list_pos] = tok2;

		tok2 = apr_strtok(NULL, RAWXLIST_SEPARATOR, &last2);
		if (!tok2) {
			e = server_create_and_stat_error(conf, pool, HTTP_BAD_REQUEST,
					0, "Bad spare rawx address(es) format");
			DAV_DEBUG_REQ(resource->info->request, 0, e->desc);
			goto end;
		}

		errno = 0;
		apr_int64_t position = apr_atoi64(tok2);
		if (position < 0 || position >= max || errno != 0) {
			char *msg = apr_psprintf(resource->pool,
					"Invalid chunk position: %s", tok2);
			e = server_create_and_stat_error(conf, pool, HTTP_BAD_REQUEST,
					0, msg);
			DAV_DEBUG_REQ(resource->info->request, 0, msg);
			goto end;
		}
		failure_array[position] = TRUE;

		tok2 = apr_strtok(NULL, RAWXLIST_SEPARATOR, &last2);
		if (!tok2) {
			char *msg = apr_psprintf(resource->pool,
					"Bad spare rawx hash: %s", tok2);
			e = server_create_and_stat_error(
					conf, pool, HTTP_BAD_REQUEST, 0, msg);
			DAV_DEBUG_REQ(resource->info->request, 0, msg);
			goto end;
		}
		spare_md5_list[spare_rawx_list_pos] = tok2;

		spare_rawx_list_pos++;

		tok1 = apr_strtok(NULL, RAWXLIST_SEPARATOR2, &last1);
	}

	if (spare_rawx_list_pos >= max && !e) {
		e = server_create_and_stat_error(conf, pool, HTTP_BAD_REQUEST, 0,
				"Too many elements in spare rawx list");
	}

end:
	return e;
}

static dav_error *
check_reconstructed_data(const dav_resource *resource, gboolean* failure_array,
		char **datachunks, char **codingchunks, char** spare_md5_list)
{
	dav_error *e = NULL;
	int k, m, subchunk_size, metachunk_size;

	/* Testing the reconstructed data with the header md5 */
	int cur_spare_rawx = 0;
	int cumulated_data = 0;

	k = resource->info->rain_params.k;
	m = resource->info->rain_params.m;
	subchunk_size = resource->info->rain_params.block_size;
	metachunk_size = resource->info->rain_params.data_size;

	/* Data strips */
	for (int i = 0; i < k; i++) {
		if (failure_array[i]) {
			int data_to_hash_size = subchunk_size;
			if (cumulated_data + subchunk_size > metachunk_size)
				data_to_hash_size = metachunk_size - cumulated_data;
			if (data_to_hash_size < 0)
				data_to_hash_size = 0;

			char* custom_chunkhash = g_compute_checksum_for_data(
					G_CHECKSUM_MD5, (const guchar*)datachunks[i],
					data_to_hash_size);

			if (custom_chunkhash) {
				g_free(custom_chunkhash);
				custom_chunkhash = NULL;
			}

			cur_spare_rawx++;
			cumulated_data += data_to_hash_size;
		}
		else
			cumulated_data += subchunk_size;
	}

	/* Coding strips */
	for (int i = 0; i < m; i++) {
		if (failure_array[k + i]) {
			char* custom_chunkhash = g_compute_checksum_for_data(
					G_CHECKSUM_MD5, (const guchar*)codingchunks[i],
					subchunk_size);

			// FIXME: if md5 has special value "?", don't do this check, but
			// add the computed checksum as a response header (client lost it).
			if (g_ascii_strncasecmp(custom_chunkhash,
					spare_md5_list[cur_spare_rawx], strlen(custom_chunkhash))) {
				e = server_create_and_stat_error(
						resource_get_server_config(resource),
						resource_get_pool(resource),
						HTTP_INTERNAL_SERVER_ERROR, 0,
						"Failed to reconstruct a coding chunk (MD5 differs)");
				DAV_DEBUG_REQ(resource->info->request, 0, e->desc);
				if (custom_chunkhash) {
					g_free(custom_chunkhash);
					custom_chunkhash = NULL;
				}
				goto end;
			}

			if (custom_chunkhash) {
				g_free(custom_chunkhash);
				custom_chunkhash = NULL;
			}

			cur_spare_rawx++;
		}
	}
	/* ------- */

end:
	return e;
}

static dav_error *
upload_to_rawx(const dav_resource *resource, gboolean* failure_array,
		char **datachunks, char **codingchunks, char **spare_rawx_list,
		int data_rawx_list_size)
{
	dav_error *e = NULL;
	apr_status_t req_status;
	int cur_spare_rawx = 0;
	int cumulated_data = 0;
	int k = resource->info->rain_params.k;
	int m = resource->info->rain_params.m;
	int subchunk_size = resource->info->rain_params.block_size;
	int metachunk_size = resource->info->rain_params.data_size;
	char *reply, *reply_code, *reply_message;
	char* custom_header;
	struct req_params_store rps;
	struct content_textinfo_s temp_content = resource->info->content;
	struct chunk_textinfo_s temp_chunk = resource->info->chunk;

	reply = apr_palloc(resource->info->request->pool,
			MAX_REPLY_HEADER_SIZE + subchunk_size);
	reply_code = apr_pcalloc(resource->info->request->pool, 4);
	reply_message = apr_pcalloc(resource->info->request->pool,
			MAX_REPLY_MESSAGE_SIZE);

	memset(&rps, 0, sizeof(rps));
	rps.pool = resource_get_pool(resource);
	rps.reply = reply;
	rps.req_type = "PUT";
	rps.resource = resource;

	/* Preparing custom header */
	custom_header = apr_psprintf(resource->info->request->pool,
			"containerid: %s\nchunknb: %s\ncontentpath: %s\ncontentsize: %s",
			temp_content.container_id, temp_content.chunk_nb,
			temp_content.path, temp_content.size);
	if (temp_content.metadata)
		custom_header = apr_psprintf(resource->info->request->pool,
				"%s\ncontentmetadata: %s", custom_header, temp_content.metadata);
	if (temp_content.system_metadata)
		custom_header = apr_psprintf(resource->info->request->pool,
				"%s\ncontentmetadata-sys: %s", custom_header,
				temp_content.system_metadata);

	/* Data strips */
	for (int i = 0; i < data_rawx_list_size; i++) {
		int byte_count_to_send = subchunk_size;
		if (cumulated_data + subchunk_size > metachunk_size)
			byte_count_to_send = metachunk_size - cumulated_data;
		if (byte_count_to_send < 0)
			byte_count_to_send = 0;

		cumulated_data += byte_count_to_send;

		if (failure_array[i]) {
			memset(reply, 0, MAX_REPLY_HEADER_SIZE + subchunk_size);

			/* Finalizing custom header */
			int startid = strlen(spare_rawx_list[cur_spare_rawx]) - 64;
			char* custom_chunkid = apr_pstrdup(resource->info->request->pool,
					spare_rawx_list[cur_spare_rawx] + startid);
			char* custom_chunkpos = apr_psprintf(resource->info->request->pool,
					"%s.%d", temp_chunk.position, i);
			char* custom_chunksize = apr_itoa(resource->info->request->pool,
					byte_count_to_send);
			char* custom_chunkhash = g_compute_checksum_for_data(
					G_CHECKSUM_MD5, (const guchar*)datachunks[i],
					byte_count_to_send);
			char* custom_header2 = apr_psprintf(resource->info->request->pool,
					"%s\nchunkid: %s\nchunkpos: %s\nchunksize: %s\nchunkhash: %s",
					custom_header, custom_chunkid, custom_chunkpos,
					custom_chunksize, custom_chunkhash);
			/* ------- */
			rps.service_address = spare_rawx_list[cur_spare_rawx];
			rps.header = custom_header2;
			rps.data_to_send = datachunks[i];
			rps.data_to_send_size = byte_count_to_send;
			req_status = rainx_http_req(&rps);

			memset(reply_code, 0, 4);
			memset(reply_message, 0, MAX_REPLY_MESSAGE_SIZE);
			if (!extract_code_message_reply(resource, reply,
					&reply_code, &reply_message)) {
				DAV_DEBUG_REQ(resource->info->request, 0,
						"error while putting the reconstructed "
						"data to the rawx %d: %s", i, reply);
				e = server_create_and_stat_error(
						resource_get_server_config(resource),
						resource_get_pool(resource),
						HTTP_INTERNAL_SERVER_ERROR, 0,
						"Rain operation failed on put");
				if (custom_chunkhash) {
					g_free(custom_chunkhash);
					custom_chunkhash = NULL;
				}
				goto end;
			}
			if (!g_str_has_prefix(reply_code, "20")) {
				DAV_DEBUG_REQ(resource->info->request, 0,
						"error while putting the reconstructed "
						"data to the rawx %d: %s", i, reply);
				e = server_create_and_stat_error(
						resource_get_server_config(resource),
						resource_get_pool(resource),
						apr_atoi64(reply_code), 0, reply_message);
				if (custom_chunkhash) {
					g_free(custom_chunkhash);
					custom_chunkhash = NULL;
				}
				goto end;
			}
			DAV_DEBUG_REQ(resource->info->request, 0,
					"rawx %d filled (reconstructed data)", i);

			if (custom_chunkhash) {
				g_free(custom_chunkhash);
				custom_chunkhash = NULL;
			}

			cur_spare_rawx++;
		}
	}
	/* Coding strips */
	for (int i = 0; i < m; i++) {
		if (failure_array[k + i]) {
			memset(reply, 0, MAX_REPLY_HEADER_SIZE + subchunk_size);

			/* Finalizing custom header */
			int startid = strlen(spare_rawx_list[cur_spare_rawx]) - 64;
			char* custom_chunkid = apr_pstrdup(resource->info->request->pool,
					spare_rawx_list[cur_spare_rawx] + startid);
			char* custom_chunkpos = apr_psprintf(resource->info->request->pool,
					"%s.p%d", temp_chunk.position, i);
			char* custom_chunksize = apr_itoa(
					resource->info->request->pool, subchunk_size);
			char* custom_chunkhash = g_compute_checksum_for_data(
					G_CHECKSUM_MD5, (const guchar*)codingchunks[i],
					subchunk_size);
			char* custom_header2 = apr_psprintf(resource->info->request->pool,
					"%s\nchunkid: %s\nchunkpos: %s\nchunksize: %s\nchunkhash: %s",
					custom_header, custom_chunkid, custom_chunkpos,
					custom_chunksize, custom_chunkhash);
			/* ------- */
			rps.service_address = spare_rawx_list[cur_spare_rawx];
			rps.header = custom_header2;
			rps.data_to_send = codingchunks[i];
			rps.data_to_send_size = subchunk_size;
			req_status = rainx_http_req(&rps);

			memset(reply_code, 0, 4);
			memset(reply_message, 0, MAX_REPLY_MESSAGE_SIZE);
			if (!extract_code_message_reply(resource, reply,
						&reply_code, &reply_message)) {
				// FIXME: use apr_strerror() to get a clear message
				DAV_DEBUG_REQ(resource->info->request, 0,
						"error while putting the reconstructed coding "
						"to the rawx %d: (%d) %s", i, req_status, reply);
				e = server_create_and_stat_error(
						resource_get_server_config(resource),
						resource_get_pool(resource),
						HTTP_INTERNAL_SERVER_ERROR,
						0, "Rain operation failed on put");
				if (custom_chunkhash) {
					g_free(custom_chunkhash);
					custom_chunkhash = NULL;
				}
				goto end;
			}
			if (!g_str_has_prefix(reply_code, "20")) {
				DAV_DEBUG_REQ(resource->info->request, 0,
						"error while putting the reconstructed coding "
						"to the rawx %d: %s", i, reply);
				e = server_create_and_stat_error(
						resource_get_server_config(resource),
						resource_get_pool(resource),
						apr_atoi64(reply_code), 0, reply_message);
				if (custom_chunkhash) {
					g_free(custom_chunkhash);
					custom_chunkhash = NULL;
				}
				goto end;
			}
			DAV_DEBUG_REQ(resource->info->request, 0,
					"rawx %d filled (reconstructed coding)", i);

			if (custom_chunkhash) {
				g_free(custom_chunkhash);
				custom_chunkhash = NULL;
			}

			cur_spare_rawx++;
		}
	}
end:
	return e;
}

static dav_error *
_send_reconstructed_data(const dav_resource *resource, ap_filter_t *output,
		char **data)
{
	dav_error *err = NULL;
	struct rain_encoding_s *rain_params = &(resource->info->rain_params);
	size_t expected_size = rain_params->data_size;
	size_t sent_size = 0;
	apr_bucket_brigade *bb = apr_brigade_create(
			resource->info->request->pool, output->c->bucket_alloc);
	for (unsigned int i = 0; i < rain_params->k && sent_size < expected_size; i++) {
		// Last subchunk may be smaller, and there may be less than k subchunks
		size_t out_size = MIN(expected_size-sent_size, rain_params->block_size);
		DAV_DEBUG_REQ(resource->info->request, 0, "writing %lu bytes, pos %u",
				out_size, i);
		apr_brigade_write(bb, NULL, resource->info, data[i], out_size);
		sent_size += out_size;
		DAV_DEBUG_REQ(resource->info->request, 0, "%lu bytes sent", sent_size);
	}
	if (sent_size != expected_size) {
		DAV_ERROR_REQ(resource->info->request, 0,
				"Expected %lu bytes, sent %lu bytes",
				expected_size, sent_size);
	}
	if (ap_pass_brigade(output, bb) != APR_SUCCESS) {
		char *err_msg = "could not write content to filter";
		DAV_DEBUG_REQ(resource->info->request, 0, err_msg);
		err = server_create_and_stat_error(resource_get_server_config(resource),
				resource->pool, HTTP_FORBIDDEN, 0, err_msg);
	} else {
		DAV_DEBUG_REQ(resource->info->request, 0,
				"Reconstructed metachunk sent to client");
	}
	return err;
}

static dav_error *
dav_rainx_deliver(const dav_resource *resource, ap_filter_t *output)
{
	/* GET MAIN METHOD */

	dav_rainx_server_conf *conf;
	apr_pool_t *pool, *subpool;
	dav_error *err = NULL;
	unsigned int i;
	char* reply = NULL;
	/* The array showing if a rawx is a failure */
	gboolean* failure_array = NULL;
	/* The array containing the spared rawx addresses */
	char** spare_rawx_list = NULL;
	/* The array containing the hash to check after reconstruction of a chunk */
	char** spare_md5_list = NULL;
	struct rain_encoding_s *rain_params;
	struct rain_env_s rain_env;

	pool = resource->pool;
	conf = resource_get_server_config(resource);
	rain_params = &(resource->info->rain_params);

	apr_pool_create(&subpool, pool);

	/* Check resource type */
	if (DAV_RESOURCE_TYPE_REGULAR != resource->type) {
		err = server_create_and_stat_error(conf, pool, HTTP_CONFLICT, 0,
			"Cannot GET this type of resource.");
		goto end_deliver;
	}

	if (resource->collection) {
		err = server_create_and_stat_error(conf, pool, HTTP_CONFLICT, 0,
			"No GET on collections");
		goto end_deliver;
	}
	/* ------- */

	/* Storage policy management (storage policy name got from the header) */
	/* Getting policy parameters (k, m, algo) */
	err = _init_rain_encoding(conf, resource);
	/* ------- */

	/* Getting the rawx addresses from the header */
	if (NULL == resource->info->content.rawx_list) {
		DAV_DEBUG_REQ(resource->info->request, 0 , "rawx list is null");
		err = server_create_and_stat_error(conf, pool, HTTP_BAD_REQUEST, 0,
				"Bad rawx list parameter");
		goto end_deliver;
	}
	/* The given rawx addresses list */
	char** rawx_list =(char**)apr_pcalloc(resource->info->request->pool,
			(rain_params->k + rain_params->m) * sizeof(char*));
	char* last;
	// FIXME: this should be separated by ';', not '|'
	char* temp_tok = apr_strtok(resource->info->content.rawx_list,
			RAWXLIST_SEPARATOR, &last);
	for (i = 0; temp_tok != NULL && i < rain_params->k + rain_params->m; i++) {
		rawx_list[i] = temp_tok;
		temp_tok = apr_strtok(NULL, RAWXLIST_SEPARATOR, &last);
	}
	/* The number of given data rawx addresses */
	unsigned int data_rawx_list_size = i - rain_params->m;
	/* ------- */

	/* Getting the spare rawx addresses, the failures array,
	 * and the original MD5 array from the header */
	if (NULL == resource->info->content.spare_rawx_list) {
		DAV_DEBUG_REQ(resource->info->request, 0 , "spare rawx list is null");
		err = server_create_and_stat_error(conf, pool, HTTP_BAD_REQUEST,
				0, "Bad spare rawx list parameter");
		goto end_deliver;
	}

	int total_subchunks = (rain_params->k + rain_params->m);
	failure_array = (gboolean*)apr_pcalloc(resource->info->request->pool,
			total_subchunks * sizeof(gboolean));
	spare_rawx_list = (char**)apr_pcalloc(resource->info->request->pool,
			total_subchunks * sizeof(char*));
	spare_md5_list = (char**)apr_pcalloc(resource->info->request->pool,
			total_subchunks * sizeof(char*));
	err = parse_spare_rawx_list(resource, total_subchunks,
			failure_array, spare_rawx_list, spare_md5_list);
	if (err)
		goto end_deliver;
	/* ------- */

	/* Creating data strips */
	char** datachunks = (char**)apr_pcalloc(resource->info->request->pool,
			rain_params->k * sizeof(char*));
	char* reply_code = apr_pcalloc(resource->info->request->pool, 4);
	char* reply_message = apr_pcalloc(resource->info->request->pool,
			MAX_REPLY_MESSAGE_SIZE);
	reply = apr_palloc(resource->info->request->pool,
			MAX_REPLY_HEADER_SIZE + rain_params->block_size);
	struct req_params_store rps;
	memset(&rps, 0, sizeof(rps));
	rps.data_to_send_size = rain_params->block_size;
	rps.pool = resource->info->request->pool;
	rps.reply = reply;
	rps.req_type = "GET";
	rps.resource = resource;
	for (i = 0; i < data_rawx_list_size; i++) {
		if (!failure_array[i]) {
			datachunks[i] = (char*)apr_pcalloc(resource->info->request->pool,
					rain_params->block_size * sizeof(char));
			memset(reply, 0, MAX_REPLY_HEADER_SIZE + rain_params->block_size);
			rps.service_address = rawx_list[i];
			rainx_http_req(&rps);

			/* Error management */
			memset(reply_code, 0, 4);
			memset(reply_message, 0, MAX_REPLY_MESSAGE_SIZE);
			if (!extract_code_message_reply(resource, reply,
					&reply_code, &reply_message) ||
					!g_str_has_prefix(reply_code, "20")) {
				DAV_DEBUG_REQ(resource->info->request,
						0, "unexpected failure on data rawx %d: %s", i, reply);
				err = server_create_and_stat_error(conf, pool, HTTP_BAD_REQUEST,
						0, "Unexpected failure on data rawx");
				goto end_deliver;
			}
			DAV_DEBUG_REQ(resource->info->request,
					0, "got the data chunk from the rawx %d", i);
			/* ------- */

			int lcont = extract_content_from_reply(datachunks + i, reply, resource);
			if (-1 == lcont) {
				DAV_DEBUG_REQ(resource->info->request,
						0, "problem occured while extracting the content "
						"got from the data rawx %d", i);
				err = server_create_and_stat_error(conf, pool,
						HTTP_INTERNAL_SERVER_ERROR, 0,
						"Problem occured while extracting the content got from a data rawx");
				goto end_deliver;
			}
		}
		else
			datachunks[i] = NULL;
	}
	/* If it remains entire padded chunks at the end */
	for (i = data_rawx_list_size; i < rain_params->k; i++) {
		DAV_DEBUG_REQ(resource->info->request, 0,
				"Fill chunk %d with zeros", i);
		datachunks[i] = (char*)apr_pcalloc(resource->info->request->pool,
				rain_params->block_size * sizeof(char));
	}
	/* ------- */

	/* Creating coding strips */
	char** codingchunks = (char**)apr_pcalloc(resource->info->request->pool,
			rain_params->m * sizeof(char*));
	memset(&rps, 0, sizeof(rps));
	rps.data_to_send_size = rain_params->block_size;
	rps.pool = resource->info->request->pool;
	rps.reply = reply;
	rps.req_type = "GET";
	rps.resource = resource;
	for (i = 0; i < rain_params->m; i++) {
		if (!failure_array[rain_params->k + i]) {
			codingchunks[i] = (char*)apr_pcalloc(resource->info->request->pool,
					rain_params->block_size * sizeof(char));

			memset(reply, 0, MAX_REPLY_HEADER_SIZE + rain_params->block_size);
			rps.service_address = rawx_list[data_rawx_list_size + i];
			rainx_http_req(&rps);

			/* Error management */
			memset(reply_code, 0, 4);
			memset(reply_message, 0, MAX_REPLY_MESSAGE_SIZE);
			if (!extract_code_message_reply(resource, reply,
						&reply_code, &reply_message) ||
						!g_str_has_prefix(reply_code, "20")) {
				DAV_DEBUG_REQ(resource->info->request,
						0, "unexpected failure on data rawx %d: %s", i, reply);
				err = server_create_and_stat_error(conf, pool, HTTP_BAD_REQUEST,
						0, "Unexpected failure on data rawx");
				goto end_deliver;
			}
			DAV_DEBUG_REQ(resource->info->request,
					0, "got the coding chunk from the rawx %d", i);
			/* ------- */

			int lcont = extract_content_from_reply(codingchunks + i,
					reply, resource);
			if (-1 == lcont) {
				DAV_DEBUG_REQ(resource->info->request, 0,
						"problem occured while extracting the content "
						"got from the coding rawx %d", i);
				err = server_create_and_stat_error(conf, pool,
						HTTP_INTERNAL_SERVER_ERROR, 0,
						"Problem occured while extracting the content "
						"got from a coding rawx");
				goto end_deliver;
			}
		}
		else
			codingchunks[i] = NULL;
	}
	/* ------- */

	/* Repairing lost data or coding subchunks */
	RAIN_ENV_INIT(rain_env, subpool)
	if (!rain_rehydrate((uint8_t**)datachunks, (uint8_t**)codingchunks,
			rain_params, &rain_env)) {
		char *err_msg = apr_pstrdup(pool, "Failed to reconstruct the original data");
		DAV_DEBUG_REQ(resource->info->request, 0, err_msg);
		err = server_create_and_stat_error(conf, pool,
				HTTP_INTERNAL_SERVER_ERROR, 0, err_msg);
		goto end_deliver;
	}
	/* ------- */

	/* Testing the reconstructed data with the header md5 */
	err = check_reconstructed_data(resource, failure_array, datachunks,
			codingchunks, spare_md5_list);
	if (err)
		goto end_deliver;
	/* ------- */

	/* Putting reconstructed subchunks into new rawx */
	if (!resource->info->on_the_fly) {
		err = upload_to_rawx(resource, failure_array, datachunks, codingchunks,
				spare_rawx_list, data_rawx_list_size);
		if (err)
			goto end_deliver;
	}
	/* ------- */

	/* Returning the whole reconstructed data */
	err = _send_reconstructed_data(resource, output, datachunks);
	/* ------- */

	server_inc_stat(conf, RAWX_STATNAME_REP_2XX, 0);

end_deliver:

	apr_pool_destroy(subpool);
	/* Now we pass here even if an error occured, for process request duration */
	server_inc_request_stat(resource_get_server_config(resource),
			RAWX_STATNAME_REQ_CHUNKGET,
			request_get_duration(resource->info->request));

	return err;
}

static dav_error *
dav_rainx_remove_resource(dav_resource *resource, dav_response **response)
{

	/* DELETE MAIN FUNC */

	apr_pool_t *pool;
	dav_error *e = NULL;

	DAV_XDEBUG_RES(resource, 0, "%s", __FUNCTION__);
	pool = resource->pool;
	*response = NULL;

	if (DAV_RESOURCE_TYPE_REGULAR != resource->type)  {
		e = server_create_and_stat_error(resource_get_server_config(resource),
				pool, HTTP_CONFLICT, 0, "Cannot DELETE this type of resource.");
		goto end_remove;
	}
	if (resource->collection) {
		e = server_create_and_stat_error(resource_get_server_config(resource),
				pool, HTTP_CONFLICT, 0, "No DELETE on collections");
		goto end_remove;
	}

	resource->exists = 0;
	resource->collection = 0;

	server_inc_stat(resource_get_server_config(resource),
			RAWX_STATNAME_REP_2XX, 0);

end_remove:

	/* Now we pass here even if an error occured, for process request duration */
	server_inc_request_stat(resource_get_server_config(resource),
			RAWX_STATNAME_REQ_CHUNKDEL,
			request_get_duration(resource->info->request));

	return e;
}

/* XXX JFS: etags are strings that uniquely identify a content.
 * A chunk is unique in a namespace, thus the e-tag must contain
 * both fields. */
static const char *
dav_rainx_getetag(const dav_resource *resource)
{
	/* return etag */
	const char *etag;

	if (!resource->exists) {
		DAV_DEBUG_RES(resource, 0, "%s : resource not found",
				__FUNCTION__);

		return NULL;
	}

	etag = apr_psprintf(resource->pool, "Dummy ETag, not yet computed");
	DAV_DEBUG_RES(resource, 0, "%s : ETag=[%s]", __FUNCTION__, etag);

	return etag;
}

/* XXX JFS : rainx walks are dummy*/
static dav_error *
dav_rainx_walk(const dav_walk_params *params, int depth, dav_response **response)
{
	dav_walk_resource wres;
	dav_error *err;

	(void) depth;
	err = NULL;
	memset(&wres, 0x00, sizeof(wres));
	wres.walk_ctx = params->walk_ctx;
	wres.pool = params->pool;
	wres.resource = params->root;

	DAV_XDEBUG_RES(params->root, 0, "sanity checks on resource");

	if (wres.resource->type != DAV_RESOURCE_TYPE_REGULAR)
		return server_create_and_stat_error(
				resource_get_server_config(params->root),
				params->root->pool, HTTP_CONFLICT, 0,
				"Only regular resources can be deleted with RAWX");
	if (wres.resource->collection)
		return server_create_and_stat_error(
				resource_get_server_config(params->root),
				params->root->pool, HTTP_CONFLICT, 0,
				"Collection resources canot be deleted with RAWX");
	if (!wres.resource->exists)
		return server_create_and_stat_error(
				resource_get_server_config(params->root),
				params->root->pool, HTTP_NOT_FOUND, 0,
				"Resource not found (no chunk)");

	err = (*params->func)(&wres, DAV_CALLTYPE_MEMBER);
	*response = wres.response;
	return err;
}

static const dav_hooks_repository dav_hooks_repository_rainx =
{
	1,
	dav_rainx_get_resource,
	dav_rainx_get_parent_resource,
	dav_rainx_is_same_resource,
	dav_rainx_is_parent_resource,
	dav_rainx_open_stream,
	dav_rainx_close_stream,
	dav_rainx_write_stream,
	dav_rainx_seek_stream,
	dav_rainx_set_headers,
	dav_rainx_deliver,
	NULL /* no collection creation */,
	NULL /* no copy of resources allowed */,
	NULL /* cannot move resources */,
	dav_rainx_remove_resource /*only for regular resources*/,
	dav_rainx_walk /* no walk across the chunks */,
	dav_rainx_getetag,
	NULL, /* no module context */
#if MODULE_MAGIC_COOKIE == 0x41503234UL /* "AP24" */
	NULL,
	NULL,
#endif
};

static const dav_provider dav_rainx_provider =
{
	&dav_hooks_repository_rainx,
	&dav_hooks_db_dbm,
	NULL,               /* no lock management */
	NULL,               /* vsn */
	NULL,               /* binding */
	NULL,               /* search */
	NULL                /* ctx */
};


void
dav_rainx_register(apr_pool_t *p)
{
	dav_register_provider(p, "rainx", &dav_rainx_provider);
}
