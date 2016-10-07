#ifndef G_LOG_DOMAIN
# define G_LOG_DOMAIN "grid.meta2.disp"
#endif

#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include <metautils/lib/metautils.h>
#include <metautils/lib/metacomm.h>

#include <glib.h>

#include <server/transport_gridd.h>
#include <server/gridd_dispatcher_filters.h>

#include <meta2v2/meta2_macros.h>
#include <meta2v2/meta2_filter_context.h>
#include <meta2v2/meta2_filters.h>
#include <meta2v2/meta2_backend_internals.h>
#include <meta2v2/meta2_bean.h>
#include <meta2v2/meta2v2_remote.h>
#include <meta2v2/generic.h>
#include <meta2v2/autogen.h>

struct on_bean_ctx_s *
_on_bean_ctx_init(struct gridd_filter_ctx_s *ctx,
		struct gridd_reply_ctx_s *reply)
{
	struct on_bean_ctx_s * obc = g_malloc0(sizeof(struct on_bean_ctx_s));
	obc->l = NULL;
	obc->all = NULL;
	obc->first = TRUE;
	obc->ctx = ctx;
	obc->reply = reply;
	return obc;
}

void
_on_bean_ctx_append_all(struct on_bean_ctx_s *obc)
{
	struct meta2_backend_s *m2b = meta2_filter_ctx_get_backend(obc->ctx);
	struct hc_url_s *url = meta2_filter_ctx_get_url(obc->ctx);
	struct event_config_s * evt_config = meta2_backend_get_event_config(m2b,
			hc_url_get(url, HCURL_NS));

	if (event_is_enabled(evt_config) || event_is_notifier_enabled(evt_config)) {
		obc->all = g_slist_concat(obc->l, obc->all);
	} else {
		GRID_TRACE("Events disabled, cleaning beans immediately");
		_bean_cleanl2(obc->l);
		obc->l = NULL;
	}
}

/**
 * Send list of beans to the client. Possibly keep a copy of
 * the list if we need it for notifications purposes.
 */
void
_on_bean_ctx_send_list(struct on_bean_ctx_s *obc, gboolean final)
{
	/* marshall the list, send and clean it */
	if (NULL != obc->l) {
		obc->reply->add_body(bean_sequence_marshall(obc->l));
		_on_bean_ctx_append_all(obc);
	}
	if (final)
		obc->reply->send_reply(CODE_FINAL_OK, "OK");
	else
		obc->reply->send_reply(CODE_PARTIAL_CONTENT, "Partial content");
	obc->l = NULL;
}

void
_on_bean_ctx_clean(struct on_bean_ctx_s *obc)
{
	if(!obc)
		return;

	struct meta2_backend_s *m2b = meta2_filter_ctx_get_backend(obc->ctx);
	struct hc_url_s *url = meta2_filter_ctx_get_url(obc->ctx);
	struct event_config_s * evt_config = meta2_backend_get_event_config(m2b,
			hc_url_get(url, HCURL_NS));

	if (obc->l) {
		if (!(event_is_enabled(evt_config) || event_is_notifier_enabled(evt_config)))
			_bean_cleanl2(obc->l);
		obc->l = NULL;
	}
	if (obc->all) {
		meta2_filter_ctx_set_input_udata(obc->ctx, obc->all,
				(GDestroyNotify)_bean_cleanl2);
		obc->all = NULL;
	}
	obc->reply = NULL;
	obc->ctx = NULL;
	g_free(obc);
}

int
meta2_filter_fill_subject(struct gridd_filter_ctx_s *ctx,
		struct gridd_reply_ctx_s *reply)
{
	struct hc_url_s *url;

	TRACE_FILTER();
	url = meta2_filter_ctx_get_url(ctx);
	if (hc_url_has(url, HCURL_REFERENCE))
		reply->subject("%s|%s", hc_url_get(url, HCURL_WHOLE),
				hc_url_get(url, HCURL_HEXID));
	else
		reply->subject("%s|%s", hc_url_get(url, HCURL_NS),
				hc_url_get(url, HCURL_HEXID));
	return FILTER_OK;
}

