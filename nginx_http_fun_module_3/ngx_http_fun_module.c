#include <ngx_config.h>
#include <ngx_core.h>
#include <ngx_http.h>

#include <cairo.h>

#define M_PI 3.14159265

struct closure {
    ngx_http_request_t *r;
    ngx_chain_t *chain;
    uint32_t length;
};

static char * ngx_http_fun(ngx_conf_t * cf, ngx_command_t * cmd, void * conf);

typedef struct {
    ngx_uint_t   radius;
} ngx_http_fun_loc_conf_t;

static ngx_command_t ngx_http_fun_commands[] = {
    {   // Our command is named "fun":
        ngx_string("fun"),
        // The directive may be specified in the location-level of your nginx-config.
        // The directive does not take any arguments (NGX_CONF_NOARGS)
        NGX_HTTP_LOC_CONF|NGX_CONF_NOARGS,
        // A pointer to our handler-function.
        ngx_http_fun,
        // We're not using these two, they're related to the configuration structures.
        0, 0,
        // A pointer to a post-processing handler.
        NULL 
    },
    {   // New parameter: "fun_radius":
        ngx_string("fun_radius"),
        // Can be specified on the main level of the config,
        // can be specified in the server level of the config,
        // can be specified in the location level of the config,
        // the directive takes 1 argument (NGX_CONF_TAKE1)
        NGX_HTTP_MAIN_CONF|NGX_HTTP_SRV_CONF|NGX_HTTP_LOC_CONF|NGX_CONF_TAKE1,
        // A builtin function for setting numeric variables
        ngx_conf_set_num_slot,
        // We tell nginx how we're storing the config.
        NGX_HTTP_LOC_CONF_OFFSET,
        offsetof(ngx_http_fun_loc_conf_t, radius),
        NULL
    },
    ngx_null_command
};

static void *
ngx_http_fun_create_loc_conf(ngx_conf_t *cf)
{
    ngx_http_fun_loc_conf_t  *conf;

    conf = ngx_pcalloc(cf->pool, sizeof(ngx_http_fun_loc_conf_t));
    if (conf == NULL) {
        return NGX_CONF_ERROR;
    }
    conf->radius = NGX_CONF_UNSET_UINT;
    return conf;
}

static char *
ngx_http_fun_merge_loc_conf(ngx_conf_t *cf, void *parent, void *child)
{
    ngx_http_fun_loc_conf_t *prev = parent;
    ngx_http_fun_loc_conf_t *conf = child;

    ngx_conf_merge_uint_value(conf->radius, prev->radius, 100);

    if (conf->radius < 1) {
        ngx_conf_log_error(NGX_LOG_EMERG, cf, 0, "radius must be equal or more than 1");
        return NGX_CONF_ERROR;
    }
    if (conf->radius > 1000 ) {
        ngx_conf_log_error(NGX_LOG_EMERG, cf, 0, "radius must be equal or less than 1000");
        return NGX_CONF_ERROR;
    }

    return NGX_CONF_OK;
}



static ngx_http_module_t ngx_http_fun_module_ctx = {
    NULL,                           // preconfiguration
    NULL,                           // postconfiguration
    NULL,                           // create main configuration
    NULL,                           // init main configuration
    NULL,                           // create server configuration
    NULL,                           // merge server configuration
    ngx_http_fun_create_loc_conf,   // create location configuration
    ngx_http_fun_merge_loc_conf     // merge location configuration
};

ngx_module_t ngx_http_fun_module = {
    NGX_MODULE_V1,
    &ngx_http_fun_module_ctx,   // module context
    ngx_http_fun_commands,      // module directives
    NGX_HTTP_MODULE,            // module type
    NULL,                       // init master
    NULL,                       // init module
    NULL,                       // init process
    NULL,                       // init thread
    NULL,                       // exit thread
    NULL,                       // exit process
    NULL,                       // exit master
    NGX_MODULE_V1_PADDING
};

