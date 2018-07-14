#include "stdafx.h"

#include <bitset>
#include <string>

#include "Emu/Cell/PPUModule.h"
#include "Utilities/File.h"
#include "Emu/VFS.h"

logs::channel cellSsl("cellSsl");



enum SpecialCerts { BaltimoreCert = 6, Class3G2V2Cert = 13, ClassSSV4Cert = 15, EntrustNetCert = 18, GTECyberTrustGlobalCert = 23 };

s32 cellSslInit(vm::ptr<void> pool, u32 poolSize)
{
	cellSsl.todo("cellSslInit(pool=0x%x, poolSize=%d)", pool, poolSize);
	return CELL_OK;
}

s32 cellSslEnd()
{
	UNIMPLEMENTED_FUNC(cellSsl);
	return CELL_OK;
}

std::string getCert(const std::string certPath, const int certID, const bool isNormalCert)
{
	int newID = certID;

	// The 'normal' certs have some special rules for loading.
	if (isNormalCert && certID >= BaltimoreCert && certID <= GTECyberTrustGlobalCert)
	{
		if (certID == BaltimoreCert)
			newID = GTECyberTrustGlobalCert;
		else if (certID == Class3G2V2Cert)
			newID = BaltimoreCert;
		else if (certID == EntrustNetCert)
			newID = ClassSSV4Cert;
		else
			newID = certID - 1;
	}

	std::string filePath = fmt::format("%sCA%02d.cer", certPath, newID);

	if (!fs::exists(filePath))
	{
		cellSsl.error("Can't find certificate file %s, do you have the PS3 firmware installed?", filePath);
		return "";
	}
	return fs::file(filePath).to_string();
}

s32 cellSslCertificateLoader(u64 flag, vm::ptr<char> buffer, u32 size, vm::ptr<u32> required)
{
	cellSsl.trace("cellSslCertificateLoader(flag=%llu, buffer=0x%x, size=%zu, required=0x%x)", flag, buffer, size, required);

	const std::bitset<58> flagBits(flag);
	const std::string certPath = vfs::get("/dev_flash/data/cert/");

	if (required)
	{
		*required = 0;
		for (int i = 1; i <= flagBits.size(); i++)
		{
			if (!flagBits[i-1])
				continue;
			// If we're loading cert 6 (the baltimore cert), then we need set that we're loading the 'normal' set of certs.
			*required += (u32)(getCert(certPath, i, flagBits[BaltimoreCert-1]).size());
		}
	}
	else
	{
		std::string final;
		for (int i = 1; i <= flagBits.size(); i++)
		{
			if (!flagBits[i-1])
				continue;
			// If we're loading cert 6 (the baltimore cert), then we need set that we're loading the 'normal' set of certs.
			final.append(getCert(certPath, i, flagBits[BaltimoreCert-1]));
		}

		memset(buffer.get_ptr(), '\0', size - 1);
		memcpy(buffer.get_ptr(), final.c_str(), final.size());
	}

	return CELL_OK;
}

s32 cellSslCertGetSerialNumber()
{
	UNIMPLEMENTED_FUNC(cellSsl);
	return CELL_OK;
}

s32 cellSslCertGetPublicKey()
{
	UNIMPLEMENTED_FUNC(cellSsl);
	return CELL_OK;
}

s32 cellSslCertGetRsaPublicKeyModulus()
{
	UNIMPLEMENTED_FUNC(cellSsl);
	return CELL_OK;
}

s32 cellSslCertGetRsaPublicKeyExponent()
{
	UNIMPLEMENTED_FUNC(cellSsl);
	return CELL_OK;
}

s32 cellSslCertGetNotBefore()
{
	UNIMPLEMENTED_FUNC(cellSsl);
	return CELL_OK;
}

s32 cellSslCertGetNotAfter()
{
	UNIMPLEMENTED_FUNC(cellSsl);
	return CELL_OK;
}

s32 cellSslCertGetSubjectName()
{
	UNIMPLEMENTED_FUNC(cellSsl);
	return CELL_OK;
}

s32 cellSslCertGetIssuerName()
{
	UNIMPLEMENTED_FUNC(cellSsl);
	return CELL_OK;
}

s32 cellSslCertGetNameEntryCount()
{
	UNIMPLEMENTED_FUNC(cellSsl);
	return CELL_OK;
}

s32 cellSslCertGetNameEntryInfo()
{
	UNIMPLEMENTED_FUNC(cellSsl);
	return CELL_OK;
}

s32 cellSslCertGetMd5Fingerprint()
{
	UNIMPLEMENTED_FUNC(cellSsl);
	return CELL_OK;
}

