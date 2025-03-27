#include "stdafx.h"
#include "aes.h"
#include "sha1.h"
#include "key_vault.h"
#include "util/logs.hpp"
#include "Utilities/StrUtil.h"
#include "Utilities/Thread.h"
#include "Emu/System.h"
#include "Emu/system_utils.hpp"
#include "Emu/VFS.h"
#include "unpkg.h"
#include "util/sysinfo.hpp"
#include "Loader/PSF.h"

#include <filesystem>

LOG_CHANNEL(pkg_log, "PKG");

package_reader::package_reader(const std::string& path, fs::file file)
	: m_path(path)
	, m_file(std::move(file))
{
	if (!m_file && !m_file.open(path))
	{
		pkg_log.error("PKG file not found!");
		return;
	}

	m_is_valid = read_header();

	if (!m_is_valid)
	{
		return;
	}

	m_is_valid = read_metadata();

	if (!m_is_valid)
	{
		return;
	}

	m_is_valid = set_decryption_key();

	if (!m_is_valid)
	{
		return;
	}

	const bool param_sfo_found = read_param_sfo();

	if (!param_sfo_found)
	{
		pkg_log.notice("PKG does not contain a PARAM.SFO");
	}
}

package_reader::~package_reader()
{
}

bool package_reader::read_header()
{
	if (m_path.empty() || !m_file)
	{
		pkg_log.error("Reading PKG header: no file to read!");
		return false;
	}

	if (archive_read(&m_header, sizeof(m_header)) != sizeof(m_header))
	{
		pkg_log.error("Reading PKG header: file is too short!");
		return false;
	}

	pkg_log.notice("Path: '%s'", m_path);
	pkg_log.notice("Header: pkg_magic = 0x%x = \"%s\"", +m_header.pkg_magic, std::string_view(reinterpret_cast<const char*>(&m_header.pkg_magic), 4).substr(1)); // Skip 0x7F
	pkg_log.notice("Header: pkg_type = 0x%x = %d", m_header.pkg_type, m_header.pkg_type);
	pkg_log.notice("Header: pkg_platform = 0x%x = %d", m_header.pkg_platform, m_header.pkg_platform);
	pkg_log.notice("Header: meta_offset = 0x%x = %d", m_header.meta_offset, m_header.meta_offset);
	pkg_log.notice("Header: meta_count = 0x%x = %d", m_header.meta_count, m_header.meta_count);
	pkg_log.notice("Header: meta_size = 0x%x = %d", m_header.meta_size, m_header.meta_size);
	pkg_log.notice("Header: file_count = 0x%x = %d", m_header.file_count, m_header.file_count);
	pkg_log.notice("Header: pkg_size = 0x%x = %d", m_header.pkg_size, m_header.pkg_size);
	pkg_log.notice("Header: data_offset = 0x%x = %d", m_header.data_offset, m_header.data_offset);
	pkg_log.notice("Header: data_size = 0x%x = %d", m_header.data_size, m_header.data_size);
	pkg_log.notice("Header: title_id = %s", m_header.title_id);
	pkg_log.notice("Header: qa_digest = 0x%x 0x%x", m_header.qa_digest[0], m_header.qa_digest[1]);
	pkg_log.notice("Header: klicensee = %s", m_header.klicensee.value());

	// Get extended PKG information for PSP or PSVita
	if (m_header.pkg_platform == PKG_PLATFORM_TYPE_PSP_PSVITA)
	{
		PKGExtHeader ext_header;

		archive_seek(PKG_HEADER_SIZE);

		if (archive_read(&ext_header, sizeof(ext_header)) != sizeof(ext_header))
		{
			pkg_log.error("Reading extended PKG header: file is too short!");
			return false;
		}

		pkg_log.notice("Extended header: magic = 0x%x = \"%s\"", +ext_header.magic, std::string_view(reinterpret_cast<const char*>(&ext_header.magic), 4).substr(1));
		pkg_log.notice("Extended header: unknown_1 = 0x%x = %d", ext_header.unknown_1, ext_header.unknown_1);
		pkg_log.notice("Extended header: ext_hdr_size = 0x%x = %d", ext_header.ext_hdr_size, ext_header.ext_hdr_size);
		pkg_log.notice("Extended header: ext_data_size = 0x%x = %d", ext_header.ext_data_size, ext_header.ext_data_size);
		pkg_log.notice("Extended header: main_and_ext_headers_hmac_offset = 0x%x = %d", ext_header.main_and_ext_headers_hmac_offset, ext_header.main_and_ext_headers_hmac_offset);
		pkg_log.notice("Extended header: metadata_header_hmac_offset = 0x%x = %d", ext_header.metadata_header_hmac_offset, ext_header.metadata_header_hmac_offset);
		pkg_log.notice("Extended header: tail_offset = 0x%x = %d", ext_header.tail_offset, ext_header.tail_offset);
		//pkg_log.notice("Extended header: padding1 = 0x%x = %d", ext_header.padding1, ext_header.padding1);
		pkg_log.notice("Extended header: pkg_key_id = 0x%x = %d", ext_header.pkg_key_id, ext_header.pkg_key_id);
		pkg_log.notice("Extended header: full_header_hmac_offset = 0x%x = %d", ext_header.full_header_hmac_offset, ext_header.full_header_hmac_offset);
		//pkg_log.notice("Extended header: padding2 = 0x%x = %d", ext_header.padding2, ext_header.padding2);
	}

	if (m_header.pkg_magic != std::bit_cast<le_t<u32>>("\x7FPKG"_u32))
	{
		pkg_log.error("Not a PKG file!");
		return false;
	}

	if (u64{umax} / sizeof(PKGEntry) < u64(m_header.file_count))
	{
		pkg_log.error("PKG file count is too large! (0x%x)", m_header.file_count);
		return false;
	}

	switch (const u16 type = m_header.pkg_type)
	{
	case PKG_RELEASE_TYPE_DEBUG:   break;
	case PKG_RELEASE_TYPE_RELEASE: break;
	default:
	{
		pkg_log.error("Unknown PKG type (0x%x)", type);
		return false;
	}
	}

	switch (const u16 platform = m_header.pkg_platform)
	{
	case PKG_PLATFORM_TYPE_PS3: break;
	case PKG_PLATFORM_TYPE_PSP_PSVITA: break;
	default:
	{
		pkg_log.error("Unknown PKG platform (0x%x)", platform);
		return false;
	}
	}

	if (m_header.pkg_size > m_file.size())
	{
		// Check if multi-files pkg
		if (!m_path.ends_with("_00.pkg"))
		{
			pkg_log.error("PKG file size mismatch (pkg_size=0x%llx)", m_header.pkg_size);
			return false;
		}

		std::vector<fs::file> filelist;
		filelist.emplace_back(std::move(m_file));

		const std::string name_wo_number = m_path.substr(0, m_path.size() - 7);
		u64 cursize = filelist[0].size();

		while (cursize < m_header.pkg_size)
		{
			const std::string archive_filename = fmt::format("%s_%02d.pkg", name_wo_number, filelist.size());

			fs::file archive_file(archive_filename);
			if (!archive_file)
			{
				pkg_log.error("Missing part of the multi-files pkg: %s", archive_filename);
				return false;
			}

			const usz add_size = archive_file.size();

			if (!add_size)
			{
				pkg_log.error("%s is empty, cannot read PKG", archive_filename);
				return false;
			}

			cursize += add_size;
			filelist.emplace_back(std::move(archive_file));
		}

		// Gather files
		m_file = fs::make_gather(std::move(filelist));
	}

	if (m_header.data_size + m_header.data_offset > m_header.pkg_size)
	{
		pkg_log.error("PKG data size mismatch (data_size=0x%llx, data_offset=0x%llx, file_size=0x%llx)", m_header.data_size, m_header.data_offset, m_header.pkg_size);
		return false;
	}

	return true;
}

