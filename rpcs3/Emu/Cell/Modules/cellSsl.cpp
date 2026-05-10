#include "stdafx.h"

#include <bitset>
#include <string>

#include "cellSsl.h"
#include "Emu/Cell/PPUModule.h"
#include "Utilities/File.h"
#include "Emu/VFS.h"
#include "Emu/IdManager.h"

#include "cellRtc.h"

LOG_CHANNEL(cellSsl);

template<>
void fmt_class_string<CellSslError>::format(std::string& out, u64 arg)
{
	format_enum(out, arg, [](auto error)
	{
		switch (error)
		{
			STR_CASE(CELL_SSL_ERROR_NOT_INITIALIZED);
			STR_CASE(CELL_SSL_ERROR_ALREADY_INITIALIZED);
			STR_CASE(CELL_SSL_ERROR_INITIALIZATION_FAILED);
			STR_CASE(CELL_SSL_ERROR_NO_BUFFER);
			STR_CASE(CELL_SSL_ERROR_INVALID_CERTIFICATE);
			STR_CASE(CELL_SSL_ERROR_UNRETRIEVABLE);
			STR_CASE(CELL_SSL_ERROR_INVALID_FORMAT);
			STR_CASE(CELL_SSL_ERROR_NOT_FOUND);
			STR_CASE(CELL_SSL_ERROR_INVALID_TIME);
			STR_CASE(CELL_SSL_ERROR_INAVLID_NEGATIVE_TIME);
			STR_CASE(CELL_SSL_ERROR_INCORRECT_TIME);
			STR_CASE(CELL_SSL_ERROR_UNDEFINED_TIME_TYPE);
			STR_CASE(CELL_SSL_ERROR_NO_MEMORY);
			STR_CASE(CELL_SSL_ERROR_NO_STRING);
			STR_CASE(CELL_SSL_ERROR_UNKNOWN_LOAD_CERT);
		}

		return unknown;
	});
}

error_code cellSslInit(vm::ptr<void> pool, u32 poolSize)
{
	cellSsl.todo("cellSslInit(pool=*0x%x, poolSize=%d)", pool, poolSize);

	auto& manager = g_fxo->get<ssl_manager>();

	if (manager.is_init)
		return CELL_SSL_ERROR_ALREADY_INITIALIZED;

	manager.is_init = true;

	return CELL_OK;
}

error_code cellSslEnd()
{
	cellSsl.todo("cellSslEnd()");

	auto& manager = g_fxo->get<ssl_manager>();

	if (!manager.is_init)
		return CELL_SSL_ERROR_NOT_INITIALIZED;

	manager.is_init = false;

	return CELL_OK;
}

error_code cellSslGetMemoryInfo()
{
	UNIMPLEMENTED_FUNC(cellSsl);
	return CELL_OK;
}

std::string getCert(const std::string& certPath, const int certID, const bool isNormalCert)
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

