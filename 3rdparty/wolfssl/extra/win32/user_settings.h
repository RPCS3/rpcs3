#ifndef _WIN_USER_SETTINGS_H_
#define _WIN_USER_SETTINGS_H_

/* Verify this is Windows */
#ifndef _WIN32
#error This user_settings.h header is only designed for Windows
#endif

/* Configurations */
#define WOLFSSL_SYS_CA_CERTS
#define WOLFSSL_DES_ECB
#define HAVE_FFDHE_2048
#define TFM_TIMING_RESISTANT
#define NO_DSA
#define TFM_ECC256
#define NO_RC4
#define NO_HC128
#define NO_RABBIT
#define WOLFSSL_SHA224
#define WOLFSSL_SHA384
#define WOLFSSL_SHA3
#define WOLFSSL_SHA512
#define WOLFSSL_SHAKE256
#define HAVE_POLY1305
#define HAVE_ONE_TIME_AUTH
#define HAVE_CHACHA
#define HAVE_HASHDRBG
#ifndef HAVE_SNI
#define HAVE_SNI
#endif
#define HAVE_ENCRYPT_THEN_MAC
#ifndef NO_MD4
#define NO_MD4
#endif
#define WC_NO_ASYNC_THREADING
#define WC_NO_HARDEN
#define HAVE_WRITE_DUP
#define WC_RSA_BLINDING
#define NO_MULTIBYTE_PRINT
#define OPENSSL_EXTRA
#define WOLFSSL_RIPEMD
#define NO_PSK
#define HAVE_EXTENDED_MASTER
#define WOLFSSL_SNIFFER
#define WOLFSSL_IGNORE_FILE_WARN
#define HAVE_AESGCM
#define HAVE_SUPPORTED_CURVES
#define HAVE_TLS_EXTENSIONS
#define HAVE_ECC
#define ECC_SHAMIR
#define ECC_TIMING_RESISTANT
#define USE_FAST_MATH
#define FP_MAX_BITS 8192
#ifndef WOLFSSL_ALT_CERT_CHAINS
#define WOLFSSL_ALT_CERT_CHAINS
#endif

/* UTF-8 aware filesystem functions for Windows */
#define WOLFSSL_USER_FILESYSTEM

#include <stdio.h>
#define XFILE      FILE*

extern FILE* wolfSSL_fopen_utf8(const char* name, const char* mode);
#define XFOPEN     wolfSSL_fopen_utf8

#define XFDOPEN    fdopen
#define XFSEEK     fseek
#define XFTELL     ftell
#define XREWIND    rewind
#define XFREAD     fread
#define XFWRITE    fwrite
#define XFCLOSE    fclose
#define XSEEK_END  SEEK_END
#define XSEEK_SET  SEEK_SET
#define XBADFILE   NULL
#define XFGETS     fgets
#define XFPRINTF   fprintf
#define XFFLUSH    fflush

/* For some reason we need to define ssize_t */
#define HAVE_SSIZE_T
#define ssize_t __int64

#include <sys/stat.h>
extern int wolfSSL_stat_utf8(const char* path, struct _stat* buffer);
#define XSTAT       wolfSSL_stat_utf8
#define XSTAT_TYPE struct _stat
#define XS_ISREG(s) (s & _S_IFREG)
#define SEPARATOR_CHAR ';'

#endif /* _WIN_USER_SETTINGS_H_ */