bool package_reader::read_metadata()
{
	// Read title ID and use it as an installation directory
	m_install_dir.resize(9);
	archive_read_block(55, &m_install_dir.front(), m_install_dir.size());

	// Read package metadata

	archive_seek(m_header.meta_offset);

	for (u32 i = 0; i < m_header.meta_count; i++)
	{
		struct packet_T
		{
			be_t<u32> id;
			be_t<u32> size;
		} packet;

		archive_read(&packet, sizeof(packet));

		// TODO
		switch (+packet.id)
		{
		case 0x1:
		{
			if (packet.size == sizeof(m_metadata.drm_type))
			{
				archive_read(&m_metadata.drm_type, sizeof(m_metadata.drm_type));
				pkg_log.notice("Metadata: DRM Type = 0x%x = %d", m_metadata.drm_type, m_metadata.drm_type);
				continue;
			}
			else
			{
				pkg_log.error("Metadata: DRM Type size mismatch (0x%x)", packet.size);
			}
			break;
		}
		case 0x2:
		{
			if (packet.size == sizeof(m_metadata.content_type))
			{
				archive_read(&m_metadata.content_type, sizeof(m_metadata.content_type));
				pkg_log.notice("Metadata: Content Type = 0x%x = %d", m_metadata.content_type, m_metadata.content_type);
				continue;
			}
			else
			{
				pkg_log.error("Metadata: Content Type size mismatch (0x%x)", packet.size);
			}
			break;
		}
		case 0x3:
		{
			if (packet.size == sizeof(m_metadata.package_type))
			{
				archive_read(&m_metadata.package_type, sizeof(m_metadata.package_type));
				pkg_log.notice("Metadata: Package Type = 0x%x = %d", m_metadata.package_type, m_metadata.package_type);
				continue;
			}
			else
			{
				pkg_log.error("Metadata: Package Type size mismatch (0x%x)", packet.size);
			}
			break;
		}
		case 0x4:
		{
			if (packet.size == sizeof(m_metadata.package_size))
			{
				archive_read(&m_metadata.package_size, sizeof(m_metadata.package_size));
				pkg_log.notice("Metadata: Package Size = 0x%x = %d", m_metadata.package_size, m_metadata.package_size);
				continue;
			}
			else
			{
				pkg_log.error("Metadata: Package Size size mismatch (0x%x)", packet.size);
			}
			break;
		}
		case 0x5:
		{
			if (packet.size == sizeof(m_metadata.package_revision.data))
			{
				archive_read(&m_metadata.package_revision.data, sizeof(m_metadata.package_revision.data));
				m_metadata.package_revision.interpret_data();
				pkg_log.notice("Metadata: Package Revision = %s", m_metadata.package_revision.to_string());
				continue;
			}
			else
			{
				pkg_log.error("Metadata: Package Revision size mismatch (0x%x)", packet.size);
			}
			break;
		}
		case 0x6:
		{
			m_metadata.title_id.resize(12);

			if (packet.size == m_metadata.title_id.size())
			{
				archive_read(&m_metadata.title_id.front(), m_metadata.title_id.size());
				m_metadata.title_id = fmt::trim(m_metadata.title_id);
				pkg_log.notice("Metadata: Title ID = %s", m_metadata.title_id);
				continue;
			}
			else
			{
				pkg_log.error("Metadata: Title ID size mismatch (0x%x)", packet.size);
			}
			break;
		}
		case 0x7:
		{
			if (packet.size == sizeof(m_metadata.qa_digest))
			{
				archive_read(&m_metadata.qa_digest, sizeof(m_metadata.qa_digest));
				pkg_log.notice("Metadata: QA Digest = %s", std::span<const u8>(m_metadata.qa_digest, sizeof(m_metadata.qa_digest)));
				continue;
			}
			else
			{
				pkg_log.error("Metadata: QA Digest size mismatch (0x%x)", packet.size);
			}
			break;
		}
		case 0x8:
		{
			if (packet.size == sizeof(m_metadata.software_revision.data))
			{
				archive_read(&m_metadata.software_revision.data, sizeof(m_metadata.software_revision.data));
				m_metadata.software_revision.interpret_data();
				pkg_log.notice("Metadata: Software Revision = %s", m_metadata.software_revision.to_string());
				continue;
			}
			else
			{
				pkg_log.error("Metadata: Software Revision size mismatch (0x%x)", packet.size);
			}
			break;
		}
		case 0x9:
		{
			if (packet.size == sizeof(m_metadata.unk_0x9))
			{
				archive_read(&m_metadata.unk_0x9, sizeof(m_metadata.unk_0x9));
				pkg_log.notice("Metadata: unk_0x9 = 0x%x = %d", m_metadata.unk_0x9, m_metadata.unk_0x9);
				continue;
			}
			else
			{
				pkg_log.error("Metadata: unk_0x9 size mismatch (0x%x)", packet.size);
			}
			break;
		}
		case 0xA:
		{
			if (packet.size > 8)
			{
				// Read an actual installation directory (DLC)
				m_install_dir.resize(packet.size);
				archive_read(&m_install_dir.front(), packet.size);
				m_install_dir = m_install_dir.c_str() + 8;
				m_metadata.install_dir = m_install_dir;
				pkg_log.notice("Metadata: Install Dir = %s", m_metadata.install_dir);
				continue;
			}
			else
			{
				pkg_log.error("Metadata: Install Dir size mismatch (0x%x)", packet.size);
			}
			break;
		}
		case 0xB:
		{
			if (packet.size == sizeof(m_metadata.unk_0xB))
			{
				archive_read(&m_metadata.unk_0xB, sizeof(m_metadata.unk_0xB));
				pkg_log.notice("Metadata: unk_0xB = 0x%x = %d", m_metadata.unk_0xB, m_metadata.unk_0xB);
				continue;
			}
			else
			{
				pkg_log.error("Metadata: unk_0xB size mismatch (0x%x)", packet.size);
			}
			break;
		}
		case 0xC:
		{
			// Unknown
			break;
		}
		case 0xD: // PSVita stuff
		{
			if (packet.size == sizeof(m_metadata.item_info))
			{
				archive_read(&m_metadata.item_info, sizeof(m_metadata.item_info));
				pkg_log.notice("Metadata: PSVita item info = %s", m_metadata.item_info.to_string());
				continue;
			}
			else
			{
				pkg_log.error("Metadata: Item info size mismatch (0x%x)", packet.size);
			}
			break;
		}
		case 0xE: // PSVita stuff
		{
			if (packet.size == sizeof(m_metadata.sfo_info))
			{
				archive_read(&m_metadata.sfo_info, sizeof(m_metadata.sfo_info));
				pkg_log.notice("Metadata: PSVita sfo info = %s", m_metadata.sfo_info.to_string());
				continue;
			}
			else
			{
				pkg_log.error("Metadata: SFO info size mismatch (0x%x)", packet.size);
			}
			break;
		}
		case 0xF: // PSVita stuff
		{
			if (packet.size == sizeof(m_metadata.unknown_data_info))
			{
				archive_read(&m_metadata.unknown_data_info, sizeof(m_metadata.unknown_data_info));
				pkg_log.notice("Metadata: PSVita unknown data info = %s", m_metadata.unknown_data_info.to_string());
				continue;
			}
			else
			{
				pkg_log.error("Metadata: unknown data info size mismatch (0x%x)", packet.size);
			}
			break;
		}
		case 0x10: // PSVita stuff
		{
			if (packet.size == sizeof(m_metadata.entirety_info))
			{
				archive_read(&m_metadata.entirety_info, sizeof(m_metadata.entirety_info));
				pkg_log.notice("Metadata: PSVita entirety info = %s", m_metadata.entirety_info.to_string());
				continue;
			}
			else
			{
				pkg_log.error("Metadata: Entirety info size mismatch (0x%x)", packet.size);
			}
			break;
		}
		case 0x11: // PSVita stuff
		{
			if (packet.size == sizeof(m_metadata.version_info))
			{
				archive_read(&m_metadata.version_info, sizeof(m_metadata.version_info));
				pkg_log.notice("Metadata: PSVita version info = %s", m_metadata.version_info.to_string());
				continue;
			}
			else
			{
				pkg_log.error("Metadata: Version info size mismatch (0x%x)", packet.size);
			}
			break;
		}
		case 0x12: // PSVita stuff
		{
			if (packet.size == sizeof(m_metadata.self_info))
			{
				archive_read(&m_metadata.self_info, sizeof(m_metadata.self_info));
				pkg_log.notice("Metadata: PSVita self info = %s", m_metadata.self_info.to_string());
				continue;
			}
			else
			{
				pkg_log.error("Metadata: Self info size mismatch (0x%x)", packet.size);
			}
			break;
		}
		default:
		{
			pkg_log.error("Unknown packet id %d", packet.id);
			break;
		}
		}

		archive_seek(packet.size, fs::seek_cur);
	}

	return true;
}

