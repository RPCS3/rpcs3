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

void decrypt_sprx_libraries(std::vector<std::string> modules, std::function<std::string(std::string old_path, std::string path, bool tried)> input_cb)
{
	if (modules.empty())
	{
		std::cout << "No paths specified" << std::endl; // For CLI
		return;
	}

	dec_log.notice("Decrypting binaries...");
	std::cout << "Decrypting binaries..." << std::endl; // For CLI

	// Always start with no KLIC
	std::vector<u128> klics{u128{}};

	if (const auto keys = g_fxo->try_get<loaded_npdrm_keys>())
	{
		// Second klic: get it from a running game
		if (const u128 klic = keys->last_key())
		{
			klics.emplace_back(klic);
		}
	}

	// Try to use the key that has been for the current running ELF
	klics.insert(klics.end(), Emu.klic.begin(), Emu.klic.end());

	for (const std::string& _module : modules)
	{
		const std::string old_path = _module;

		fs::file elf_file;

		bool tried = false;
		bool invalid = false;
		usz key_it = 0;
		u32 file_magic{};

		while (true)
		{
			for (; key_it < klics.size(); key_it++)
			{
				if (!elf_file.open(old_path) || !elf_file.read(file_magic))
				{
					file_magic = 0;
				}

				switch (file_magic)
				{
				case "SCE\0"_u32:
				{
					// First KLIC is no KLIC
					elf_file = decrypt_self(std::move(elf_file), key_it != 0 ? reinterpret_cast<u8*>(&klics[key_it]) : nullptr);

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
					elf_file = DecryptEDAT(elf_file, old_path, key_it != 0 ? 8 : 1, reinterpret_cast<u8*>(&klics[key_it]), true);

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
					new_file.write(elf_file.to_string());
					dec_log.success("Decrypted %s -> %s", old_path, new_path);
					std::cout << "Decrypted " << old_path << " -> " << new_path << std::endl; // For CLI
				}
				else
				{
					dec_log.error("Failed to create %s", new_path);
					std::cout << "Failed to create " << new_path << std::endl; // For CLI
				}

				break;
			}

			if (!invalid)
			{
				// Allow the user to manually type KLIC if decryption failed

				const std::string filename = old_path.substr(old_path.find_last_of(fs::delim) + 1);
				const std::string text = input_cb ? input_cb(old_path, filename, tried) : "";

				if (!text.empty())
				{
					auto& klic = (tried ? klics.back() : klics.emplace_back());

					ensure(text.size() == 32);

					// It must succeed (only hex characters are present)
					u64 lo_ = 0;
					u64 hi_ = 0;
					std::from_chars(&text[0], &text[16], lo_, 16);
					std::from_chars(&text[16], &text[32], hi_, 16);

					be_t<u64> lo = std::bit_cast<be_t<u64>>(lo_);
					be_t<u64> hi = std::bit_cast<be_t<u64>>(hi_);

					klic = (u128{+hi} << 64) | +lo;

					// Retry with specified KLIC
					key_it -= +std::exchange(tried, true); // Rewind on second and above attempt
					dec_log.notice("KLIC entered for %s: %s", filename, klic);
					continue;
				}

				dec_log.notice("User has cancelled entering KLIC.");
			}

			dec_log.error("Failed to decrypt \"%s\".", old_path);
			std::cout << "Failed to decrypt \"" << old_path << "\"." << std::endl; // For CLI
			break;
		}
	}

	dec_log.notice("Finished decrypting all binaries.");
	std::cout << "Finished decrypting all binaries." << std::endl; // For CLI
}
