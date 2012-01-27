#include <ngx_config.h>
#include <ngx_core.h>
#include <ngx_http.h>

static char * ngx_http_fun(ngx_conf_t * cf, ngx_command_t * cmd, void * conf);

static u_char ngx_fun_string[] = "This is fun!";

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
    ngx_null_command
};

static ngx_http_module_t ngx_http_fun_module_ctx = {
    NULL,   // preconfiguration
    NULL,   // postconfiguration
    NULL,   // create main configuration
    NULL,   // init main configuration
    NULL,   // create server configuration
    NULL,   // merge server configuration
    NULL,   // create location configuration
    NULL    // merge location configuration
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

static ngx_int_t
ngx_http_fun_handler(ngx_http_request_t * r)
{
    ngx_int_t   rc;
    ngx_buf_t   * b;
    ngx_chain_t out;

    // we response to 'GET' and 'HEAD' requests only
    if (!(r->method & (NGX_HTTP_GET|NGX_HTTP_HEAD))) {
        return NGX_HTTP_NOT_ALLOWED;
    }

    // discard request body, since we don't need it here
    rc = ngx_http_discard_request_body(r);
    if (rc != NGX_OK) {
        return rc;
    }

    // set the 'Content-type' header
    r->headers_out.content_type_len = sizeof("text/html") - 1;
    r->headers_out.content_type.len = sizeof("text/html") - 1;
    r->headers_out.content_type.data = (u_char * ) "text/html";

    // send the header only, if the request type is http 'HEAD'
    if (r->method == NGX_HTTP_HEAD) {
        r->headers_out.status = NGX_HTTP_OK;
        r->headers_out.content_length_n = sizeof(ngx_fun_string) - 1;
        return ngx_http_send_header(r);
    }

    // allocate a buffer for your response body
    b = ngx_pcalloc(r->pool, sizeof(ngx_buf_t));
    if (b == NULL) {
        return NGX_HTTP_INTERNAL_SERVER_ERROR;
    }

    // attach this buffer to the buffer chain
    out.buf = b;
    out.next = NULL;

    // adjust the pointers of the buffer
    b->pos = ngx_fun_string;
    b->last = ngx_fun_string + sizeof(ngx_fun_string) - 1;
    b->memory = 1; // This buffer is in read-only memory
                   // this means that filters should copy it, and not try to rewrite in place.
    b->last_buf = 1; // this is the last buffer in the buffer chain

    // set the status line
    r->headers_out.status = NGX_HTTP_OK;
    r->headers_out.content_length_n = sizeof(ngx_fun_string) - 1;

    // send the headers of your response
    rc = ngx_http_send_header(r);
    if (rc == NGX_ERROR || rc > NGX_OK || r->header_only) {
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