bool package_reader::set_decryption_key()
{
	if (!m_is_valid)
	{
		return false;
	}

	if (m_header.pkg_platform == PKG_PLATFORM_TYPE_PSP_PSVITA && m_metadata.content_type >= 0x15 && m_metadata.content_type <= 0x17)
	{
		// PSVita
		// TODO: Not all the keys seem to match the content types. I was only able to install a dlc (0x16) with PKG_AES_KEY_VITA_1

		aes_context ctx;
		aes_setkey_enc(&ctx, m_metadata.content_type == 0x15u ? PKG_AES_KEY_VITA_1 : m_metadata.content_type == 0x16u ? PKG_AES_KEY_VITA_2 : PKG_AES_KEY_VITA_3, 128);
		aes_crypt_ecb(&ctx, AES_ENCRYPT, reinterpret_cast<const uchar*>(&m_header.klicensee), m_dec_key.data());
		return true;
	}

	std::memcpy(m_dec_key.data(), PKG_AES_KEY, m_dec_key.size());

	if (std::vector<PKGEntry> entries; !read_entries(entries))
	{
		pkg_log.notice("PKG may be IDU, retrying with IDU key.");

		std::memcpy(m_dec_key.data(), PKG_AES_KEY_IDU, m_dec_key.size());

		if (!read_entries(entries))
		{
			pkg_log.error("PKG decryption failed!");
			return false;
		}
	}

	return true;
}