s32 _cellSslConvertCipherId()
{
	UNIMPLEMENTED_FUNC(cellSsl);
	return CELL_OK;
}

s32 _cellSslConvertSslVersion()
{
	UNIMPLEMENTED_FUNC(cellSsl);
	return CELL_OK;
}

s32 _cellSslIsInitd()
{
	UNIMPLEMENTED_FUNC(cellSsl);
	return CELL_OK;
}

s32 _cellSslPemReadPrivateKey()
{
	UNIMPLEMENTED_FUNC(cellSsl);
	return CELL_OK;
}

s32 _cellSslPemReadX509()
{
	UNIMPLEMENTED_FUNC(cellSsl);
	return CELL_OK;
}

s32 BER_read_item()
{
	UNIMPLEMENTED_FUNC(cellSsl);
	return CELL_OK;
}

s32 BIO_ctrl()
{
	UNIMPLEMENTED_FUNC(cellSsl);
	return CELL_OK;
}

s32 BIO_dump()
{
	UNIMPLEMENTED_FUNC(cellSsl);
	return CELL_OK;
}

s32 BIO_free()
{
	UNIMPLEMENTED_FUNC(cellSsl);
	return CELL_OK;
}

s32 BIO_get_cb_arg()
{
	UNIMPLEMENTED_FUNC(cellSsl);
	return CELL_OK;
}

s32 BIO_get_retry_reason()
{
	UNIMPLEMENTED_FUNC(cellSsl);
	return CELL_OK;
}

s32 BIO_new_mem()
{
	UNIMPLEMENTED_FUNC(cellSsl);
	return CELL_OK;
}

s32 BIO_new_socket()
{
	UNIMPLEMENTED_FUNC(cellSsl);
	return CELL_OK;
}

s32 BIO_printf()
{
	UNIMPLEMENTED_FUNC(cellSsl);
	return CELL_OK;
}

s32 BIO_ptr_ctrl()
{
	UNIMPLEMENTED_FUNC(cellSsl);
	return CELL_OK;
}

s32 BIO_set_cb_arg()
{
	UNIMPLEMENTED_FUNC(cellSsl);
	return CELL_OK;
}

s32 ERR_clear_error()
{
	UNIMPLEMENTED_FUNC(cellSsl);
	return CELL_OK;
}

s32 ERR_get_error()
{
	UNIMPLEMENTED_FUNC(cellSsl);
	return CELL_OK;
}

s32 ERR_error_string()
{
	UNIMPLEMENTED_FUNC(cellSsl);
	return CELL_OK;
}

s32 ERR_func_error_string()
{
	UNIMPLEMENTED_FUNC(cellSsl);
	return CELL_OK;
}

s32 ERR_peek_error()
{
	UNIMPLEMENTED_FUNC(cellSsl);
	return CELL_OK;
}

s32 EVP_PKEY_free()
{
	UNIMPLEMENTED_FUNC(cellSsl);
	return CELL_OK;
}

s32 R_time()
{
	UNIMPLEMENTED_FUNC(cellSsl);
	return CELL_OK;
}

s32 R_time_cmp()
{
	UNIMPLEMENTED_FUNC(cellSsl);
	return CELL_OK;
}

s32 R_time_export()
{
	UNIMPLEMENTED_FUNC(cellSsl);
	return CELL_OK;
}

s32 R_time_free()
{
	UNIMPLEMENTED_FUNC(cellSsl);
	return CELL_OK;
}

s32 R_time_import()
{
	UNIMPLEMENTED_FUNC(cellSsl);
	return CELL_OK;
}

s32 R_time_new()
{
	UNIMPLEMENTED_FUNC(cellSsl);
	return CELL_OK;
}

s32 SSL_CIPHER_description()
{
	UNIMPLEMENTED_FUNC(cellSsl);
	return CELL_OK;
}

s32 SSL_CIPHER_get_bits()
{
	UNIMPLEMENTED_FUNC(cellSsl);
	return CELL_OK;
}

s32 SSL_CIPHER_get_id()
{
	UNIMPLEMENTED_FUNC(cellSsl);
	return CELL_OK;
}

s32 SSL_CIPHER_get_name()
{
	UNIMPLEMENTED_FUNC(cellSsl);
	return CELL_OK;
}

s32 SSL_CIPHER_get_version()
{
	UNIMPLEMENTED_FUNC(cellSsl);
	return CELL_OK;
}

s32 SSL_CTX_ctrl()
{
	UNIMPLEMENTED_FUNC(cellSsl);
	return CELL_OK;
}

