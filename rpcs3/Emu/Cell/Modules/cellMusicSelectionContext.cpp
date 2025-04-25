#include "stdafx.h"
#include "cellMusic.h"
#include "util/yaml.hpp"
#include "Emu/VFS.h"

#include <random>

// This is just a helper and not a real cell entity

LOG_CHANNEL(cellMusicSelectionContext);

bool music_selection_context::set(const CellMusicSelectionContext& in)
{
	if (memcmp(in.data, magic, sizeof(magic)) != 0)
	{
		return false;
	}

	constexpr u32 pos = sizeof(magic);
	hash = &in.data[pos];

	return load_playlist();
}

CellMusicSelectionContext music_selection_context::get() const
{
	if (hash.size() + sizeof(magic) > CELL_MUSIC_SELECTION_CONTEXT_SIZE)
	{
		fmt::throw_exception("Contents of music_selection_context are too large");
	}

	CellMusicSelectionContext out{};
	u32 pos = 0;

	std::memset(out.data, 0, CELL_MUSIC_SELECTION_CONTEXT_SIZE);
	std::memcpy(out.data, magic, sizeof(magic));
	pos += sizeof(magic);
	std::memcpy(&out.data[pos], hash.c_str(), hash.size());

	return out;
}

std::string music_selection_context::to_string() const
{
	std::string str = fmt::format(".magic='%s', .content_type=%d, .repeat_mode=%d, .context_option=%d, .first_track=%d, .tracks=%d, .hash='%s', .playlist:",
		magic, static_cast<u32>(content_type), static_cast<u32>(repeat_mode), static_cast<u32>(context_option), first_track, playlist.size(), hash);

	for (usz i = 0; i < playlist.size(); i++)
	{
		fmt::append(str, "\n - Track %d: %s", i, ::at32(playlist, i));
	}

	return str;
}

std::string music_selection_context::get_next_hash()
{
	static u64 hash_counter = 0;
	return fmt::format("music_selection_context_%d", hash_counter++);
}

std::string music_selection_context::context_to_hex(const CellMusicSelectionContext& context)
{
	std::string dahex;

	for (usz i = 0; i < CELL_MUSIC_SELECTION_CONTEXT_SIZE; i++)
	{
		fmt::append(dahex, " %.2x", context.data[i]);
	}

	return dahex;
}

std::string music_selection_context::get_yaml_path() const
{
	std::string path = fs::get_cache_dir() + "cache/playlists/";

	if (!fs::create_path(path))
	{
		cellMusicSelectionContext.fatal("get_yaml_path: Failed to create path: %s (%s)", path, fs::g_tls_error);
	}

	return path + hash + ".yml";
}

void music_selection_context::set_playlist(const std::string& path)
{
	playlist.clear();

	const std::string dir_path = "/dev_hdd0/music";
	const std::string vfs_dir_path = vfs::get("/dev_hdd0/music");

	if (fs::is_dir(path))
	{
		content_type = CELL_SEARCH_CONTENTTYPE_MUSICLIST;

		for (auto&& dir_entry : fs::dir{path})
		{
			if (dir_entry.name == "." || dir_entry.name == "..")
			{
				continue;
			}

			std::string track = dir_path + std::string(path + "/" + dir_entry.name).substr(vfs_dir_path.length());
			cellMusicSelectionContext.notice("set_playlist: Adding track to playlist: '%s'. (path: '%s', name: '%s')", track, path, dir_entry.name);
			playlist.push_back(std::move(track));
		}
	}
	else
	{
		content_type = CELL_SEARCH_CONTENTTYPE_MUSIC;

		std::string track = dir_path + path.substr(vfs_dir_path.length());
		cellMusicSelectionContext.notice("set_playlist: Adding track to playlist: '%s'. (path: '%s')", track, path);
		playlist.push_back(std::move(track));
	}

	valid = true;
}

void music_selection_context::create_playlist(const std::string& new_hash)
{
	hash = new_hash;

	const std::string yaml_path = get_yaml_path();
	cellMusicSelectionContext.notice("create_playlist: Saving music playlist file %s", yaml_path);

	YAML::Emitter out;
	out << YAML::BeginMap;
	out << "Version" << target_version;
	out << "FileType" << target_file_type;
	out << "ContentType" << content_type;
	out << "ContextOption" << context_option;
	out << "RepeatMode" << repeat_mode;
	out << "FirstTrack" << first_track;
	out << "Tracks" << YAML::BeginSeq;

	for (const std::string& track : playlist)
	{
		out << track;
	}

	out << YAML::EndSeq;
	out << YAML::EndMap;

	fs::pending_file file(yaml_path);

	if (!file.file || file.file.write(out.c_str(), out.size()) < out.size() || !file.commit())
	{
		cellMusicSelectionContext.error("create_playlist: Failed to create music playlist file '%s' (error=%s)", yaml_path, fs::g_tls_error);
	}
}