bool package_reader::read_entries(std::vector<PKGEntry>& entries)
{
	entries.clear();
	entries.resize(m_header.file_count + BUF_PADDING / sizeof(PKGEntry) + 1);

	const usz read_size = decrypt(0, m_header.file_count * sizeof(PKGEntry), m_header.pkg_platform == PKG_PLATFORM_TYPE_PSP_PSVITA ? PKG_AES_KEY2 : m_dec_key.data(), entries.data());

	if (read_size < m_header.file_count * sizeof(PKGEntry))
	{
		return false;
	}

	entries.resize(m_header.file_count);

	const usz fsz = m_file.size() - m_header.data_offset;

	// Data integrity validation
	for (const PKGEntry& entry : entries)
	{
		if (!entry.name_size)
		{
			continue;
		}

		if (entry.name_size > PKG_MAX_FILENAME_SIZE)
		{
			return false;
		}

		if (fsz < entry.name_size || fsz - entry.name_size < entry.name_offset)
		{
			// Name exceeds file(s)
			return false;
		}

		if (entry.file_size)
		{
			if (fsz < entry.file_size || fsz - entry.file_size < entry.file_offset)
			{
				// Data exceeds file(s)
				return false;
			}

			if (entry.name_offset == entry.file_offset)
			{
				// Repeated value: odd
				return false;
			}
		}
	}

	return true;
}

bool package_reader::read_param_sfo()
{
	std::vector<PKGEntry> entries;

	if (!read_entries(entries))
	{
		return false;
	}

	std::vector<u8> data_buf;

	for (const PKGEntry& entry : entries)
	{
		if (entry.name_size > PKG_MAX_FILENAME_SIZE)
		{
			pkg_log.error("PKG name size is too big (size=0x%x, offset=0x%x)", entry.name_size, entry.name_offset);
			continue;
		}

		const bool is_psp = (entry.type & PKG_FILE_ENTRY_PSP) != 0u;

		std::string name(entry.name_size + BUF_PADDING, '\0');

		if (usz read_size = decrypt(entry.name_offset, entry.name_size, is_psp ? PKG_AES_KEY2 : m_dec_key.data(), name.data()); read_size < entry.name_size)
		{
			pkg_log.error("PKG name could not be read (size=0x%x, offset=0x%x)", entry.name_size, entry.name_offset);
			continue;
		}

		fmt::trim_back(name, "\0"sv);

		// We're looking for the PARAM.SFO file, if there is any
		if (usz ndelim = name.find_first_not_of('/'); ndelim == umax || name.substr(ndelim) != "PARAM.SFO")
		{
			continue;
		}

		// Read the package's PARAM.SFO
		fs::file tmp = fs::make_stream<std::vector<uchar>>();
		{
			for (u64 pos = 0; pos < entry.file_size; pos += BUF_SIZE)
			{
				const u64 block_size = std::min<u64>(BUF_SIZE, entry.file_size - pos);

				data_buf.resize(block_size + BUF_PADDING);

				if (decrypt(entry.file_offset + pos, block_size, is_psp ? PKG_AES_KEY2 : m_dec_key.data(), data_buf.data()) != block_size)
				{
					pkg_log.error("Failed to decrypt PARAM.SFO file");
					return false;
				}

				if (tmp.write(data_buf.data(), block_size) != block_size)
				{
					pkg_log.error("Failed to write to temporary PARAM.SFO file");
					return false;
				}
			}

			tmp.seek(0);

			m_psf = psf::load_object(tmp, name);

			if (m_psf.empty())
			{
				// Invalid
				continue;
			}

			return true;
		}
	}

	return false;
}