s32 SSL_CTX_free()
{
	UNIMPLEMENTED_FUNC(cellSsl);
	return CELL_OK;
}

s32 SSL_CTX_new()
{
	UNIMPLEMENTED_FUNC(cellSsl);
	return CELL_OK;
}

s32 SSL_CTX_set_app_verify_cb()
{
	UNIMPLEMENTED_FUNC(cellSsl);
	return CELL_OK;
}

s32 SSL_CTX_set_info_cb()
{
	UNIMPLEMENTED_FUNC(cellSsl);
	return CELL_OK;
}

s32 SSL_CTX_set_options()
{
	UNIMPLEMENTED_FUNC(cellSsl);
	return CELL_OK;
}

s32 SSL_CTX_set_verify_mode()
{
	UNIMPLEMENTED_FUNC(cellSsl);
	return CELL_OK;
}

s32 SSL_CTX_use_certificate()
{
	UNIMPLEMENTED_FUNC(cellSsl);
	return CELL_OK;
}

s32 SSL_CTX_use_PrivateKey()
{
	UNIMPLEMENTED_FUNC(cellSsl);
	return CELL_OK;
}

s32 SSL_SESSION_free()
{
	UNIMPLEMENTED_FUNC(cellSsl);
	return CELL_OK;
}

s32 SSL_alert_desc_string_long()
{
	UNIMPLEMENTED_FUNC(cellSsl);
	return CELL_OK;
}

s32 SSL_alert_type_string_long()
{
	UNIMPLEMENTED_FUNC(cellSsl);
	return CELL_OK;
}

s32 SSL_clear()
{
	UNIMPLEMENTED_FUNC(cellSsl);
	return CELL_OK;
}

s32	SSL_do_handshake()
{
	UNIMPLEMENTED_FUNC(cellSsl);
	return CELL_OK;
}

s32 SSL_free()
{
	UNIMPLEMENTED_FUNC(cellSsl);
	return CELL_OK;
}

s32 SSL_get_current_cipher()
{
	UNIMPLEMENTED_FUNC(cellSsl);
	return CELL_OK;
}

s32 SSL_get_error()
{
	UNIMPLEMENTED_FUNC(cellSsl);
	return CELL_OK;
}

s32 SSL_get_rbio()
{
	UNIMPLEMENTED_FUNC(cellSsl);
	return CELL_OK;
}

s32 SSL_get_version()
{
	UNIMPLEMENTED_FUNC(cellSsl);
	return CELL_OK;
}

s32 SSL_new()
{
	UNIMPLEMENTED_FUNC(cellSsl);
	return CELL_OK;
}

s32 SSL_peek()
{
	UNIMPLEMENTED_FUNC(cellSsl);
	return CELL_OK;
}

s32 SSL_read()
{
	UNIMPLEMENTED_FUNC(cellSsl);
	return CELL_OK;
}

s32 SSL_set_bio()
{
	UNIMPLEMENTED_FUNC(cellSsl);
	return CELL_OK;
}

s32 SSL_set_connect_state()
{
	UNIMPLEMENTED_FUNC(cellSsl);
	return CELL_OK;
}

s32 SSL_set_session()
{
	UNIMPLEMENTED_FUNC(cellSsl);
	return CELL_OK;
}

s32 SSL_set_ssl_method()
{
	UNIMPLEMENTED_FUNC(cellSsl);
	return CELL_OK;
}

s32 SSL_shutdown()
{
	UNIMPLEMENTED_FUNC(cellSsl);
	return CELL_OK;
}

s32 SSL_state()
{
	UNIMPLEMENTED_FUNC(cellSsl);
	return CELL_OK;
}

s32 SSL_state_string_long()
{
	UNIMPLEMENTED_FUNC(cellSsl);
	return CELL_OK;
}

s32 SSL_version()
{
	UNIMPLEMENTED_FUNC(cellSsl);
	return CELL_OK;
}

s32 SSL_want()
{
	UNIMPLEMENTED_FUNC(cellSsl);
	return CELL_OK;
}

s32 SSL_write()
{
	UNIMPLEMENTED_FUNC(cellSsl);
	return CELL_OK;
}

s32 SSLCERT_check_private_key()
{
	UNIMPLEMENTED_FUNC(cellSsl);
	return CELL_OK;
}

s32 SSLCERT_free()
{
	UNIMPLEMENTED_FUNC(cellSsl);
	return CELL_OK;
}

s32 SSLCERT_from_binary()
{
	UNIMPLEMENTED_FUNC(cellSsl);
	return CELL_OK;
}