#define FILL_URL_FIELD(K, F) do { \
	tmp = meta2_filter_ctx_get_param(ctx, K); \
	if(NULL != tmp) { \
		hc_url_set(url, F, tmp); \
		tmp = NULL; \
	} \
} while(0)

int
meta2_filter_pack_url(struct gridd_filter_ctx_s *ctx,
		struct gridd_reply_ctx_s *reply)
{
	struct hc_url_s *url = NULL;
	const char *tmp = NULL;
	char *hexid = NULL;

	TRACE_FILTER();
	(void) reply;

	if (!(url = meta2_filter_ctx_get_url(ctx))) {
		GRID_DEBUG("URL NOT FOUND in CONTEXT, create it");
		url = hc_url_empty();
		meta2_filter_ctx_set_url(ctx, url);
	}

	GRID_DEBUG("HEXID : %s", hc_url_get(url, HCURL_HEXID));
	if(hc_url_has(url, HCURL_HEXID)) {
		hexid = g_strdup(hc_url_get(url, HCURL_HEXID));
	}

	FILL_URL_FIELD(M2V1_KEY_VIRTUAL_NAMESPACE, HCURL_NS);
	FILL_URL_FIELD(M2V1_KEY_REF, HCURL_REFERENCE);
	FILL_URL_FIELD(M2V1_KEY_REFID, HCURL_HEXID);
	FILL_URL_FIELD(M2V1_KEY_PATH, HCURL_PATH);

	if(!hc_url_has(url, HCURL_NS))  {
		const struct meta2_backend_s *backend = meta2_filter_ctx_get_backend(ctx);
		url = hc_url_set(url, HCURL_NS, backend->backend.ns_name);
	}

	if(NULL != hexid) {
		hc_url_set(url, HCURL_HEXID, hexid);
		g_free(hexid);
	}

	// Hack in case there was "?version=XXX" in M2V1_KEY_PATH
	struct hc_url_s *url2 = hc_url_init(hc_url_get(url, HCURL_WHOLE));
	hc_url_set(url, HCURL_PATH, hc_url_get(url2, HCURL_PATH));
	if (hc_url_has(url2, HCURL_SNAPORVERS)) {
		hc_url_set(url, HCURL_SNAPORVERS, hc_url_get(url2, HCURL_SNAPORVERS));
	}
	hc_url_clean(url2);

	return FILTER_OK;
}

inline guint32
meta2_filter_get_flags(const struct gridd_filter_ctx_s *ctx)
{
	const char *fstr = meta2_filter_ctx_get_param(ctx, M2_KEY_GET_FLAGS);
	if (NULL != fstr)
		return (guint32) g_ascii_strtoull(fstr, NULL, 10);
	return 0;
}

int
meta2_filter_fail_reply(struct gridd_filter_ctx_s *ctx,
		struct gridd_reply_ctx_s *reply)
{
	GError *e = NULL;

	TRACE_FILTER();
	e = meta2_filter_ctx_get_error(ctx);
	if(NULL != e) {
		GRID_DEBUG("Error defined by KO execution filter, return it");
		reply->send_error(0, e);
	} else {
		GRID_DEBUG("Error not defined by KO execution filter, return 500");
		reply->send_error(0, NEWERROR(500,
					"Request execution failed : No error"));
	}

	return FILTER_OK;
}

int
meta2_filter_success_reply(struct gridd_filter_ctx_s *ctx,
		struct gridd_reply_ctx_s *reply)
{
	TRACE_FILTER();
	(void) ctx;
	reply->send_reply(200, "OK");
	return FILTER_OK;
}

int
meta2_filter_not_implemented_reply(struct gridd_filter_ctx_s *ctx,
		struct gridd_reply_ctx_s *reply)
{
	TRACE_FILTER();
	(void) ctx;
	reply->send_reply(501, "NOT IMPLEMENTED");
	return FILTER_OK;
}