// TODO: maybe also check if VERSION matches
package_install_result package_reader::check_target_app_version() const
{
	if (!m_is_valid)
	{
		return {package_install_result::error_type::other};
	}

	const auto category       = psf::get_string(m_psf, "CATEGORY", "");
	const auto title_id       = psf::get_string(m_psf, "TITLE_ID", "");
	const auto app_ver        = psf::get_string(m_psf, "APP_VER", "");
	const auto target_app_ver = psf::get_string(m_psf, "TARGET_APP_VER", "");

	if (category != "GD")
	{
		// We allow anything that isn't an update for now
		return {package_install_result::error_type::no_error};
	}

	if (title_id.empty())
	{
		// Let's allow packages without ID for now
		return {package_install_result::error_type::no_error};
	}

	if (app_ver.empty())
	{
		if (!target_app_ver.empty())
		{
			// Let's see if this case exists
			pkg_log.fatal("Trying to install an unversioned patch with a target app version (%s). Please contact a developer!", target_app_ver);
		}

		// This is probably not a version dependant patch, so we may install the package
		return {package_install_result::error_type::no_error};
	}

	const std::string sfo_path = rpcs3::utils::get_hdd0_dir() + "game/" + std::string(title_id) + "/PARAM.SFO";
	const fs::file installed_sfo_file(sfo_path);
	if (!installed_sfo_file)
	{
		if (!target_app_ver.empty())
		{
			// We are unable to compare anything with the target app version
			pkg_log.error("A target app version is required (%s), but no PARAM.SFO was found for %s. (path='%s', error=%s)", target_app_ver, title_id, sfo_path, fs::g_tls_error);
			return {package_install_result::error_type::app_version, {std::string(target_app_ver)}};
		}

		// There is nothing we need to compare, so we may install the package
		return {package_install_result::error_type::no_error};
	}

	const auto installed_psf = psf::load_object(installed_sfo_file, sfo_path);

	const auto installed_title_id = psf::get_string(installed_psf, "TITLE_ID", "");
	const auto installed_app_ver  = psf::get_string(installed_psf, "APP_VER", "");

	if (title_id != installed_title_id || installed_app_ver.empty())
	{
		// Let's allow this package for now
		return {package_install_result::error_type::no_error};
	}

	std::add_pointer_t<char> ev0, ev1;
	const double old_version = std::strtod(installed_app_ver.data(), &ev0);

	if (installed_app_ver.data() + installed_app_ver.size() != ev0)
	{
		pkg_log.error("Failed to convert the installed app version to double (%s)", installed_app_ver);
		return {package_install_result::error_type::other};
	}

	if (target_app_ver.empty())
	{
		// This is most likely the first patch. Let's make sure its version is high enough for the installed game.

		const double new_version = std::strtod(app_ver.data(), &ev1);

		if (app_ver.data() + app_ver.size() != ev1)
		{
			pkg_log.error("Failed to convert the package's app version to double (%s)", app_ver);
			return {package_install_result::error_type::other};
		}

		if (new_version >= old_version)
		{
			// Yay! The patch has a higher or equal version than the installed game.
			return {package_install_result::error_type::no_error};
		}

		pkg_log.error("The new app version (%s) is smaller than the installed app version (%s)", app_ver, installed_app_ver);
		return {package_install_result::error_type::app_version, {std::string(app_ver), std::string(installed_app_ver)}};
	}

	// Check if the installed app version matches the target app version

	const double target_version = std::strtod(target_app_ver.data(), &ev1);

	if (target_app_ver.data() + target_app_ver.size() != ev1)
	{
		pkg_log.error("Failed to convert the package's target app version to double (%s)", target_app_ver);
		return {package_install_result::error_type::other};
	}

	if (target_version == old_version)
	{
		// Yay! This patch is for the installed game version.
		return {package_install_result::error_type::no_error};
	}

	pkg_log.error("The installed app version (%s) does not match the target app version (%s)", installed_app_ver, target_app_ver);
	return {package_install_result::error_type::app_version, {std::string(target_app_ver), std::string(installed_app_ver)}};
}

bool package_reader::set_install_path()
{
	if (!m_is_valid)
	{
		return false;
	}

	m_install_path.clear();

	// Get full path
	std::string dir = rpcs3::utils::get_hdd0_dir();

	// Based on https://www.psdevwiki.com/ps3/PKG_files#ContentType
	switch (m_metadata.content_type)
	{
	case PKG_CONTENT_TYPE_THEME:
		dir += "theme/";
		break;
	case PKG_CONTENT_TYPE_WIDGET:
		dir += "widget/";
		break;
	case PKG_CONTENT_TYPE_LICENSE:
		dir += "home/" + Emu.GetUsr() + "/exdata/";
		break;
	case PKG_CONTENT_TYPE_VSH_MODULE:
		dir += "vsh/modules/";
		break;
	case PKG_CONTENT_TYPE_PSN_AVATAR:
		dir += "home/" + Emu.GetUsr() + "/psn_avatar/";
		break;
	case PKG_CONTENT_TYPE_VMC:
		dir += "tmp/vmc/";
		break;
	// TODO: Find out if other content types are installed elsewhere
	default:
		dir += "game/";
		break;
	}

	// TODO: Verify whether other content types require appending title ID
	if (m_metadata.content_type != PKG_CONTENT_TYPE_LICENSE)
		dir += m_install_dir + '/';

	// If false, an existing directory is being overwritten: cannot cancel the operation
	m_was_null = !fs::is_dir(dir);

	m_install_path = dir;
	return true;
}