s32 SSLCERT_get_basic_constraints_int()
{
	UNIMPLEMENTED_FUNC(cellSsl);
	return CELL_OK;
}

s32 SSLCERT_get_extension()
{
	UNIMPLEMENTED_FUNC(cellSsl);
	return CELL_OK;
}

s32 SSLCERT_get_issuer_name()
{
	UNIMPLEMENTED_FUNC(cellSsl);
	return CELL_OK;
}

s32 SSLCERT_get_notAfter()
{
	UNIMPLEMENTED_FUNC(cellSsl);
	return CELL_OK;
}

s32 SSLCERT_get_notBefore()
{
	UNIMPLEMENTED_FUNC(cellSsl);
	return CELL_OK;
}

s32 SSLCERT_get_pubkey()
{
	UNIMPLEMENTED_FUNC(cellSsl);
	return CELL_OK;
}

s32 SSLCERT_get_subject_name()
{
	UNIMPLEMENTED_FUNC(cellSsl);
	return CELL_OK;
}

s32 SSLCERT_NAME_cmp()
{
	UNIMPLEMENTED_FUNC(cellSsl);
	return CELL_OK;
}

s32 SSLCERT_NAME_ENTRY_get_info()
{
	UNIMPLEMENTED_FUNC(cellSsl);
	return CELL_OK;
}

s32 SSLCERT_NAME_get_entry()
{
	UNIMPLEMENTED_FUNC(cellSsl);
	return CELL_OK;
}

s32 SSLCERT_NAME_get_entry_count()
{
	UNIMPLEMENTED_FUNC(cellSsl);
	return CELL_OK;
}

s32 SSLCERT_NAME_oneline()
{
	UNIMPLEMENTED_FUNC(cellSsl);
	return CELL_OK;
}

s32 SSLCERT_OID_to_string()
{
	UNIMPLEMENTED_FUNC(cellSsl);
	return CELL_OK;
}

s32 SSLCERT_verify()
{
	UNIMPLEMENTED_FUNC(cellSsl);
	return CELL_OK;
}

s32 SSLv3_client_method()
{
	UNIMPLEMENTED_FUNC(cellSsl);
	return CELL_OK;
}

s32 TLSv1_client_method()
{
	UNIMPLEMENTED_FUNC(cellSsl);
	return CELL_OK;
}