error_code cellSslCertificateLoader(u64 flag, vm::ptr<char> buffer, u32 size, vm::ptr<u32> required)
{
	cellSsl.trace("cellSslCertificateLoader(flag=%llu, buffer=*0x%x, size=%zu, required=*0x%x)", flag, buffer, size, required);

	const std::bitset<58> flagBits(flag);
	const std::string certPath = vfs::get("/dev_flash/data/cert/");

	if (required)
	{
		*required = 0;
		for (uint i = 1; i <= flagBits.size(); i++)
		{
			if (!flagBits[i-1])
				continue;
			// If we're loading cert 6 (the baltimore cert), then we need set that we're loading the 'normal' set of certs.
			*required += ::size32(getCert(certPath, i, flagBits[BaltimoreCert-1]));
		}
	}
	else
	{
		std::string final;
		for (uint i = 1; i <= flagBits.size(); i++)
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

error_code cellSslCertGetSerialNumber(vm::cptr<void> sslCert, vm::cpptr<u8> sboData, vm::ptr<u64> sboLength)
{
	cellSsl.todo("cellSslCertGetSerialNumber(sslCert=*0x%x, sboData=**0x%x, sboLength=*0x%x)", sslCert, sboData, sboLength);

	if (!g_fxo->get<ssl_manager>().is_init)
		return CELL_SSL_ERROR_NOT_INITIALIZED;

	if (!sslCert)
		return CELL_SSL_ERROR_INVALID_CERTIFICATE;

	if (!sboData || !sboLength)
		return CELL_SSL_ERROR_NO_BUFFER;

	return CELL_OK;
}

error_code cellSslCertGetPublicKey(vm::cptr<void> sslCert, vm::cpptr<u8> sboData, vm::ptr<u64> sboLength)
{
	cellSsl.todo("cellSslCertGetPublicKey(sslCert=*0x%x, sboData=**0x%x, sboLength=*0x%x)", sslCert, sboData, sboLength);

	if (!g_fxo->get<ssl_manager>().is_init)
		return CELL_SSL_ERROR_NOT_INITIALIZED;

	if (!sslCert)
		return CELL_SSL_ERROR_INVALID_CERTIFICATE;

	if (!sboData || !sboLength)
		return CELL_SSL_ERROR_NO_BUFFER;

	return CELL_OK;
}

error_code cellSslCertGetRsaPublicKeyModulus(vm::cptr<void> sslCert, vm::cpptr<u8> sboData, vm::ptr<u64> sboLength)
{
	cellSsl.todo("cellSslCertGetRsaPublicKeyModulus(sslCert=*0x%x, sboData=**0x%x, sboLength=*0x%x)", sslCert, sboData, sboLength);

	if (!g_fxo->get<ssl_manager>().is_init)
		return CELL_SSL_ERROR_NOT_INITIALIZED;

	if (!sslCert)
		return CELL_SSL_ERROR_INVALID_CERTIFICATE;

	if (!sboData || !sboLength)
		return CELL_SSL_ERROR_NO_BUFFER;

	return CELL_OK;
}

error_code cellSslCertGetRsaPublicKeyExponent(vm::cptr<void> sslCert, vm::cpptr<u8> sboData, vm::ptr<u64> sboLength)
{
	cellSsl.todo("cellSslCertGetRsaPublicKeyExponent(sslCert=*0x%x, sboData=**0x%x, sboLength=*0x%x)", sslCert, sboData, sboLength);

	if (!g_fxo->get<ssl_manager>().is_init)
		return CELL_SSL_ERROR_NOT_INITIALIZED;

	if (!sslCert)
		return CELL_SSL_ERROR_INVALID_CERTIFICATE;

	if (!sboData || !sboLength)
		return CELL_SSL_ERROR_NO_BUFFER;

	return CELL_OK;
}

error_code cellSslCertGetNotBefore(vm::cptr<void> sslCert, vm::ptr<CellRtcTick> begin)
{
	cellSsl.todo("cellSslCertGetNotBefore(sslCert=*0x%x, begin=*0x%x)", sslCert, begin);

	if (!g_fxo->get<ssl_manager>().is_init)
		return CELL_SSL_ERROR_NOT_INITIALIZED;

	if (!sslCert)
		return CELL_SSL_ERROR_INVALID_CERTIFICATE;

	if (!begin)
		return CELL_SSL_ERROR_NO_BUFFER;

	return CELL_OK;
}

error_code cellSslCertGetNotAfter(vm::cptr<void> sslCert, vm::ptr<CellRtcTick> limit)
{
	cellSsl.todo("cellSslCertGetNotAfter(sslCert=*0x%x, limit=*0x%x)", sslCert, limit);

	if (!g_fxo->get<ssl_manager>().is_init)
		return CELL_SSL_ERROR_NOT_INITIALIZED;

	if (!sslCert)
		return CELL_SSL_ERROR_INVALID_CERTIFICATE;

	if (!limit)
		return CELL_SSL_ERROR_NO_BUFFER;

	return CELL_OK;
}

error_code cellSslCertGetSubjectName(vm::cptr<void> sslCert, vm::cpptr<void> certName)
{
	cellSsl.todo("cellSslCertGetSubjectName(sslCert=*0x%x, certName=**0x%x)", sslCert, certName);

	if (!g_fxo->get<ssl_manager>().is_init)
		return CELL_SSL_ERROR_NOT_INITIALIZED;

	if (!sslCert)
		return CELL_SSL_ERROR_INVALID_CERTIFICATE;

	if (!certName)
		return CELL_SSL_ERROR_NO_BUFFER;

	return CELL_OK;
}

error_code cellSslCertGetIssuerName(vm::cptr<void> sslCert, vm::cpptr<void> certName)
{
	cellSsl.todo("cellSslCertGetIssuerName(sslCert=*0x%x, certName=**0x%x)", sslCert, certName);

	if (!g_fxo->get<ssl_manager>().is_init)
		return CELL_SSL_ERROR_NOT_INITIALIZED;

	if (!sslCert)
		return CELL_SSL_ERROR_INVALID_CERTIFICATE;

	if (!certName)
		return CELL_SSL_ERROR_NO_BUFFER;

	return CELL_OK;
}

error_code cellSslCertGetNameEntryCount(vm::cptr<void> certName, vm::ptr<u32> entryCount)
{
	cellSsl.todo("cellSslCertGetNameEntryCount(certName=*0x%x, entryCount=*0x%x)", certName, entryCount);

	if (!g_fxo->get<ssl_manager>().is_init)
		return CELL_SSL_ERROR_NOT_INITIALIZED;

	if (!certName)
		return CELL_SSL_ERROR_INVALID_CERTIFICATE;

	if (!entryCount)
		return CELL_SSL_ERROR_NO_BUFFER;

	return CELL_OK;
}

error_code cellSslCertGetNameEntryInfo(vm::cptr<void> certName, u32 entryNum, vm::cpptr<char> oidName, vm::cpptr<u8> value, vm::ptr<u64> valueLength, s32 flag)
{
	cellSsl.todo("cellSslCertGetNameEntryInfo(certName=*0x%x, entryNum=%d, oidName=**0x%x, value=**0x%x, valueLength=*0x%x, flag=0x%x)", certName, entryNum, oidName, value, valueLength, flag);

	if (!g_fxo->get<ssl_manager>().is_init)
		return CELL_SSL_ERROR_NOT_INITIALIZED;

	if (!certName)
		return CELL_SSL_ERROR_INVALID_CERTIFICATE;

	if (!oidName || !value || !valueLength)
		return CELL_SSL_ERROR_NO_BUFFER;

	return CELL_OK;
}

error_code cellSslCertGetMd5Fingerprint(vm::cptr<void> sslCert, vm::cptr<u8> buf, vm::cptr<u32> plen)
{
	cellSsl.todo("cellSslCertGetMd5Fingerprint(sslCert=*0x%x, buf=*0x%x, plen=*0x%x)", sslCert, buf, plen);

	if (!g_fxo->get<ssl_manager>().is_init)
		return CELL_SSL_ERROR_NOT_INITIALIZED;

	if (!sslCert)
		return CELL_SSL_ERROR_INVALID_CERTIFICATE;

	if (!buf || !plen)
		return CELL_SSL_ERROR_NO_BUFFER;

	return CELL_OK;
}

error_code _cellSslConvertCipherId()
{
	UNIMPLEMENTED_FUNC(cellSsl);
	return CELL_OK;
}

error_code _cellSslConvertSslVersion()
{
	UNIMPLEMENTED_FUNC(cellSsl);
	return CELL_OK;
}

b8 _cellSslIsInitd()
{
	UNIMPLEMENTED_FUNC(cellSsl);
	return false;
}

error_code _cellSslPemReadPrivateKey()
{
	UNIMPLEMENTED_FUNC(cellSsl);
	return CELL_OK;
}

error_code _cellSslPemReadX509()
{
	UNIMPLEMENTED_FUNC(cellSsl);
	return CELL_OK;
}

error_code BER_read_item()
{
	UNIMPLEMENTED_FUNC(cellSsl);
	return CELL_OK;
}

error_code BIO_ctrl()
{
	UNIMPLEMENTED_FUNC(cellSsl);
	return CELL_OK;
}

error_code BIO_dump()
{
	UNIMPLEMENTED_FUNC(cellSsl);
	return CELL_OK;
}

error_code BIO_free()
{
	UNIMPLEMENTED_FUNC(cellSsl);
	return CELL_OK;
}

error_code BIO_get_cb_arg()
{
	UNIMPLEMENTED_FUNC(cellSsl);
	return CELL_OK;
}

error_code BIO_get_retry_reason()
{
	UNIMPLEMENTED_FUNC(cellSsl);
	return CELL_OK;
}

error_code BIO_new_mem()
{
	UNIMPLEMENTED_FUNC(cellSsl);
	return CELL_OK;
}

error_code BIO_new_socket()
{
	UNIMPLEMENTED_FUNC(cellSsl);
	return CELL_OK;
}

error_code BIO_printf()
{
	UNIMPLEMENTED_FUNC(cellSsl);
	return CELL_OK;
}

error_code BIO_ptr_ctrl()
{
	UNIMPLEMENTED_FUNC(cellSsl);
	return CELL_OK;
}

error_code BIO_set_cb_arg()
{
	UNIMPLEMENTED_FUNC(cellSsl);
	return CELL_OK;
}

error_code ERR_clear_error()
{
	UNIMPLEMENTED_FUNC(cellSsl);
	return CELL_OK;
}

error_code ERR_get_error()
{
	UNIMPLEMENTED_FUNC(cellSsl);
	return CELL_OK;
}

error_code ERR_error_string()
{
	UNIMPLEMENTED_FUNC(cellSsl);
	return CELL_OK;
}

error_code ERR_func_error_string()
{
	UNIMPLEMENTED_FUNC(cellSsl);
	return CELL_OK;
}

error_code ERR_peek_error()
{
	UNIMPLEMENTED_FUNC(cellSsl);
	return CELL_OK;
}

error_code EVP_PKEY_free()
{
	UNIMPLEMENTED_FUNC(cellSsl);
	return CELL_OK;
}

error_code R_time()
{
	UNIMPLEMENTED_FUNC(cellSsl);
	return CELL_OK;
}

error_code R_time_cmp()
{
	UNIMPLEMENTED_FUNC(cellSsl);
	return CELL_OK;
}

error_code R_time_export()
{
	UNIMPLEMENTED_FUNC(cellSsl);
	return CELL_OK;
}

error_code R_time_free()
{
	UNIMPLEMENTED_FUNC(cellSsl);
	return CELL_OK;
}

error_code R_time_import()
{
	UNIMPLEMENTED_FUNC(cellSsl);
	return CELL_OK;
}

error_code R_time_new()
{
	UNIMPLEMENTED_FUNC(cellSsl);
	return CELL_OK;
}

error_code SSL_CIPHER_description()
{
	UNIMPLEMENTED_FUNC(cellSsl);
	return CELL_OK;
}

error_code SSL_CIPHER_get_bits()
{
	UNIMPLEMENTED_FUNC(cellSsl);
	return CELL_OK;
}

error_code SSL_CIPHER_get_id()
{
	UNIMPLEMENTED_FUNC(cellSsl);
	return CELL_OK;
}

error_code SSL_CIPHER_get_name()
{
	UNIMPLEMENTED_FUNC(cellSsl);
	return CELL_OK;
}

error_code SSL_CIPHER_get_version()
{
	UNIMPLEMENTED_FUNC(cellSsl);
	return CELL_OK;
}

error_code SSL_CTX_ctrl()
{
	UNIMPLEMENTED_FUNC(cellSsl);
	return CELL_OK;
}

error_code SSL_CTX_free()
{
	UNIMPLEMENTED_FUNC(cellSsl);
	return CELL_OK;
}

error_code SSL_CTX_new()
{
	UNIMPLEMENTED_FUNC(cellSsl);
	return CELL_OK;
}

error_code SSL_CTX_set_app_verify_cb()
{
	UNIMPLEMENTED_FUNC(cellSsl);
	return CELL_OK;
}

error_code SSL_CTX_set_info_cb()
{
	UNIMPLEMENTED_FUNC(cellSsl);
	return CELL_OK;
}

error_code SSL_CTX_set_options()
{
	UNIMPLEMENTED_FUNC(cellSsl);
	return CELL_OK;
}

error_code SSL_CTX_set_verify_mode()
{
	UNIMPLEMENTED_FUNC(cellSsl);
	return CELL_OK;
}

error_code SSL_CTX_use_certificate()
{
	UNIMPLEMENTED_FUNC(cellSsl);
	return CELL_OK;
}

error_code SSL_CTX_use_PrivateKey()
{
	UNIMPLEMENTED_FUNC(cellSsl);
	return CELL_OK;
}

error_code SSL_SESSION_free()
{
	UNIMPLEMENTED_FUNC(cellSsl);
	return CELL_OK;
}

error_code SSL_alert_desc_string_long()
{
	UNIMPLEMENTED_FUNC(cellSsl);
	return CELL_OK;
}

error_code SSL_alert_type_string_long()
{
	UNIMPLEMENTED_FUNC(cellSsl);
	return CELL_OK;
}

error_code SSL_clear()
{
	UNIMPLEMENTED_FUNC(cellSsl);
	return CELL_OK;
}

error_code	SSL_do_handshake()
{
	UNIMPLEMENTED_FUNC(cellSsl);
	return CELL_OK;
}

error_code SSL_free()
{
	UNIMPLEMENTED_FUNC(cellSsl);
	return CELL_OK;
}

error_code SSL_get_current_cipher()
{
	UNIMPLEMENTED_FUNC(cellSsl);
	return CELL_OK;
}

error_code SSL_get_error()
{
	UNIMPLEMENTED_FUNC(cellSsl);
	return CELL_OK;
}

error_code SSL_get_rbio()
{
	UNIMPLEMENTED_FUNC(cellSsl);
	return CELL_OK;
}

error_code SSL_get_version()
{
	UNIMPLEMENTED_FUNC(cellSsl);
	return CELL_OK;
}

error_code SSL_new()
{
	UNIMPLEMENTED_FUNC(cellSsl);
	return CELL_OK;
}

error_code SSL_peek()
{
	UNIMPLEMENTED_FUNC(cellSsl);
	return CELL_OK;
}

error_code SSL_read()
{
	UNIMPLEMENTED_FUNC(cellSsl);
	return CELL_OK;
}

error_code SSL_set_bio()
{
	UNIMPLEMENTED_FUNC(cellSsl);
	return CELL_OK;
}

error_code SSL_set_connect_state()
{
	UNIMPLEMENTED_FUNC(cellSsl);
	return CELL_OK;
}

error_code SSL_set_session()
{
	UNIMPLEMENTED_FUNC(cellSsl);
	return CELL_OK;
}

error_code SSL_set_ssl_method()
{
	UNIMPLEMENTED_FUNC(cellSsl);
	return CELL_OK;
}

error_code SSL_shutdown()
{
	UNIMPLEMENTED_FUNC(cellSsl);
	return CELL_OK;
}

error_code SSL_state()
{
	UNIMPLEMENTED_FUNC(cellSsl);
	return CELL_OK;
}

error_code SSL_state_string_long()
{
	UNIMPLEMENTED_FUNC(cellSsl);
	return CELL_OK;
}

error_code SSL_version()
{
	UNIMPLEMENTED_FUNC(cellSsl);
	return CELL_OK;
}

error_code SSL_want()
{
	UNIMPLEMENTED_FUNC(cellSsl);
	return CELL_OK;
}

error_code SSL_write()
{
	UNIMPLEMENTED_FUNC(cellSsl);
	return CELL_OK;
}

error_code SSLCERT_check_private_key()
{
	UNIMPLEMENTED_FUNC(cellSsl);
	return CELL_OK;
}

error_code SSLCERT_free()
{
	UNIMPLEMENTED_FUNC(cellSsl);
	return CELL_OK;
}

error_code SSLCERT_from_binary()
{
	UNIMPLEMENTED_FUNC(cellSsl);
	return CELL_OK;
}

error_code SSLCERT_get_basic_constraints_int()
{
	UNIMPLEMENTED_FUNC(cellSsl);
	return CELL_OK;
}

error_code SSLCERT_get_extension()
{
	UNIMPLEMENTED_FUNC(cellSsl);
	return CELL_OK;
}

error_code SSLCERT_get_issuer_name()
{
	UNIMPLEMENTED_FUNC(cellSsl);
	return CELL_OK;
}

error_code SSLCERT_get_notAfter()
{
	UNIMPLEMENTED_FUNC(cellSsl);
	return CELL_OK;
}

error_code SSLCERT_get_notBefore()
{
	UNIMPLEMENTED_FUNC(cellSsl);
	return CELL_OK;
}

error_code SSLCERT_get_pubkey()
{
	UNIMPLEMENTED_FUNC(cellSsl);
	return CELL_OK;
}

error_code SSLCERT_get_subject_name()
{
	UNIMPLEMENTED_FUNC(cellSsl);
	return CELL_OK;
}

error_code SSLCERT_NAME_cmp()
{
	UNIMPLEMENTED_FUNC(cellSsl);
	return CELL_OK;
}

error_code SSLCERT_NAME_ENTRY_get_info()
{
	UNIMPLEMENTED_FUNC(cellSsl);
	return CELL_OK;
}

error_code SSLCERT_NAME_get_entry()
{
	UNIMPLEMENTED_FUNC(cellSsl);
	return CELL_OK;
}

error_code SSLCERT_NAME_get_entry_count()
{
	UNIMPLEMENTED_FUNC(cellSsl);
	return CELL_OK;
}

error_code SSLCERT_NAME_oneline()
{
	UNIMPLEMENTED_FUNC(cellSsl);
	return CELL_OK;
}

error_code SSLCERT_OID_to_string()
{
	UNIMPLEMENTED_FUNC(cellSsl);
	return CELL_OK;
}

error_code SSLCERT_verify()
{
	UNIMPLEMENTED_FUNC(cellSsl);
	return CELL_OK;
}

error_code SSLv3_client_method()
{
	UNIMPLEMENTED_FUNC(cellSsl);
	return CELL_OK;
}

error_code TLSv1_client_method()
{
	UNIMPLEMENTED_FUNC(cellSsl);
	return CELL_OK;
}

DECLARE(ppu_module_manager::cellSsl)("cellSsl", []()
{
	REG_FUNC(cellSsl, cellSslInit);
	REG_FUNC(cellSsl, cellSslEnd);
	REG_FUNC(cellSsl, cellSslGetMemoryInfo);

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