bool package_reader::fill_data(std::map<std::string, install_entry*>& all_install_entries)
{
	if (!m_is_valid)
	{
		return false;
	}

	if (!fs::create_path(m_install_path))
	{
		pkg_log.error("Could not create the installation directory %s (error=%s)", m_install_path, fs::g_tls_error);
		return false;
	}

	m_install_entries.clear();
	m_bootable_file_path.clear();
	m_entry_indexer = 0;
	m_written_bytes = 0;

	usz num_failures = 0;

	std::vector<PKGEntry> entries;

	if (!read_entries(entries))
	{
		return false;
	}

	// Create directories first
	for (const auto& entry : entries)
	{
		if (entry.name_size > PKG_MAX_FILENAME_SIZE)
		{
			num_failures++;
			pkg_log.error("PKG name size is too big (size=0x%x, offset=0x%x)", entry.name_size, entry.name_offset);
			break;
		}

		std::string name(entry.name_size + BUF_PADDING, '\0');

		const bool is_psp = (entry.type & PKG_FILE_ENTRY_PSP) != 0u;

		if (const usz read_size = decrypt(entry.name_offset, entry.name_size, is_psp ? PKG_AES_KEY2 : m_dec_key.data(), name.data()); read_size < entry.name_size)
		{
			num_failures++;
			pkg_log.error("PKG name could not be read (size=0x%x, offset=0x%x)", entry.name_size, entry.name_offset);
			break;
		}

		fmt::trim_back(name, "\0"sv);

		std::string path = m_install_path + vfs::escape(name);

		if (entry.pad || (entry.type & ~PKG_FILE_ENTRY_KNOWN_BITS))
		{
			pkg_log.todo("Entry with unknown type or padding: type=0x%08x, pad=0x%x, name='%s'", entry.type, entry.pad, name);
		}
		else
		{
			pkg_log.notice("Entry: type=0x%08x, name='%s'", entry.type, name);
		}

		const u8 entry_type = entry.type & 0xff;

		switch (entry_type)
		{
		case PKG_FILE_ENTRY_FOLDER:
		case 0x12:
		{
			if (fs::is_dir(path))
			{
				pkg_log.warning("Reused existing directory %s", path);
			}
			else if (fs::create_path(path))
			{
				pkg_log.notice("Created directory %s", path);
			}
			else
			{
				num_failures++;
				pkg_log.error("Failed to create directory %s", path);
				break;
			}

			break;
		}
		default:
		{
			// TODO: check for valid utf8 characters
			const std::string true_path = std::filesystem::weakly_canonical(path).string();
			if (true_path.empty())
			{
				num_failures++;
				pkg_log.error("Failed to get weakly_canonical path for '%s'", path);
				break;
			}

			auto map_ptr = &*all_install_entries.try_emplace(true_path).first;

			m_install_entries.push_back({
				.weak_reference = map_ptr,
				.name = std::string(name),
				.file_offset = entry.file_offset,
				.file_size = entry.file_size,
				.type = entry.type,
				.pad = entry.pad
			});

			if (map_ptr->second && !(entry.type & PKG_FILE_ENTRY_OVERWRITE))
			{
				// Cannot override
				continue;
			}

			// Link
			map_ptr->second = &m_install_entries.back();
			continue;
		}
		}
	}

	if (num_failures != 0)
	{
		pkg_log.error("Package installation failed: %s", m_install_path);
		return false;
	}

	return true;
}

fs::file DecryptEDAT(const fs::file& input, const std::string& input_file_name, int mode, u8 *custom_klic);

