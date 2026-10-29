#include "apr_file_io.h"
#include "apr_mmap.h"
#include "apr_tables.h"
#include "apr_time.h"

#define APR_WANT_STRFUNC
#include "apr_want.h"

#include "httpd.h"
#include "http_protocol.h"
#include "http_request.h"

/* mod_cgidso からは dso_main() が呼び出される */

int dso_main(request_rec *r, int argc, char **argv)
{
    int i, j, status;
    apr_file_t *f;
    apr_finfo_t finfo;
    apr_mmap_t *m;

    /* Not Modified の場合 */
    /* return HTTP_NOT_MODIFIED; */

    /* リダイレクトする場合 */
    /* apr_table_setn(r->headers_out, "Location", "/new-URL/"); */
    /* return HTTP_MOVED_TEMPORARILY; */

    /* Content-Type の設定例 */
    ap_set_content_type(r, "text/plain");

    /* Last-Modified の設定例
     * time_t と apr_time_t はスケールが異なるので注意
     */
    ap_update_mtime(r, r->request_time - apr_time_from_sec(24 * 60 * 60));
    ap_set_last_modified(r);

    /* ETag の設定例 */
    apr_table_setn(r->headers_out, "ETag", "ThisIsETag");

    /* ETag, Last-Modified 等の条件判断はこれが簡単 */
    if ((status = ap_meets_conditions(r)))
        return status;

    /* Content-Encoding の設定例 */
    /* r->content_encoding = "gzip"; */

    /* コンテンツ出力例 */
    ap_rprintf(r, "PATH_INFO: \"%s\"\n", r->path_info);
    ap_rprintf(r, "QUERY_STRING: \"%s\"\n", r->args);

    /* 環境変数の取得はこのように */
    ap_rprintf(r, "SERVER_SIGNATURE: \"%s\"\n",
               apr_table_get(r->subprocess_env, "SERVER_SIGNATURE"));

    ap_rputs("----------------------------------------\n", r);
    for (i = 0; i < argc; i++)
        ap_rprintf(r, "argv[%d] = \"%s\"\n", i, argv[i]);

    if (argc > 1 && !strcmp(argv[1], "dump")
        && !apr_file_open(&f, r->filename, APR_READ, APR_OS_DEFAULT, r->pool)
        && !apr_file_info_get(&finfo, APR_FINFO_MIN, f)
        && !apr_mmap_create(&m, f, 0, finfo.size, APR_MMAP_READ, r->pool)) {

        ap_rputs("----------------------------------------\n", r);
        for (i = 0; i < m->size; ) {
            ap_rprintf(r, "%.4x:", i);
            for (j = 0; i < m->size && j < 16; i++, j++)
                ap_rprintf(r, " %.2x", ((apr_byte_t *)m->mm)[i]);
            ap_rputc('\n', r);
        }
        /* Apache API を使用した場合は，自動的に munmap(), close() 等相当の
         * クリーンアップ処理が行われる．普通の open(), mmap() 等を
         * 使用した場合は，明示的に資源解放を行わなければならない．
         */
        /* apr_mmap_delete(m); */
        /* apr_file_close(f); */
    }

    return OK;
}
