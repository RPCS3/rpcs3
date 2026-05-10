#include "stdafx.h"
#include "decrypt_binaries.h"
#include "unedat.h"
#include "unself.h"
#include "Emu/IdManager.h"
#include "Emu/System.h"
#include "Utilities/StrUtil.h"

#include <charconv>
#include <iostream>

LOG_CHANNEL(dec_log, "DECRYPT");

usz decrypt_binaries_t::decrypt(std::string_view klic_input)
{
	if (m_index >= m_modules.size())
	{
		std::cout << "No paths specified" << std::endl; // For CLI
		m_index = umax;
		return umax;
	}

	if (m_klics.empty())
	{
		dec_log.notice("Decrypting binaries...");
		std::cout << "Decrypting binaries..." << std::endl; // For CLI

		// Always start with no KLIC
		m_klics.emplace_back(u128{});

		if (const auto keys = g_fxo->try_get<loaded_npdrm_keys>())
		{
			// Second klic: get it from a running game
			if (const u128 klic = keys->last_key())
			{
				m_klics.emplace_back(klic);
			}
		}

		// Try to use the key that has been for the current running ELF
		m_klics.insert(m_klics.end(), Emu.klic.begin(), Emu.klic.end());
	}

	if (std::string_view text = klic_input.substr(klic_input.find_first_of('x') + 1); text.size() == 32)
	{
		// Allowed to fail (would simply repeat the operation if fails again)
		u64 lo = 0;
		u64 hi = 0;
		bool success = false;

		if (auto res = std::from_chars(text.data() + 0, text.data() + 16, lo, 16); res.ec == std::errc() && res.ptr == text.data() + 16)
		{
			if (res = std::from_chars(text.data() + 16, text.data() + 32, hi, 16); res.ec == std::errc() && res.ptr == text.data() + 32)
			{
				success = true;
			}
		}

		if (success)
		{
			lo = std::bit_cast<be_t<u64>>(lo);
			hi = std::bit_cast<be_t<u64>>(hi);

			if (u128 input_key = ((u128{hi} << 64) | lo))
			{
				m_klics.emplace_back(input_key);
			}
		}
	}

	while (m_index < m_modules.size())
	{
		const std::string& _module = m_modules[m_index];
		const std::string old_path = _module;

		fs::file elf_file;
		fs::file internal_file;

		bool invalid = false;
		usz key_it = 0;
		u32 file_magic{};

		while (true)
		{
			for (; key_it < m_klics.size(); key_it++)
			{
				internal_file.close();

				if (!elf_file.open(old_path) || !elf_file.read(file_magic))
				{
					file_magic = 0;
				}

				switch (file_magic)
				{
				case "SCE\0"_u32:
				{
					// First KLIC is no KLIC
					elf_file = decrypt_self(elf_file, key_it != 0 ? reinterpret_cast<u8*>(&m_klics[key_it]) : nullptr);

					if (!elf_file)
					{
						// Try another key
						continue;
					}

					break;
				}
				case "NPD\0"_u32:
				{
					// EDAT / SDAT
					internal_file = std::move(elf_file);
					elf_file = DecryptEDAT(internal_file, old_path, key_it != 0 ? 8 : 1, reinterpret_cast<u8*>(&m_klics[key_it]));

					if (!elf_file)
					{
						// Try another key
						continue;
					}

					break;
				}
				default:
				{
					invalid = true;
					break;
				}
				}

				if (invalid)
				{
					elf_file.close();
				}

				break;
			}

			if (elf_file)
			{
				const std::string exec_ext = fmt::to_lower(_module).ends_with(".sprx") ? ".prx" : ".elf";
				const std::string new_path = file_magic == "NPD\0"_u32 ? old_path + ".unedat" :
					old_path.substr(0, old_path.find_last_of('.')) + exec_ext;

				if (fs::file new_file{new_path, fs::rewrite})
				{
					// 16MB buffer
					std::vector<u8> buffer(std::min<usz>(elf_file.size(), 1u << 24));

					elf_file.seek(0);

					while (usz read_size = elf_file.read(buffer.data(), buffer.size()))
					{
						new_file.write(buffer.data(), read_size);
					}

					dec_log.success("Decrypted %s -> %s", old_path, new_path);
					std::cout << "Decrypted " << old_path << " -> " << new_path << std::endl; // For CLI
					m_index++;
				}
				else
				{
					dec_log.error("Failed to create %s", new_path);
					std::cout << "Failed to create " << new_path << std::endl; // For CLI
					m_index = umax;
				}

				break;
			}

			if (!invalid)
			{
				// Allow the user to manually type KLIC if decryption failed
				return m_index;
			}

			dec_log.error("Failed to decrypt \"%s\".", old_path);
			std::cout << "Failed to decrypt \"" << old_path << "\"." << std::endl; // For CLI
			return m_index;
		}
	}

	dec_log.notice("Finished decrypting all binaries.");
	std::cout << "Finished decrypting all binaries." << std::endl; // For CLI
	return m_index;
}
