#include "stdafx.h"
#include "aes.h"
#include "sha1.h"
#include "key_vault.h"
#include "util/logs.hpp"
#include "Utilities/StrUtil.h"
#include "Emu/System.h"
#include "Emu/VFS.h"
#include "unpkg.h"
#include "Loader/PSF.h"

LOG_CHANNEL(pkg_log, "PKG");

package_reader::package_reader(const std::string& path)
	: m_path(path)
{
	if (!fs::is_file(path))
	{
		pkg_log.error("PKG file not found!");
		return;
	}

	filelist.emplace_back(fs::file{ path });

	m_is_valid = read_header();

	if (!m_is_valid)
	{
		return;
	}

	m_is_valid = read_metadata();
}

package_reader::~package_reader()
{
}

bool package_reader::read_header()
{
	if (m_path.empty() || filelist.empty())
	{
		pkg_log.error("Reading PKG header: no file to read!");
		return false;
	}

	if (archive_read(&header, sizeof(header)) != sizeof(header))
	{
		pkg_log.error("Reading PKG header: file is too short!");
		return false;
	}

	pkg_log.notice("Header: pkg_magic = 0x%x = \"%s\"", +header.pkg_magic, std::string(reinterpret_cast<const char*>(&header.pkg_magic), 4));
	pkg_log.notice("Header: pkg_type = 0x%x = %d", header.pkg_type, header.pkg_type);
	pkg_log.notice("Header: pkg_platform = 0x%x = %d", header.pkg_platform, header.pkg_platform);
	pkg_log.notice("Header: meta_offset = 0x%x = %d", header.meta_offset, header.meta_offset);
	pkg_log.notice("Header: meta_count = 0x%x = %d", header.meta_count, header.meta_count);
	pkg_log.notice("Header: meta_size = 0x%x = %d", header.meta_size, header.meta_size);
	pkg_log.notice("Header: file_count = 0x%x = %d", header.file_count, header.file_count);
	pkg_log.notice("Header: pkg_size = 0x%x = %d", header.pkg_size, header.pkg_size);
	pkg_log.notice("Header: data_offset = 0x%x = %d", header.data_offset, header.data_offset);
	pkg_log.notice("Header: data_size = 0x%x = %d", header.data_size, header.data_size);
	pkg_log.notice("Header: title_id = %s", header.title_id);
	pkg_log.notice("Header: qa_digest = 0x%x 0x%x", header.qa_digest[0], header.qa_digest[1]);
	//pkg_log.notice("Header: klicensee = 0x%x = %d", header.klicensee, header.klicensee);

	// Get extended PKG information for PSP or PSVita
	if (header.pkg_platform == PKG_PLATFORM_TYPE_PSP_PSVITA)
	{
		PKGExtHeader ext_header;

		archive_seek(PKG_HEADER_SIZE);

		if (archive_read(&ext_header, sizeof(ext_header)) != sizeof(ext_header))
		{
			pkg_log.error("Reading extended PKG header: file is too short!");
			return false;
		}

		pkg_log.notice("Extended header: magic = 0x%x = \"%s\"", +ext_header.magic, std::string(reinterpret_cast<const char*>(&ext_header.magic), 4));
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

	if (header.pkg_magic != std::bit_cast<le_t<u32>>("\x7FPKG"_u32))
	{
		pkg_log.error("Not a PKG file!");
		return false;
	}

	switch (const u16 type = header.pkg_type)
	{
	case PKG_RELEASE_TYPE_DEBUG:   break;
	case PKG_RELEASE_TYPE_RELEASE: break;
	default:
	{
		pkg_log.error("Unknown PKG type (0x%x)", type);
		return false;
	}
	}

	switch (const u16 platform = header.pkg_platform)
	{
	case PKG_PLATFORM_TYPE_PS3: break;
	case PKG_PLATFORM_TYPE_PSP_PSVITA: break;
	default:
	{
		pkg_log.error("Unknown PKG platform (0x%x)", platform);
		return false;
	}
	}

	if (header.pkg_size > filelist[0].size())
	{
		// Check if multi-files pkg
		if (!m_path.ends_with("_00.pkg"))
		{
			pkg_log.error("PKG file size mismatch (pkg_size=0x%llx)", header.pkg_size);
			return false;
		}

		const std::string name_wo_number = m_path.substr(0, m_path.size() - 7);
		u64 cursize = filelist[0].size();
		while (cursize < header.pkg_size)
		{
			const std::string archive_filename = fmt::format("%s_%02d.pkg", name_wo_number, filelist.size());

			fs::file archive_file(archive_filename);
			if (!archive_file)
			{
				pkg_log.error("Missing part of the multi-files pkg: %s", archive_filename);
				return false;
			}

			cursize += archive_file.size();
			filelist.emplace_back(std::move(archive_file));
		}
	}

	if (header.data_size + header.data_offset > header.pkg_size)
	{
		pkg_log.error("PKG data size mismatch (data_size=0x%llx, data_offset=0x%llx, file_size=0x%llx)", header.data_size, header.data_offset, header.pkg_size);
		return false;
	}

	return true;
}

bool package_reader::read_metadata()
{
	if (!m_is_valid)
	{
		return false;
	}

	// Read title ID and use it as an installation directory
	install_dir.resize(9);
	archive_seek(55);
	archive_read(&install_dir.front(), install_dir.size());

	// Read package metadata

	archive_seek(header.meta_offset);

	for (u32 i = 0; i < header.meta_count; i++)
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
			if (packet.size == sizeof(metadata.drm_type))
			{
				archive_read(&metadata.drm_type, sizeof(metadata.drm_type));
				pkg_log.notice("Metadata: DRM Type = 0x%x = %d", metadata.drm_type, metadata.drm_type);
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
			if (packet.size == sizeof(metadata.content_type))
			{
				archive_read(&metadata.content_type, sizeof(metadata.content_type));
				pkg_log.notice("Metadata: Content Type = 0x%x = %d", metadata.content_type, metadata.content_type);
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
			if (packet.size == sizeof(metadata.package_type))
			{
				archive_read(&metadata.package_type, sizeof(metadata.package_type));
				pkg_log.notice("Metadata: Package Type = 0x%x = %d", metadata.package_type, metadata.package_type);
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
			if (packet.size == sizeof(metadata.package_size))
			{
				archive_read(&metadata.package_size, sizeof(metadata.package_size));
				pkg_log.notice("Metadata: Package Size = 0x%x = %d", metadata.package_size, metadata.package_size);
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
			if (packet.size == sizeof(metadata.package_revision.data))
			{
				archive_read(&metadata.package_revision.data, sizeof(metadata.package_revision.data));
				metadata.package_revision.interpret_data();
				pkg_log.notice("Metadata: Package Revision = %s", metadata.package_revision.to_string());
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
			metadata.title_id.resize(12);

			if (packet.size == metadata.title_id.size())
			{
				archive_read(&metadata.title_id, metadata.title_id.size());
				metadata.title_id = fmt::trim(metadata.title_id);
				pkg_log.notice("Metadata: Title ID = %s", metadata.title_id);
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
			if (packet.size == sizeof(metadata.qa_digest))
			{
				archive_read(&metadata.qa_digest, sizeof(metadata.qa_digest));
				pkg_log.notice("Metadata: QA Digest = 0x%x", metadata.qa_digest);
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
			if (packet.size == sizeof(metadata.software_revision.data))
			{
				archive_read(&metadata.software_revision.data, sizeof(metadata.software_revision.data));
				metadata.software_revision.interpret_data();
				pkg_log.notice("Metadata: Software Revision = %s", metadata.software_revision.to_string());
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
			if (packet.size == sizeof(metadata.unk_0x9))
			{
				archive_read(&metadata.unk_0x9, sizeof(metadata.unk_0x9));
				pkg_log.notice("Metadata: unk_0x9 = 0x%x = %d", metadata.unk_0x9, metadata.unk_0x9);
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
				install_dir.resize(packet.size);
				archive_read(&install_dir.front(), packet.size);
				install_dir = install_dir.c_str() + 8;
				metadata.install_dir = install_dir;
				pkg_log.notice("Metadata: Install Dir = %s", metadata.install_dir);
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
			if (packet.size == sizeof(metadata.unk_0xB))
			{
				archive_read(&metadata.unk_0xB, sizeof(metadata.unk_0xB));
				pkg_log.notice("Metadata: unk_0xB = 0x%x = %d", metadata.unk_0xB, metadata.unk_0xB);
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
			if (packet.size == sizeof(metadata.item_info))
			{
				archive_read(&metadata.item_info, sizeof(metadata.item_info));
				pkg_log.notice("Metadata: PSVita item info = %s", metadata.item_info.to_string());
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
			if (packet.size == sizeof(metadata.sfo_info))
			{
				archive_read(&metadata.sfo_info, sizeof(metadata.sfo_info));
				pkg_log.notice("Metadata: PSVita sfo info = %s", metadata.sfo_info.to_string());
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
			if (packet.size == sizeof(metadata.unknown_data_info))
			{
				archive_read(&metadata.unknown_data_info, sizeof(metadata.unknown_data_info));
				pkg_log.notice("Metadata: PSVita unknown data info = %s", metadata.unknown_data_info.to_string());
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
			if (packet.size == sizeof(metadata.entirety_info))
			{
				archive_read(&metadata.entirety_info, sizeof(metadata.entirety_info));
				pkg_log.notice("Metadata: PSVita entirety info = %s", metadata.entirety_info.to_string());
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
			if (packet.size == sizeof(metadata.version_info))
			{
				archive_read(&metadata.version_info, sizeof(metadata.version_info));
				pkg_log.notice("Metadata: PSVita version info = %s", metadata.version_info.to_string());
				continue;
			}
			else
			{
				pkg_log.error("Metadata: Version info size mismatch (0x%x)", packet.size);
			}
		}
		case 0x12: // PSVita stuff
		{
			if (packet.size == sizeof(metadata.self_info))
			{
				archive_read(&metadata.self_info, sizeof(metadata.self_info));
				pkg_log.notice("Metadata: PSVita self info = %s", metadata.self_info.to_string());
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

bool package_reader::decrypt_data()
{
	if (!m_is_valid)
	{
		return false;
	}

	if (header.pkg_platform == PKG_PLATFORM_TYPE_PSP_PSVITA && metadata.content_type >= 0x15 && metadata.content_type <= 0x17)
	{
		// PSVita
		// TODO: Not all the keys seem to match the content types. I was only able to install a dlc (0x16) with PKG_AES_KEY_VITA_1

		aes_context ctx;
		aes_setkey_enc(&ctx, metadata.content_type == 0x15u ? PKG_AES_KEY_VITA_1 : metadata.content_type == 0x16u ? PKG_AES_KEY_VITA_2 : PKG_AES_KEY_VITA_3, 128);
		aes_crypt_ecb(&ctx, AES_ENCRYPT, reinterpret_cast<const uchar*>(&header.klicensee), dec_key.data());
		decrypt(0, header.file_count * sizeof(PKGEntry), dec_key.data());
	}
	else
	{
		std::memcpy(dec_key.data(), PKG_AES_KEY, dec_key.size());
		decrypt(0, header.file_count * sizeof(PKGEntry), header.pkg_platform == PKG_PLATFORM_TYPE_PSP_PSVITA ? PKG_AES_KEY2 : dec_key.data());
	}

	return true;
}

// TODO: maybe also check if VERSION matches
package_error package_reader::check_target_app_version()
{
	if (!decrypt_data())
	{
		return package_error::other;
	}

	std::vector<PKGEntry> entries(header.file_count);

	std::memcpy(entries.data(), buf.get(), entries.size() * sizeof(PKGEntry));

	for (const auto& entry : entries)
	{
		if (entry.name_size > 256)
		{
			pkg_log.error("PKG name size is too big (0x%x)", entry.name_size);
			continue;
		}

		const bool is_psp = (entry.type & PKG_FILE_ENTRY_PSP) != 0u;

		decrypt(entry.name_offset, entry.name_size, is_psp ? PKG_AES_KEY2 : dec_key.data());

		const std::string name{ reinterpret_cast<char*>(buf.get()), entry.name_size };

		// We're looking for the PARAM.SFO file, if there is any 
		if (name != "PARAM.SFO")
		{
			continue;
		}

		// Read the package's PARAM.SFO
		if (fs::file tmp = fs::make_stream<std::vector<uchar>>())
		{
			for (u64 pos = 0; pos < entry.file_size; pos += BUF_SIZE)
			{
				const u64 block_size = std::min<u64>(BUF_SIZE, entry.file_size - pos);

				if (decrypt(entry.file_offset + pos, block_size, is_psp ? PKG_AES_KEY2 : dec_key.data()) != block_size)
				{
					pkg_log.error("Failed to decrypt PARAM.SFO file");
					return package_error::other;
				}

				if (tmp.write(buf.get(), block_size) != block_size)
				{
					pkg_log.error("Failed to write to temporary PARAM.SFO file");
					return package_error::other;
				}
			}

			tmp.seek(0);

			const auto psf = psf::load_object(tmp);

			const auto category = psf::get_string(psf, "CATEGORY", "");
			const auto title_id = psf::get_string(psf, "TITLE_ID", "");
			const auto app_ver = psf::get_string(psf, "APP_VER", "");
			const auto target_app_ver = psf::get_string(psf, "TARGET_APP_VER", "");

			if (category != "GD")
			{
				// We allow anything that isn't an update for now
				return package_error::no_error;
			}

			if (title_id.empty())
			{
				// Let's allow packages without ID for now
				return package_error::no_error;
			}

			if (app_ver.empty())
			{
				if (!target_app_ver.empty())
				{
					// Let's see if this case exists
					pkg_log.fatal("Trying to install an unversioned patch with a target app version (%s). Please contact a developer!", target_app_ver);
				}

				// This is probably not a version dependant patch, so we may install the package
				return package_error::no_error;
			}

			const fs::file installed_sfo_file(Emu.GetHddDir() + "game/" + std::string(title_id) + "/PARAM.SFO");
			if (!installed_sfo_file)
			{
				if (!target_app_ver.empty())
				{
					// We are unable to compare anything with the target app version
					pkg_log.error("A target app version is required (%s), but no PARAM.SFO was found for %s", target_app_ver, title_id);
					return package_error::app_version;
				}

				// There is nothing we need to compare, so we may install the package
				return package_error::no_error;
			}

			const auto installed_psf = psf::load_object(installed_sfo_file);

			const auto installed_title_id = psf::get_string(installed_psf, "TITLE_ID", "");
			const auto installed_app_ver = psf::get_string(installed_psf, "APP_VER", "");

			if (title_id != installed_title_id || installed_app_ver.empty())
			{
				// Let's allow this package for now
				return package_error::no_error;
			}

			std::add_pointer_t<char> ev0, ev1;
			const double old_version = std::strtod(installed_app_ver.data(), &ev0);

			if (installed_app_ver.data() + installed_app_ver.size() != ev0)
			{
				pkg_log.error("Failed to convert the installed app version to double (%s)", installed_app_ver);
				return package_error::other;
			}

			if (target_app_ver.empty())
			{
				// This is most likely the first patch. Let's make sure its version is high enough for the installed game.

				const double new_version = std::strtod(app_ver.data(), &ev1);

				if (app_ver.data() + app_ver.size() != ev1)
				{
					pkg_log.error("Failed to convert the package's app version to double (%s)", app_ver);
					return package_error::other;
				}

				if (new_version >= old_version)
				{
					// Yay! The patch has a higher or equal version than the installed game.
					return package_error::no_error;
				}

				pkg_log.error("The new app version (%s) is smaller than the installed app version (%s)", app_ver, installed_app_ver);
				return package_error::app_version;
			}

			// Check if the installed app version matches the target app version

			const double target_version = std::strtod(target_app_ver.data(), &ev1);

			if (target_app_ver.data() + target_app_ver.size() != ev1)
			{
				pkg_log.error("Failed to convert the package's target app version to double (%s)", target_app_ver);
				return package_error::other;
			}

			if (target_version == old_version)
			{
				// Yay! This patch is for the installed game version.
				return package_error::no_error;
			}

			pkg_log.error("The installed app version (%s) does not match the target app version (%s)", installed_app_ver, target_app_ver);
			return package_error::app_version;
		}

		pkg_log.error("Failed to create temporary PARAM.SFO file");
		return package_error::other;
	}

	return package_error::no_error;
}

bool package_reader::extract_data(atomic_t<double>& sync)
{
	if (!m_is_valid)
	{
		return false;
	}

	// Get full path and create the directory
	std::string dir = Emulator::GetHddDir();

	// Based on https://www.psdevwiki.com/ps3/PKG_files#ContentType
	switch (metadata.content_type)
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

	dir += install_dir + '/';

	// If false, an existing directory is being overwritten: cannot cancel the operation
	const bool was_null = !fs::is_dir(dir);

	if (!fs::create_path(dir))
	{
		pkg_log.error("Could not create the installation directory %s", dir);
		return false;
	}

	if (!decrypt_data())
	{
		return false;
	}

	size_t num_failures = 0;

	std::vector<PKGEntry> entries(header.file_count);

	std::memcpy(entries.data(), buf.get(), entries.size() * sizeof(PKGEntry));

	for (const auto& entry : entries)
	{
		if (entry.name_size > 256)
		{
			num_failures++;
			pkg_log.error("PKG name size is too big (0x%x)", entry.name_size);
			continue;
		}

		const bool is_psp = (entry.type & PKG_FILE_ENTRY_PSP) != 0u;

		decrypt(entry.name_offset, entry.name_size, is_psp ? PKG_AES_KEY2 : dec_key.data());

		const std::string name{ reinterpret_cast<char*>(buf.get()), entry.name_size };
		const std::string path = dir + vfs::escape(name);

		pkg_log.notice("Entry 0x%08x: %s", entry.type, name);

		switch (entry.type & 0xff)
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

			if (fs::file out{ path, fs::rewrite })
			{
				bool extract_success = true;
				for (u64 pos = 0; pos < entry.file_size; pos += BUF_SIZE)
				{
					const u64 block_size = std::min<u64>(BUF_SIZE, entry.file_size - pos);

					if (decrypt(entry.file_offset + pos, block_size, is_psp ? PKG_AES_KEY2 : dec_key.data()) != block_size)
					{
						extract_success = false;
						pkg_log.error("Failed to extract file %s", path);
						break;
					}

					if (out.write(buf.get(), block_size) != block_size)
					{
						extract_success = false;
						pkg_log.error("Failed to write file %s", path);
						break;
					}

					if (sync.fetch_add((block_size + 0.0) / header.data_size) < 0.)
					{
						if (was_null)
						{
							pkg_log.error("Package installation cancelled: %s", dir);
							out.close();
							fs::remove_all(dir, true);
							return false;
						}

						// Cannot cancel the installation
						sync += 1.;
					}
				}

				if (extract_success)
				{
					if (did_overwrite)
					{
						pkg_log.warning("Overwritten file %s", path);
					}
					else
					{
						pkg_log.notice("Created file %s", path);
					}
				}
				else
				{
					num_failures++;
				}
			}
			else
			{
				num_failures++;
				pkg_log.error("Failed to create file %s", path);
			}

			break;
		}

		case PKG_FILE_ENTRY_FOLDER:
		case 0x12:
		{
			if (fs::create_dir(path))
			{
				pkg_log.notice("Created directory %s", path);
			}
			else if (fs::is_dir(path))
			{
				pkg_log.warning("Reused existing directory %s", path);
			}
			else
			{
				num_failures++;
				pkg_log.error("Failed to create directory %s", path);
			}

			break;
		}

		default:
		{
			num_failures++;
			pkg_log.error("Unknown PKG entry type (0x%x) %s", entry.type, name);
		}
		}
	}

	if (num_failures == 0)
	{
		pkg_log.success("Package successfully installed to %s", dir);
	}
	else
	{
		fs::remove_all(dir, true);
		pkg_log.error("Package installation failed: %s", dir);
	}

	return num_failures == 0;
}

void package_reader::archive_seek(const s64 new_offset, const fs::seek_mode damode)
{
	if (damode == fs::seek_set)
		cur_offset = new_offset;
	else if (damode == fs::seek_cur)
		cur_offset += new_offset;

	u64 _offset = 0;
	for (size_t i = 0; i < filelist.size(); i++)
	{
		if (cur_offset < (_offset + filelist[i].size()))
		{
			cur_file = i;
			cur_file_offset = cur_offset - _offset;
			filelist[i].seek(cur_file_offset);
			break;
		}
		_offset += filelist[i].size();
	}
};

u64 package_reader::archive_read(void* data_ptr, const u64 num_bytes)
{
	ASSERT(filelist.size() > cur_file && filelist[cur_file]);

	const u64 num_bytes_left = filelist[cur_file].size() - cur_file_offset;

	// check if it continues in another file
	if (num_bytes > num_bytes_left)
	{
		filelist[cur_file].read(data_ptr, num_bytes_left);

		if ((cur_file + 1) < filelist.size())
		{
			++cur_file;
		}
		else
		{
			cur_offset += num_bytes_left;
			cur_file_offset = filelist[cur_file].size();
			return num_bytes_left;
		}
		const u64 num_read = filelist[cur_file].read(static_cast<u8*>(data_ptr) + num_bytes_left, num_bytes - num_bytes_left);
		cur_offset += (num_read + num_bytes_left);
		cur_file_offset = num_read;
		return (num_read + num_bytes_left);
	}

	const u64 num_read = filelist[cur_file].read(data_ptr, num_bytes);

	cur_offset += num_read;
	cur_file_offset += num_read;

	return num_read;
};

u64 package_reader::decrypt(u64 offset, u64 size, const uchar* key)
{
	if (!m_is_valid)
	{
		return 0;
	}

	if (!buf)
	{
		// Allocate buffer with BUF_SIZE size or more if required
		buf.reset(new u128[std::max<u64>(BUF_SIZE, sizeof(PKGEntry) * header.file_count) / sizeof(u128)]);
	}

	archive_seek(header.data_offset + offset);

	// Read the data and set available size
	const u64 read = archive_read(buf.get(), size);

	// Get block count
	const u64 blocks = (read + 15) / 16;

	if (header.pkg_type == PKG_RELEASE_TYPE_DEBUG)
	{
		// Debug key
		be_t<u64> input[8] =
		{
			header.qa_digest[0],
			header.qa_digest[0],
			header.qa_digest[1],
			header.qa_digest[1],
		};

		for (u64 i = 0; i < blocks; i++)
		{
			// Initialize stream cipher for current position
			input[7] = offset / 16 + i;

			union sha1_hash
			{
				u8 data[20];
				u128 _v128;
			} hash;

			sha1(reinterpret_cast<const u8*>(input), sizeof(input), hash.data);

			buf[i] ^= hash._v128;
		}
	}
	else if (header.pkg_type == PKG_RELEASE_TYPE_RELEASE)
	{
		aes_context ctx;

		// Set encryption key for stream cipher
		aes_setkey_enc(&ctx, key, 128);

		// Initialize stream cipher for start position
		be_t<u128> input = header.klicensee.value() + offset / 16;

		// Increment stream position for every block
		for (u64 i = 0; i < blocks; i++, input++)
		{
			u128 key;

			aes_crypt_ecb(&ctx, AES_ENCRYPT, reinterpret_cast<const u8*>(&input), reinterpret_cast<u8*>(&key));

			buf[i] ^= key;
		}
	}
	else
	{
		pkg_log.error("Unknown release type (0x%x)", header.pkg_type);
	}

	// Return the amount of data written in buf
	return read;
};