static cairo_status_t
copy_png_to_chain(void *closure, const unsigned char *data, unsigned int length)
{
    // closure is a 'struct closure'
    struct closure *c = closure;
    ngx_buf_t *b = ngx_pcalloc(c->r->pool, sizeof(ngx_buf_t));
    unsigned char *d = ngx_pcalloc(c->r->pool, length);
    ngx_chain_t *ch = c->chain;

    c->length += length;

    if (b == NULL || d == NULL) {
        return CAIRO_STATUS_NO_MEMORY;
    }

    ngx_memcpy(d, data, length);

    b->pos = d;
    b->last = d + length;
    b->memory = 1;
    b->last_buf = 1;

    if ( c->chain->buf == NULL )
    {
        c->chain->buf = b;
        return CAIRO_STATUS_SUCCESS;
    }

    while ( ch->next )
    {
        ch = ch->next;
    }
    ch->next = ngx_pcalloc(c->r->pool, sizeof(ngx_chain_t));
    if ( ch->next == NULL )
    {
        return CAIRO_STATUS_NO_MEMORY;
    }

    ch->next->buf = b;
    ch->next->next = NULL;

    return CAIRO_STATUS_SUCCESS;
}


static ngx_int_t
ngx_http_fun_handler(ngx_http_request_t * r)
{
    ngx_int_t   rc;
    ngx_chain_t out;
    struct closure c = { r, &out, 0 };

    ngx_http_fun_loc_conf_t  *cglcf;
    cglcf = ngx_http_get_module_loc_conf(r, ngx_http_fun_module);


    // we response to 'GET' and 'HEAD' requests only
    if (!(r->method & (NGX_HTTP_GET|NGX_HTTP_HEAD))) {
        return NGX_HTTP_NOT_ALLOWED;
    }

    // discard request body, since we don't need it here
    rc = ngx_http_discard_request_body(r);
    if (rc != NGX_OK) {
        return rc;
    }

    // Let's do the cairo-stuff.
    cairo_surface_t *surface;
    cairo_t *cr;

    surface = cairo_image_surface_create (CAIRO_FORMAT_ARGB32, cglcf->radius*2 + 64, cglcf->radius*2 + 64);
    cr = cairo_create (surface);

    double xc = cglcf->radius + 32;
    double yc = cglcf->radius + 32;
    double radius = cglcf->radius;
    double angle1 = 270.0  * (M_PI/180.0);  /* angles are specified */
    double angle2 = 180.0 * (M_PI/180.0);  /* in radians           */

    cairo_set_line_width (cr, 10.0);
    cairo_arc (cr, xc, yc, radius, angle1, angle2);
    cairo_stroke (cr);

    /* draw helping lines */
    cairo_set_source_rgba (cr, 1, 0.2, 0.2, 0.6);
    cairo_set_line_width (cr, 6.0);

    cairo_arc (cr, xc, yc, 10.0, 0, 2*M_PI);
    cairo_fill (cr);

    cairo_arc (cr, xc, yc, radius, angle1, angle1);
    cairo_line_to (cr, xc, yc);
    cairo_arc (cr, xc, yc, radius, angle2, angle2);
    cairo_line_to (cr, xc, yc);
    cairo_stroke (cr);

    out.buf = NULL;
    out.next = NULL;

    // Copy the png image to our buffer chain (we provide our own callback-function)
    rc = cairo_surface_write_to_png_stream(surface, copy_png_to_chain, &c);

    // Free cairo stuff.
    cairo_destroy(cr);
    cairo_surface_destroy(surface);

    // If we for some reason didn't manage to copy the png to our buffer, throw 503.
    if ( rc != CAIRO_STATUS_SUCCESS )
    {
        return NGX_HTTP_INTERNAL_SERVER_ERROR;
    }

    // set the 'Content-type' header
    r->headers_out.content_type_len = sizeof("image/png") - 1;
    r->headers_out.content_type.len = sizeof("image/png") - 1;
    r->headers_out.content_type.data = (u_char * ) "image/png";
    // set the status line
    r->headers_out.status = NGX_HTTP_OK;
    r->headers_out.content_length_n = c.length;

    // send the headers of your response
    rc = ngx_http_send_header(r);

    // We've added the NGX_HTTP_HEAD check here, because we're unable to set content length before
    // we've actually calculated it (which is done by generating the image).
    // This is a waste of resources, and is why caching solutions exist.
    if (rc == NGX_ERROR || rc > NGX_OK || r->header_only || r->method == NGX_HTTP_HEAD) {
        return rc;
    }

    // send the buffer chain of your response
    return ngx_http_output_filter(r, &out);
}

static char *
ngx_http_fun(ngx_conf_t * cf, ngx_command_t * cmd, void * conf)
{
    ngx_http_core_loc_conf_t * clcf;

    clcf = ngx_http_conf_get_module_loc_conf(cf, ngx_http_core_module);
    clcf->handler = ngx_http_fun_handler; // handler to process the 'fun' directive

    return NGX_CONF_OK;
}

