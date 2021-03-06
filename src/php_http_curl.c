/*
    +--------------------------------------------------------------------+
    | PECL :: http                                                       |
    +--------------------------------------------------------------------+
    | Redistribution and use in source and binary forms, with or without |
    | modification, are permitted provided that the conditions mentioned |
    | in the accompanying LICENSE file are met.                          |
    +--------------------------------------------------------------------+
    | Copyright (c) 2004-2014, Michael Wallner <mike@php.net>            |
    +--------------------------------------------------------------------+
*/

#include "php_http_api.h"

#if PHP_HTTP_HAVE_LIBCURL

#if ZTS && PHP_HTTP_HAVE_LIBCURL_SSL
#	include "TSRM.h"
#	if PHP_WIN32
#		define PHP_HTTP_NEED_OPENSSL_TSL 1
#		include <openssl/crypto.h>
#	else /* !PHP_WIN32 */
#		if PHP_HTTP_HAVE_LIBCURL_OPENSSL
#			define PHP_HTTP_NEED_OPENSSL_TSL 1
#			include <openssl/crypto.h>
#		elif PHP_HTTP_HAVE_LIBCURL_GNUTLS
#			define PHP_HTTP_NEED_GNUTLS_TSL 1
#			include <gcrypt.h>
#		endif /* PHP_HTTP_HAVE_LIBCURL_OPENSSL || PHP_HTTP_HAVE_LIBCURL_GNUTLS */
#	endif /* PHP_WIN32 */
#endif /* ZTS && PHP_HTTP_HAVE_LIBCURL_SSL */


#if PHP_HTTP_NEED_OPENSSL_TSL
static MUTEX_T *php_http_openssl_tsl = NULL;

static void php_http_openssl_thread_lock(int mode, int n, const char * file, int line)
{
	if (mode & CRYPTO_LOCK) {
		tsrm_mutex_lock(php_http_openssl_tsl[n]);
	} else {
		tsrm_mutex_unlock(php_http_openssl_tsl[n]);
	}
}

static ulong php_http_openssl_thread_id(void)
{
	return (ulong) tsrm_thread_id();
}
#endif
#if PHP_HTTP_NEED_GNUTLS_TSL
static int php_http_gnutls_mutex_create(void **m)
{
	if (*((MUTEX_T *) m) = tsrm_mutex_alloc()) {
		return SUCCESS;
	} else {
		return FAILURE;
	}
}

static int php_http_gnutls_mutex_destroy(void **m)
{
	tsrm_mutex_free(*((MUTEX_T *) m));
	return SUCCESS;
}

static int php_http_gnutls_mutex_lock(void **m)
{
	return tsrm_mutex_lock(*((MUTEX_T *) m));
}

static int php_http_gnutls_mutex_unlock(void **m)
{
	return tsrm_mutex_unlock(*((MUTEX_T *) m));
}

static struct gcry_thread_cbs php_http_gnutls_tsl = {
	GCRY_THREAD_OPTION_USER,
	NULL,
	php_http_gnutls_mutex_create,
	php_http_gnutls_mutex_destroy,
	php_http_gnutls_mutex_lock,
	php_http_gnutls_mutex_unlock
};
#endif


PHP_MINIT_FUNCTION(http_curl)
{
#if PHP_HTTP_NEED_OPENSSL_TSL
	/* mod_ssl, libpq or ext/curl might already have set thread lock callbacks */
	if (!CRYPTO_get_id_callback()) {
		int i, c = CRYPTO_num_locks();

		php_http_openssl_tsl = malloc(c * sizeof(MUTEX_T));

		for (i = 0; i < c; ++i) {
			php_http_openssl_tsl[i] = tsrm_mutex_alloc();
		}

		CRYPTO_set_id_callback(php_http_openssl_thread_id);
		CRYPTO_set_locking_callback(php_http_openssl_thread_lock);
	}
#endif
#if PHP_HTTP_NEED_GNUTLS_TSL
	gcry_control(GCRYCTL_SET_THREAD_CBS, &php_http_gnutls_tsl);
#endif

	if (CURLE_OK != curl_global_init(CURL_GLOBAL_ALL)) {
		return FAILURE;
	}

	return SUCCESS;
}

PHP_MSHUTDOWN_FUNCTION(http_curl)
{
	curl_global_cleanup();
#if PHP_HTTP_NEED_OPENSSL_TSL
	if (php_http_openssl_tsl) {
		int i, c = CRYPTO_num_locks();

		CRYPTO_set_id_callback(NULL);
		CRYPTO_set_locking_callback(NULL);

		for (i = 0; i < c; ++i) {
			tsrm_mutex_free(php_http_openssl_tsl[i]);
		}

		free(php_http_openssl_tsl);
		php_http_openssl_tsl = NULL;
	}
#endif
	return SUCCESS;
}

#endif /* PHP_HTTP_HAVE_LIBCURL */

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: noet sw=4 ts=4 fdm=marker
 * vim<600: noet sw=4 ts=4
 */