void package_reader::extract_worker()
{
	std::vector<u8> read_cache;

	while (m_num_failures == 0 && !m_aborted)
	{
		// Make sure m_entry_indexer does not exceed m_install_entries
		const usz index = m_entry_indexer.fetch_op([this](usz& v)
		{
			if (v < m_install_entries.size())
			{
				v++;
				return true;
			}

			return false;
		}).first;

		if (index >= m_install_entries.size())
		{
			break;
		}

		const install_entry& entry = ::at32(m_install_entries, index);

		if (!entry.is_dominating())
		{
			// Overwritten by another entry
			m_written_bytes += entry.file_size;
			continue;
		}

		const bool is_psp = (entry.type & PKG_FILE_ENTRY_PSP) != 0u;

		const std::string& path = entry.weak_reference->first;
		const std::string& name = entry.name;

		if (entry.pad || (entry.type & ~PKG_FILE_ENTRY_KNOWN_BITS))
		{
			pkg_log.todo("Entry with unknown type or padding: type=0x%08x, pad=0x%x, name='%s'", entry.type, entry.pad, name);
		}
		else
		{
			pkg_log.notice("Entry: type=0x%08x, name='%s'", entry.type, name);
		}

		switch (const u8 entry_type = entry.type & 0xff)
		{
		case PKG_FILE_ENTRY_NPDRM:
		case PKG_FILE_ENTRY_NPDRMEDAT:
		case PKG_FILE_ENTRY_SDAT:
		case PKG_FILE_ENTRY_REGULAR:
		case PKG_FILE_ENTRY_UNK0:
		case PKG_FILE_ENTRY_UNK1:
		case 0xe:
		case 0x10:
		case 0x11:
		case 0x13:
		case 0x14:
		case 0x15:
		case 0x16:
		case 0x18:
		case 0x19:
		{
			const bool did_overwrite = fs::is_file(path);

			if (did_overwrite && !(entry.type & PKG_FILE_ENTRY_OVERWRITE))
			{
				pkg_log.notice("Didn't overwrite %s", path);
				break;
			}

			const bool is_buffered = entry_type == PKG_FILE_ENTRY_SDAT;

			if (entry_type == PKG_FILE_ENTRY_NPDRMEDAT)
			{
				pkg_log.warning("NPDRM EDAT!");
			}

			if (fs::file out{ path, did_overwrite ? fs::rewrite : fs::write_new })
			{
				bool extract_success = true;

				struct pkg_file_reader : fs::file_base
				{
					const std::function<u64(u64, void*, u64)> m_read_func;
					const install_entry& m_entry;
					usz m_pos;

					explicit pkg_file_reader(std::function<u64(u64, void* buffer, u64)> read_func, const install_entry& entry) noexcept
						: m_read_func(std::move(read_func))
						, m_entry(entry)
						, m_pos(0)
					{
					}

					fs::stat_t get_stat() override
					{
						fs::stat_t stat{};
						stat.size = m_entry.file_size;
						return stat;
					}

					bool trunc(u64) override
					{
						return false;
					}

					u64 read(void* buffer, u64 size) override
					{
						const u64 result = pkg_file_reader::read_at(m_pos, buffer, size);
						m_pos += result;
						return result;
					}

					u64 read_at(u64 offset, void* buffer, u64 size) override
					{
						return m_read_func(offset, buffer, size);
					}

					u64 write(const void*, u64) override
					{
						return 0;
					}

					u64 seek(s64 offset, fs::seek_mode whence) override
					{
						const s64 new_pos =
							whence == fs::seek_set ? offset :
							whence == fs::seek_cur ? offset + m_pos :
							whence == fs::seek_end ? offset + size() : -1;

						if (new_pos < 0)
						{
							fs::g_tls_error = fs::error::inval;
							return -1;
						}

						m_pos = new_pos;
						return m_pos;
					}

					u64 size() override
					{
						return m_entry.file_size;
					}

					fs::file_id get_id() override
					{
						fs::file_id id{};

						id.type.insert(0, "pkg_file_reader: "sv);
						return id;
					}
				};

				read_cache.clear();

				auto reader = std::make_unique<pkg_file_reader>([&, cache_off = u64{umax}](usz pos, void* ptr, usz size) mutable -> u64
				{
					if (pos >= entry.file_size || !size)
					{
						return 0;
					}

					size = std::min<u64>(entry.file_size - pos, size);

					u64 size_cache_end = 0;
					u64 read_size = 0;

					// Check if exists in cache
					if (!read_cache.empty() && cache_off <= pos && pos < cache_off + read_cache.size())
					{
						read_size = std::min<u64>(pos + size, cache_off + read_cache.size()) - pos;

						std::memcpy(ptr, read_cache.data() + (pos - cache_off), read_size);
						pos += read_size;
					}
					else if (!read_cache.empty() && cache_off < pos + size && cache_off + read_cache.size() >= pos + size)
					{
						size_cache_end = size - (std::max<u64>(cache_off, pos) - pos);

						std::memcpy(static_cast<u8*>(ptr) + (cache_off - pos), read_cache.data(), size_cache_end);
						size -= size_cache_end;
					}

					if (pos >= entry.file_size || !size)
					{
						return read_size + size_cache_end;
					}

					// Try to cache for later
					if (size <= BUF_SIZE && !size_cache_end && !read_size)
					{
						const u64 block_size = std::min<u64>({BUF_SIZE, std::max<u64>(size * 5 / 3, 65536), entry.file_size - pos});

						read_cache.resize(block_size + BUF_PADDING);
						cache_off = pos;

						const usz advance_size = decrypt(entry.file_offset + pos, block_size, is_psp ? PKG_AES_KEY2 : m_dec_key.data(), read_cache.data());

						if (!advance_size)
						{
							cache_off = umax;
							return 0;
						}

						read_cache.resize(advance_size);

						size = std::min<usz>(advance_size, size);
						std::memcpy(ptr, read_cache.data(), size);
						return size;
					}

					while (read_size < size)
					{
						const u64 block_size = std::min<u64>(BUF_SIZE, size - read_size);

						const usz advance_size = decrypt(entry.file_offset + pos, block_size, is_psp ? PKG_AES_KEY2 : m_dec_key.data(), static_cast<u8*>(ptr) + read_size);

						if (!advance_size)
						{
							break;
						}

						read_size += advance_size;
						pos += advance_size;
					}

					return read_size + size_cache_end;
				}, entry);

				fs::file in_data;
				in_data.reset(std::move(reader));

				fs::file final_data;

				if (is_buffered)
				{
					final_data = DecryptEDAT(in_data, name, 1, reinterpret_cast<u8*>(&m_header.klicensee));
				}
				else
				{
					final_data = std::move(in_data);
				}

				if (!final_data)
				{
					m_num_failures++;
					pkg_log.error("Failed to decrypt EDAT file %s (error=%s)", path, fs::g_tls_error);
					break;
				}

				// 16MB buffer
				std::vector<u8> buffer(std::min<usz>(entry.file_size, 1u << 24) + BUF_PADDING);

				while (usz read_size = final_data.read(buffer.data(), buffer.size() - BUF_PADDING))
				{
					out.write(buffer.data(), read_size);
					m_written_bytes += read_size;
				}

				final_data.close();
				out.close();

				if (extract_success)
				{
					if (did_overwrite)
					{
						pkg_log.warning("Overwritten file %s", path);
					}
					else
					{
						pkg_log.notice("Created file %s", path);

						if (name == "USRDIR/EBOOT.BIN" && entry.file_size > 4)
						{
							// Expose the creation of a bootable file
							m_bootable_file_path = path;
						}
					}
				}
				else
				{
					m_num_failures++;
				}
			}
			else
			{
				m_num_failures++;
				pkg_log.error("Failed to create file %s (is_buffered=%d, did_overwrite=%d, error=%s)", path, is_buffered, did_overwrite, fs::g_tls_error);
			}

			break;
		}
		default:
		{
			m_num_failures++;
			pkg_log.error("Unknown PKG entry type (0x%x) %s", entry.type, name);
			break;
		}
		}
	}
}

