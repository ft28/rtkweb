#if !defined (LWS_PLUGIN_STATIC)
#define LWS_DLL
#define LWS_INTERNAL
#include "../lib/libwebsockets.h"
#endif

#include <string.h>

#define PERIOD 2000

struct per_vhost_data__test {
	uv_timer_t timeout_watcher;
	struct lws_context *context;
	struct lws_vhost *vhost;
	const struct lws_protocols *protocol;
};

struct per_session_data__test {
	int number_int;
        float number_float;
};

static void
uv_timeout_cb_test(uv_timer_t *w
#if UV_VERSION_MAJOR == 0
		, int status
#endif
)
{
	struct per_vhost_data__test *vhd = lws_container_of(w, struct per_vhost_data__test, timeout_watcher);
	lws_callback_on_writable_all_protocol_vhost(vhd->vhost, vhd->protocol);
}

static int
callback_test(struct lws *wsi, enum lws_callback_reasons reason,
			void *user, void *in, size_t len)
{
	struct per_session_data__test *pss =
			(struct per_session_data__test *)user;
	struct per_vhost_data__test *vhd =
			(struct per_vhost_data__test *)
			lws_protocol_vh_priv_get(lws_get_vhost(wsi),
					lws_get_protocol(wsi));
	unsigned char buf[LWS_PRE + 20];
	unsigned char *p = &buf[LWS_PRE];
        int n;
	size_t size_int = sizeof(pss->number_int);
	size_t size_float = sizeof(pss->number_float);

	switch (reason) {
	case LWS_CALLBACK_PROTOCOL_INIT:
		vhd = lws_protocol_vh_priv_zalloc(lws_get_vhost(wsi), lws_get_protocol(wsi), sizeof(struct per_vhost_data__test));
		vhd->context = lws_get_context(wsi);
		vhd->protocol = lws_get_protocol(wsi);
		vhd->vhost = lws_get_vhost(wsi);

		uv_timer_init(lws_uv_getloop(vhd->context, 0), &vhd->timeout_watcher);
		lws_libuv_static_refcount_add((uv_handle_t *)&vhd->timeout_watcher, vhd->context);

		uv_timer_start(&vhd->timeout_watcher, uv_timeout_cb_test, PERIOD, PERIOD);

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
                pss->number_int = 0;
                pss->number_float = 0.0f;
		lws_set_timer_usecs(wsi, 3 * LWS_USEC_PER_SEC);
		break;

	case LWS_CALLBACK_SERVER_WRITEABLE:
                pss->number_int++;
                pss->number_float = (float)(pss->number_int) / 100.0f;

                memcpy(p, &pss->number_int, size_int);
                memcpy(p + size_int, &pss->number_float, size_float);

		lwsl_notice("test write:  %d\n", pss->number_int);
		n = lws_write(wsi, p, size_int + size_float, LWS_WRITE_BINARY);
		if (n < (int)(size_int + size_float)) {
			lwsl_err("ERROR %d writing to di socket\n", n);
			return -1;
		}
		break;

	case LWS_CALLBACK_RECEIVE:
		if (len < 6)
			break;
		if (strncmp((const char *)in, "reset\n", 6) == 0)
			pss->number_int = 0;
			pss->number_float = 0.0f;
		if (strncmp((const char *)in, "closeme\n", 8) == 0) {
			lwsl_notice("test: closing as requested\n");
			lws_close_reason(wsi, LWS_CLOSE_STATUS_GOINGAWAY, (unsigned char *)"seeya", 5);
			return -1;
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

#define LWS_PLUGIN_PROTOCOL_TEST \
	{ \
		"test-protocol", \
		callback_test, \
		sizeof(struct per_session_data__test), \
		10, /* rx buf size must be >= permessage-deflate rx size */ \
		0, NULL, 0 \
	}

#if !defined (LWS_PLUGIN_STATIC)
		
static const struct lws_protocols protocols[] = {
	LWS_PLUGIN_PROTOCOL_TEST
};

LWS_EXTERN LWS_VISIBLE int
init_protocol_test(struct lws_context *context,
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
destroy_protocol_test(struct lws_context *context)
{
	return 0;
}

#endif