bool music_selection_context::load_playlist()
{
	playlist.clear();

	const std::string path = get_yaml_path();
	cellMusicSelectionContext.notice("load_playlist: Loading music playlist file '%s'", path);

	std::string content;
	{
		// Load patch file
		fs::file file{path};

		if (!file)
		{
			cellMusicSelectionContext.error("load_playlist: Failed to load music playlist file '%s': %s", path, fs::g_tls_error);
			return false;
		}

		content = file.to_string();
	}

	auto [root, error] = yaml_load(content);

	if (!error.empty() || !root)
	{
		cellMusicSelectionContext.error("load_playlist: Failed to load music playlist file '%s':\n%s", path, error);
		return false;
	}

	std::string err;

	const std::string version = get_yaml_node_value<std::string>(root["Version"], err);
	if (!err.empty())
	{
		cellMusicSelectionContext.error("load_playlist: No Version entry found. Error: '%s' (file: '%s')", err, path);
		return false;
	}

	if (version != target_version)
	{
		cellMusicSelectionContext.error("load_playlist: Version '%s' does not match music playlist target '%s' (file: '%s')", version, target_version, path);
		return false;
	}

	const std::string file_type = get_yaml_node_value<std::string>(root["FileType"], err);
	if (!err.empty())
	{
		cellMusicSelectionContext.error("load_playlist: No FileType entry found. Error: '%s' (file: '%s')", err, path);
		return false;
	}

	if (file_type != target_file_type)
	{
		cellMusicSelectionContext.error("load_playlist: FileType '%s' does not match music playlist target '%s' (file: '%s')", file_type, target_file_type, path);
		return false;
	}

	content_type = static_cast<CellSearchContentType>(get_yaml_node_value<u32>(root["ContentType"], err));
	if (!err.empty())
	{
		cellMusicSelectionContext.error("load_playlist: No ContentType entry found. Error: '%s' (file: '%s')", err, path);
		return false;
	}

	context_option = static_cast<CellSearchContextOption>(get_yaml_node_value<u32>(root["ContextOption"], err));
	if (!err.empty())
	{
		cellMusicSelectionContext.error("load_playlist: No ContextOption entry found. Error: '%s' (file: '%s')", err, path);
		return false;
	}

	repeat_mode = static_cast<CellSearchRepeatMode>(get_yaml_node_value<u32>(root["RepeatMode"], err));
	if (!err.empty())
	{
		cellMusicSelectionContext.error("load_playlist: No RepeatMode entry found. Error: '%s' (file: '%s')", err, path);
		return false;
	}

	first_track = get_yaml_node_value<u32>(root["FirstTrack"], err);
	if (!err.empty())
	{
		cellMusicSelectionContext.error("load_playlist: No FirstTrack entry found. Error: '%s' (file: '%s')", err, path);
		return false;
	}

	const YAML::Node& track_node = root["Tracks"];

	if (!track_node || track_node.Type() != YAML::NodeType::Sequence)
	{
		cellMusicSelectionContext.error("load_playlist: No Tracks entry found or Tracks is not a Sequence. (file: '%s')", path);
		return false;
	}

	for (usz i = 0; i < track_node.size(); i++)
	{
		cellMusicSelectionContext.notice("load_playlist: Adding track to playlist: '%s'. (file: '%s')", track_node[i].Scalar(), path);
		playlist.push_back(track_node[i].Scalar());
	}

	cellMusicSelectionContext.notice("load_playlist: Loaded music playlist file '%s' (context: %s)", path, to_string());
	valid = true;
	return true;
}

void music_selection_context::set_track(std::string_view track)
{
	if (track.empty()) return;

	if (playlist.empty())
	{
		cellMusicSelectionContext.error("set_track: No tracks to play... (requested path='%s')", track);
		return;
	}

	for (usz i = 0; i < playlist.size(); i++)
	{
		cellMusicSelectionContext.error("set_track: Comparing track '%s' vs '%s'", track, playlist[i]);
		if (track.ends_with(playlist[i]))
		{
			first_track = current_track = static_cast<u32>(i);
			return;
		}
	}

	cellMusicSelectionContext.error("set_track: Track '%s' not found...", track);
}

u32 music_selection_context::step_track(bool next)
{
	if (playlist.empty())
	{
		cellMusicSelectionContext.error("step_track: No tracks to play...");
		current_track = umax;
		return umax;
	}

	switch (repeat_mode)
	{
	case CELL_SEARCH_REPEATMODE_NONE:
	{
		if (next)
		{
			// Try to play the next track.
			if (++current_track >= playlist.size())
			{
				// We are at the end of the playlist.
				cellMusicSelectionContext.notice("step_track: No more tracks to play in playlist...");
				current_track = umax;
				return umax;
			}
		}
		else
		{
			// Try to play the previous track.
			if (current_track == 0)
			{
				// We are at the start of the playlist.
				cellMusicSelectionContext.notice("step_track: No more tracks to play in playlist...");
				current_track = umax;
				return umax;
			}

			current_track--;
		}
		break;
	}
	case CELL_SEARCH_REPEATMODE_REPEAT1:
	{
		// Keep decoding the same track.
		break;
	}
	case CELL_SEARCH_REPEATMODE_ALL:
	{
		if (next)
		{
			// Play the next track. Start with the first track if we reached the end of the playlist.
			current_track = (current_track + 1) % playlist.size();
		}
		else
		{
			// Play the previous track. Start with the last track if we reached the start of the playlist.
			if (current_track == 0)
			{
				current_track = ::narrow<u32>(playlist.size() - 1);
			}
			else
			{
				current_track--;
			}
		}
		break;
	}
	case CELL_SEARCH_REPEATMODE_NOREPEAT1:
	{
		// We are done. We only wanted to decode a single track.
		cellMusicSelectionContext.notice("step_track: No more tracks to play...");
		current_track = umax;
		return umax;
	}
	default:
	{
		fmt::throw_exception("step_track: Unknown repeat mode %d", static_cast<u32>(repeat_mode));
	}
	}

	if (context_option == CELL_SEARCH_CONTEXTOPTION_SHUFFLE && repeat_mode == CELL_SEARCH_REPEATMODE_ALL && playlist.size() > 1)
	{
		if (next ? current_track == 0 : current_track == (playlist.size() - 1))
		{
			// We reached the first or last track again. Let's shuffle!
			cellMusicSelectionContext.notice("step_track: Shuffling playlist...");
			auto engine = std::default_random_engine{};
			std::shuffle(std::begin(playlist), std::end(playlist), engine);
		}
	}

	return current_track;
}