package_install_result package_reader::extract_data(std::deque<package_reader>& readers, std::deque<std::string>& bootable_paths)
{
	package_install_result::error_type error = package_install_result::error_type::no_error;
	usz num_failures = 0;

	// Set paths first in order to know if the install dir was empty before starting any installations.
	// This will also allow us to remove all the new packages in one path at once if any of them fail.
	for (package_reader& reader : readers)
	{
		reader.m_result = result::not_started;

		if (!reader.set_install_path())
		{
			error = package_install_result::error_type::other;
			reader.m_result = result::error; // We don't know if it's dirty yet.
			return {error};
		}
	}

	for (package_reader& reader : readers)
	{
		// Use a seperate map for each reader. We need to check if the target app version exists for each package in sequence.
		std::map<std::string, install_entry*> all_install_entries;

		if (error != package_install_result::error_type::no_error || num_failures > 0)
		{
			ensure(reader.m_result == result::error || reader.m_result == result::error_dirty);
			return {error};
		}

		// Check if this package is allowed to be installed on top of the existing data
		const package_install_result version_check = reader.check_target_app_version();

		if (version_check.error != package_install_result::error_type::no_error)
		{
			reader.m_result = result::error; // We don't know if it's dirty yet.
			return version_check;
		}

		reader.m_result = result::started;

		// Parse the files to be installed and create all paths.
		if (!reader.fill_data(all_install_entries))
		{
			error = package_install_result::error_type::other;
			// Do not return yet. We may need to clean up down below.
		}

		reader.m_num_failures = error == package_install_result::error_type::no_error ? 0 : 1;

		if (reader.m_num_failures == 0)
		{
			const usz thread_count = std::min<usz>(utils::get_thread_count(), reader.m_install_entries.size());

			named_thread_group workers("PKG Installer "sv, std::max<u32>(::narrow<u32>(thread_count), 1) - 1, [&]()
			{
				reader.extract_worker();
			});

			reader.extract_worker();
			workers.join();
		}

		num_failures += reader.m_num_failures;

		// We don't count this package as aborted if all entries were processed.
		if (reader.m_num_failures || (reader.m_aborted && reader.m_entry_indexer < reader.m_install_entries.size()))
		{
			// Clear boot path. We don't want to propagate potentially broken paths to the caller.
			reader.m_bootable_file_path.clear();

			bool cleaned = reader.m_was_null;

			if (reader.m_was_null && fs::is_dir(reader.m_install_path))
			{
				pkg_log.notice("Removing partial installation ('%s')", reader.m_install_path);

				if (!fs::remove_all(reader.m_install_path, true))
				{
					pkg_log.notice("Failed to remove partial installation ('%s') (error=%s)", reader.m_install_path, fs::g_tls_error);
					cleaned = false;
				}
			}

			if (reader.m_num_failures)
			{
				pkg_log.error("Package failed to install ('%s')", reader.m_install_path);
				reader.m_result = cleaned ? result::error : result::error_dirty;
			}
			else
			{
				pkg_log.warning("Package installation aborted ('%s')", reader.m_install_path);
				reader.m_result = cleaned ? result::aborted : result::aborted_dirty;
			}

			break;
		}

		reader.m_result = result::success;

		if (reader.get_progress(1) != 1)
		{
			pkg_log.warning("Missing %d bytes from PKG total files size.", reader.m_header.data_size - reader.m_written_bytes);
			reader.m_written_bytes = reader.m_header.data_size; // Mark as completed anyway
		}

		// May be empty
		bootable_paths.emplace_back(std::move(reader.m_bootable_file_path));
	}

	if (error == package_install_result::error_type::no_error && num_failures > 0)
	{
		error = package_install_result::error_type::other;
	}

	return {error};
}

void package_reader::archive_seek(const s64 new_offset, const fs::seek_mode damode)
{
	if (m_file) m_file.seek(new_offset, damode);
}

u64 package_reader::archive_read(void* data_ptr, const u64 num_bytes)
{
	return m_file ? m_file.read(data_ptr, num_bytes) : 0;
}

std::span<const char> package_reader::archive_read_block(u64 offset, void* data_ptr, u64 num_bytes)
{
	const usz read_n = m_file.read_at(offset, data_ptr, num_bytes);

	return {static_cast<const char*>(data_ptr), read_n};
}

usz package_reader::decrypt(u64 offset, u64 size, const uchar* key, void* local_buf)
{
	if (!m_is_valid)
	{
		return 0;
	}

	if (m_header.data_offset > ~offset)
	{
		return 0;
	}

	// Read the data and set available size
	const auto data_span = archive_read_block(m_header.data_offset + offset, local_buf, size);
	ensure(data_span.data() == static_cast<void*>(local_buf));

	// Get block count
	const u64 blocks = (data_span.size() + 15) / 16;
	const auto out_data = reinterpret_cast<u8*>(local_buf);

	if (m_header.pkg_type == PKG_RELEASE_TYPE_DEBUG)
	{
		// Debug key
		be_t<u64> input[8] =
		{
			m_header.qa_digest[0],
			m_header.qa_digest[0],
			m_header.qa_digest[1],
			m_header.qa_digest[1],
		};

		for (u64 i = 0; i < blocks; i++)
		{
			// Initialize stream cipher for current position
			input[7] = offset / 16 + i;

			struct sha1_hash
			{
				u8 data[20];
			} hash{};

			sha1(reinterpret_cast<const u8*>(input), sizeof(input), hash.data);

			const u128 v = read_from_ptr<u128>(out_data, i * 16);
			write_to_ptr<u128>(out_data, i * 16, v ^ read_from_ptr<u128>(hash.data));
		}
	}
	else if (m_header.pkg_type == PKG_RELEASE_TYPE_RELEASE)
	{
		aes_context ctx;

		// Set encryption key for stream cipher
		aes_setkey_enc(&ctx, key, 128);

		// Initialize stream cipher for start position
		be_t<u128> input = m_header.klicensee.value() + offset / 16;

		// Increment stream position for every block
		for (u64 i = 0; i < blocks; i++, input++)
		{
			u128 key;

			aes_crypt_ecb(&ctx, AES_ENCRYPT, reinterpret_cast<const u8*>(&input), reinterpret_cast<u8*>(&key));

			const u128 v = read_from_ptr<u128>(out_data, i * 16);
			write_to_ptr<u128>(out_data, i * 16, v ^ key);
		}
	}
	else
	{
		pkg_log.error("Unknown release type (0x%x)", m_header.pkg_type);
	}

	if (blocks * 16 != size)
	{
		// Put NTS and other zeroes on unaligned reads
		std::memset(out_data + size, 0, blocks * 16 - size);
	}

	// Return the amount of data written in buf
	return std::min<usz>(size, data_span.size());
}

int package_reader::get_progress(int maximum) const
{
	const usz wr = m_written_bytes;

	return wr >= m_header.data_size ? maximum : ::narrow<int>(wr * maximum / m_header.data_size);
}

void package_reader::abort_extract()
{
	m_aborted = true;
}