DECLARE(ppu_module_manager::cellSsl)("cellSsl", []()
{
	REG_FUNC(cellSsl, cellSslInit);
	REG_FUNC(cellSsl, cellSslEnd);

	REG_FUNC(cellSsl, cellSslCertificateLoader);

	REG_FUNC(cellSsl, cellSslCertGetSerialNumber);
	REG_FUNC(cellSsl, cellSslCertGetPublicKey);
	REG_FUNC(cellSsl, cellSslCertGetRsaPublicKeyModulus);
	REG_FUNC(cellSsl, cellSslCertGetRsaPublicKeyExponent);
	REG_FUNC(cellSsl, cellSslCertGetNotBefore);
	REG_FUNC(cellSsl, cellSslCertGetNotAfter);
	REG_FUNC(cellSsl, cellSslCertGetSubjectName);
	REG_FUNC(cellSsl, cellSslCertGetIssuerName);
	REG_FUNC(cellSsl, cellSslCertGetNameEntryCount);
	REG_FUNC(cellSsl, cellSslCertGetNameEntryInfo);
	REG_FUNC(cellSsl, cellSslCertGetMd5Fingerprint);

	REG_FUNC(cellSsl, _cellSslConvertCipherId);
	REG_FUNC(cellSsl, _cellSslConvertSslVersion);
	REG_FUNC(cellSsl, _cellSslIsInitd);
	REG_FUNC(cellSsl, _cellSslPemReadPrivateKey);
	REG_FUNC(cellSsl, _cellSslPemReadX509);

	REG_FUNC(cellSsl, BER_read_item);

	REG_FUNC(cellSsl, BIO_ctrl);
	REG_FUNC(cellSsl, BIO_dump);
	REG_FUNC(cellSsl, BIO_free);
	REG_FUNC(cellSsl, BIO_get_cb_arg);
	REG_FUNC(cellSsl, BIO_get_retry_reason);
	REG_FUNC(cellSsl, BIO_new_mem);
	REG_FUNC(cellSsl, BIO_new_socket);
	REG_FUNC(cellSsl, BIO_printf);
	REG_FUNC(cellSsl, BIO_ptr_ctrl);
	REG_FUNC(cellSsl, BIO_set_cb_arg);

	REG_FUNC(cellSsl, ERR_clear_error);
	REG_FUNC(cellSsl, ERR_get_error);
	REG_FUNC(cellSsl, ERR_error_string);
	REG_FUNC(cellSsl, ERR_func_error_string);
	REG_FUNC(cellSsl, ERR_peek_error);

	REG_FUNC(cellSsl, EVP_PKEY_free);

	REG_FUNC(cellSsl, R_time);
	REG_FUNC(cellSsl, R_time_cmp);
	REG_FUNC(cellSsl, R_time_export);
	REG_FUNC(cellSsl, R_time_free);
	REG_FUNC(cellSsl, R_time_import);
	REG_FUNC(cellSsl, R_time_new);

	REG_FUNC(cellSsl, SSL_CIPHER_description);
	REG_FUNC(cellSsl, SSL_CIPHER_get_bits);
	REG_FUNC(cellSsl, SSL_CIPHER_get_id);
	REG_FUNC(cellSsl, SSL_CIPHER_get_name);
	REG_FUNC(cellSsl, SSL_CIPHER_get_version);

	REG_FUNC(cellSsl, SSL_CTX_ctrl);
	REG_FUNC(cellSsl, SSL_CTX_free);
	REG_FUNC(cellSsl, SSL_CTX_new);
	REG_FUNC(cellSsl, SSL_CTX_set_app_verify_cb);
	REG_FUNC(cellSsl, SSL_CTX_set_info_cb);
	REG_FUNC(cellSsl, SSL_CTX_set_options);
	REG_FUNC(cellSsl, SSL_CTX_set_verify_mode);
	REG_FUNC(cellSsl, SSL_CTX_use_certificate);
	REG_FUNC(cellSsl, SSL_CTX_use_PrivateKey);

	REG_FUNC(cellSsl, SSL_SESSION_free);

	REG_FUNC(cellSsl, SSL_alert_desc_string_long);
	REG_FUNC(cellSsl, SSL_alert_type_string_long);
	REG_FUNC(cellSsl, SSL_clear);
	REG_FUNC(cellSsl, SSL_do_handshake);
	REG_FUNC(cellSsl, SSL_free);
	REG_FUNC(cellSsl, SSL_get_current_cipher);
	REG_FUNC(cellSsl, SSL_get_error);
	REG_FUNC(cellSsl, SSL_get_rbio);
	REG_FUNC(cellSsl, SSL_get_version);
	REG_FUNC(cellSsl, SSL_new);
	REG_FUNC(cellSsl, SSL_peek);
	REG_FUNC(cellSsl, SSL_read);
	REG_FUNC(cellSsl, SSL_set_bio);
	REG_FUNC(cellSsl, SSL_set_connect_state);
	REG_FUNC(cellSsl, SSL_set_session);
	REG_FUNC(cellSsl, SSL_set_ssl_method);
	REG_FUNC(cellSsl, SSL_shutdown);
	REG_FUNC(cellSsl, SSL_state);
	REG_FUNC(cellSsl, SSL_state_string_long);
	REG_FUNC(cellSsl, SSL_version);
	REG_FUNC(cellSsl, SSL_want);
	REG_FUNC(cellSsl, SSL_write);

	REG_FUNC(cellSsl, SSLCERT_free);
	REG_FUNC(cellSsl, SSLCERT_from_binary);

	REG_FUNC(cellSsl, SSLCERT_check_private_key);
	REG_FUNC(cellSsl, SSLCERT_get_basic_constraints_int);
	REG_FUNC(cellSsl, SSLCERT_get_extension);
	REG_FUNC(cellSsl, SSLCERT_get_issuer_name);
	REG_FUNC(cellSsl, SSLCERT_get_notAfter);
	REG_FUNC(cellSsl, SSLCERT_get_notBefore);
	REG_FUNC(cellSsl, SSLCERT_get_pubkey);
	REG_FUNC(cellSsl, SSLCERT_get_subject_name);

	REG_FUNC(cellSsl, SSLCERT_NAME_cmp);
	REG_FUNC(cellSsl, SSLCERT_NAME_ENTRY_get_info);
	REG_FUNC(cellSsl, SSLCERT_NAME_get_entry);
	REG_FUNC(cellSsl, SSLCERT_NAME_get_entry_count);
	REG_FUNC(cellSsl, SSLCERT_NAME_oneline);

	REG_FUNC(cellSsl, SSLCERT_OID_to_string);

	REG_FUNC(cellSsl, SSLCERT_verify);

	REG_FUNC(cellSsl, SSLv3_client_method);

	REG_FUNC(cellSsl, TLSv1_client_method);
});
