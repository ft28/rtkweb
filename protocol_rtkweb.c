#if !defined (LWS_PLUGIN_STATIC)
#define LWS_DLL
#define LWS_INTERNAL
#include <libwebsockets.h>
#endif

#include <string.h>
#include "rtkweb.h"


#define PERIOD 2000

struct per_vhost_data__rtkweb {
	uv_timer_t timeout_watcher;
	struct lws_context *context;
	struct lws_vhost *vhost;
	const struct lws_protocols *protocol;
        Record record;
	lws_pthread_mutex(lock);
};

struct per_session_data__rtkweb {
};

static void
uv_timeout_cb_rtkweb(uv_timer_t *w
#if UV_VERSION_MAJOR == 0
		, int status
#endif
)
{
	struct per_vhost_data__rtkweb *vhd = lws_container_of(w, struct per_vhost_data__rtkweb, timeout_watcher);

	lws_pthread_mutex_lock(&vhd->lock);
        update_record(&vhd->record);
        lws_pthread_mutex_unlock(&vhd->lock);

	lws_callback_on_writable_all_protocol_vhost(vhd->vhost, vhd->protocol);
}

static int
callback_rtkweb(struct lws *wsi, enum lws_callback_reasons reason,
			void *user, void *in, size_t len)
{
        /*
	struct per_session_data__rtkweb *pss =
			(struct per_session_data__rtkweb *)user;
        */
	struct per_vhost_data__rtkweb *vhd =
			(struct per_vhost_data__rtkweb *)
			lws_protocol_vh_priv_get(lws_get_vhost(wsi),
					lws_get_protocol(wsi));
	unsigned char buf[LWS_PRE + 1024];
	unsigned char *p = &buf[LWS_PRE];
        int size_record = (int)sizeof(Record);
        int n;

	switch (reason) {
	case LWS_CALLBACK_PROTOCOL_INIT:
		vhd = lws_protocol_vh_priv_zalloc(lws_get_vhost(wsi), lws_get_protocol(wsi), sizeof(struct per_vhost_data__rtkweb));
		vhd->context = lws_get_context(wsi);
		vhd->protocol = lws_get_protocol(wsi);
		vhd->vhost = lws_get_vhost(wsi);
		open_rtksvr("/opt/rtkweb/sample", 0, 0);

		uv_timer_init(lws_uv_getloop(vhd->context, 0), &vhd->timeout_watcher);
		lws_libuv_static_refcount_add((uv_handle_t *)&vhd->timeout_watcher, vhd->context);

		uv_timer_start(&vhd->timeout_watcher, uv_timeout_cb_rtkweb, PERIOD, PERIOD);

		break;

	case LWS_CALLBACK_PROTOCOL_DESTROY:
		if (!vhd) {
			break;
                }
		lwsl_notice("di: LWS_CALLBACK_PROTOCOL_DESTROY: v=%p, ctx=%p\n", vhd, vhd->context);
		uv_timer_stop(&vhd->timeout_watcher);
		uv_close((uv_handle_t *)&vhd->timeout_watcher, lws_libuv_static_refcount_del);
		break;

	case LWS_CALLBACK_ESTABLISHED:
		lws_set_timer_usecs(wsi, 3 * LWS_USEC_PER_SEC);
		break;

	case LWS_CALLBACK_SERVER_WRITEABLE:
                lws_pthread_mutex_lock(&vhd->lock);
                memcpy(p, &vhd->record, size_record);
                lws_pthread_mutex_unlock(&vhd->lock);

		n = lws_write(wsi, p, size_record,  LWS_WRITE_BINARY);
		if (n < size_record) {
			lwsl_err("ERROR %d writing to di socket\n", n);
			return -1;
		}
		break;

	case LWS_CALLBACK_RECEIVE:
		if (len < 5)
			break;
		if (strncmp((const char *)in, "start\n", 6) == 0)
			start_rtksvr();
			lwsl_notice("rtkweb: start\n");
		if (strncmp((const char *)in, "stop\n", 5) == 0) {
			stop_rtksvr();
			lwsl_notice("rtkweb: stop\n");
		}
		break;

	case LWS_CALLBACK_TIMER:
		lwsl_info("%s: LWS_CALLBACK_TIMER: %p\n", __func__, wsi);
		lws_set_timer_usecs(wsi, 3 * LWS_USEC_PER_SEC);
		break;

	default:
		break;
	}

	return 0;
}

#define LWS_PLUGIN_PROTOCOL_RTKWEB \
	{ \
		"rtkweb-protocol", \
		callback_rtkweb, \
		sizeof(struct per_session_data__rtkweb), \
		1024, /* rx buf size must be >= permessage-deflate rx size */ \
		0, NULL, 0 \
	}

#if !defined (LWS_PLUGIN_STATIC)
		
static const struct lws_protocols protocols[] = {
	LWS_PLUGIN_PROTOCOL_RTKWEB
};

LWS_EXTERN LWS_VISIBLE int
init_protocol_rtkweb(struct lws_context *context,
			     struct lws_plugin_capability *c)
{
	if (c->api_magic != LWS_PLUGIN_API_MAGIC) {
		lwsl_err("Plugin API %d, library API %d", LWS_PLUGIN_API_MAGIC,
			 c->api_magic);
		return 1;
	}

	c->protocols = protocols;
	c->count_protocols = ARRAY_SIZE(protocols);
	c->extensions = NULL;
	c->count_extensions = 0;

	return 0;
}

LWS_EXTERN LWS_VISIBLE int
destroy_protocol_rtkweb(struct lws_context *context)
{
	return 0;
}

#endif
