/* Copyright 1999-2005 The Apache Software Foundation or its licensors, as
 * applicable.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/* This product includes software developed by
 * The Apache Software Foundation (http://www.apache.org/).
 *
 * Portions of this software were developed at the National Center
 * for Supercomputing Applications (NCSA) at the University of
 * Illinois at Urbana-Champaign.
 */

/*
 * http_script: keeps all script-related ramblings together.
 *
 * Compliant to cgi/1.1 spec
 *
 * Adapted by rst from original NCSA code by Rob McCool
 *
 * Apache adds some new env vars; REDIRECT_URL and REDIRECT_QUERY_STRING for
 * custom error responses, and DOCUMENT_ROOT because we found it useful.
 * It also adds SERVER_ADMIN - useful for scripts to know who to mail when
 * they fail.
 */

#include "apr_dso.h"
#include "apr_pools.h"
#include "apr_strings.h"
#include "apr_tables.h"

#define APR_WANT_STRFUNC
#include "apr_want.h"

#include "httpd.h"
#include "http_config.h"
#include "http_core.h"
#include "http_log.h"
#include "util_script.h"

module AP_MODULE_DECLARE_DATA cgidso_module;

#define HANDLER_NAME "dso-script"

/* KLUDGE --- for back-combatibility, we don't have to check ExecCGI
 * in ScriptAliased directories, which means we need to know if this
 * request came through ScriptAlias or not... so the Alias module
 * leaves a note for us.
 */

static int is_scriptaliased(request_rec *r)
{
    const char *t = apr_table_get(r->notes, "alias-forced-type");
    return t && !strcasecmp(t, HANDLER_NAME);
}

/* This routine is called to create the argument list to be passed
 * to the CGI script.  The query info is split into separate arguments,
 * where "+" is the separator between keyword arguments.
 *
 * Do not process the args if they containing an '=' assignment.
 */
static char **create_argv(apr_pool_t *p, int *argc,
                          const char *argv0, const char *args)
{
    int x, numwords;
    char **av;
    char *w;
    int idx = 0;

    if (!args || ap_strchr_c(args, '='))
        numwords = 0;
    else {
        /* count the number of keywords */
        for (x = 0, numwords = 1; args[x]; x++)
            if (args[x] == '+')
                ++numwords;
    }

    if (numwords > APACHE_ARG_MAX - 2)
        numwords = APACHE_ARG_MAX - 2;  /* Truncate args to prevent overrun */

    av = (char **) apr_palloc(p, (numwords + 2) * sizeof(char *));

    av[idx++] = apr_pstrdup(p, argv0);
    for (x = 1; x <= numwords; x++) {
        w = ap_getword_nulls(p, &args, '+');
        if (*w) {
            ap_unescape_url(w);
            av[idx++] = ap_escape_shell_cmd(p, w);
        }
    }
    av[idx] = NULL;
    if (argc)
        *argc = idx;
    return av;
}


static int cgidso_init(apr_pool_t *p, apr_pool_t *plog, apr_pool_t *ptemp,
                       server_rec *main_server)
{
    void *data;
    static const char userdata_key[] = "cgidso_init";

    apr_pool_userdata_get(&data, userdata_key, main_server->process->pool);

    if (!data)
        apr_pool_userdata_set((const void *)1, userdata_key,
                            apr_pool_cleanup_null, main_server->process->pool);
    else
        ap_log_error(APLOG_MARK, APLOG_NOTICE, 0, main_server,
                     "mod_cgidso enabled");

    return OK;
}

static int cgidso_handler(request_rec *r)
{
    int argc;
    char *argv0, **argv;
    apr_dso_handle_t *dso_handle;
    union {
        int (*f)(request_rec *, int, char **);
        apr_dso_handle_sym_t d;
    } dso_main;

    if (strcmp(r->handler, HANDLER_NAME))
        return DECLINED;

    if (r->method_number == M_OPTIONS) {
        /* 99 out of 100 cgidso scripts, this is all they support */
        r->allowed |=  (AP_METHOD_BIT << M_GET)
                     | (AP_METHOD_BIT << M_POST);
        return DECLINED;
    }

    if (!(ap_allow_options(r) & OPT_EXECCGI) && !is_scriptaliased(r)) {
        ap_log_rerror(APLOG_MARK, APLOG_ERR, 0, r,
                      "%s: Options ExecCGI is off in this directory", r->filename);
        return HTTP_FORBIDDEN;
    }

    if (r->finfo.filetype == 0) {
        ap_log_rerror(APLOG_MARK, APLOG_ERR, 0, r,
                      "%s: script not found or unable to stat", r->filename);
        return HTTP_NOT_FOUND;
    }
    if (r->finfo.filetype == APR_DIR) {
        ap_log_rerror(APLOG_MARK, APLOG_ERR, 0, r,
                      "%s: attempt to invoke directory as script", r->filename);
        return HTTP_FORBIDDEN;
    }

    if ((r->used_path_info == AP_REQ_REJECT_PATH_INFO) &&
        r->path_info && *r->path_info) {
        /* default to accept */
        ap_log_rerror(APLOG_MARK, APLOG_ERR, 0, r,
                      "%s: AcceptPathInfo off disallows user's path", r->filename);
        return HTTP_NOT_FOUND;
    }

    /* DSO will be unloaded automatically when r->pool is destroyed */
    if (apr_dso_load(&dso_handle, r->filename, r->pool)
        || apr_dso_sym(&dso_main.d, dso_handle, "dso_main")) {
        ap_log_rerror(APLOG_MARK, APLOG_ERR, 0, r,
                      "%s: %s", r->filename, apr_dso_error(dso_handle, NULL, 0));
        return HTTP_INTERNAL_SERVER_ERROR;
    }

    if ((argv0 = strrchr(r->filename, '/')) != NULL)
        argv0++;
    else
        argv0 = r->filename;
    argv = create_argv(r->pool, &argc, argv0, r->args);
    ap_add_common_vars(r);
    ap_add_cgi_vars(r);

    return dso_main.f(r, argc, argv);
}


static void register_hooks(apr_pool_t *p)
{
    ap_hook_post_config(cgidso_init, NULL, NULL, APR_HOOK_MIDDLE);
    ap_hook_handler(cgidso_handler, NULL, NULL, APR_HOOK_MIDDLE);
}

module AP_MODULE_DECLARE_DATA cgidso_module = {
    STANDARD20_MODULE_STUFF,
    NULL, /* dir config creater */
    NULL, /* dir merger --- default is to override */
    NULL, /* server config */
    NULL, /* merge server config */
    NULL, /* command table */
    register_hooks /* register_handlers */
};
