#include "stdafx.h"
#include "Emu/VFS.h"
#include "Emu/IdManager.h"
#include "Emu/Cell/PPUModule.h"
#include "cellMusic.h"
#include "cellSysutil.h"
#include <string>

#include "cellSearch.h"
#include "Utilities/StrUtil.h"

#ifdef _MSC_VER
#pragma warning(push, 0)
#else
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wold-style-cast"
#endif
extern "C" {
#include "libavformat/avformat.h"
#include "libavutil/dict.h"
}

LOG_CHANNEL(cellSearch);

template<>
void fmt_class_string<CellSearchError>::format(std::string& out, u64 arg)
{
	format_enum(out, arg, [](auto error)
	{
		switch (error)
		{
			STR_CASE(CELL_SEARCH_CANCELED);
			STR_CASE(CELL_SEARCH_ERROR_PARAM);
			STR_CASE(CELL_SEARCH_ERROR_BUSY);
			STR_CASE(CELL_SEARCH_ERROR_NO_MEMORY);
			STR_CASE(CELL_SEARCH_ERROR_UNKNOWN_MODE);
			STR_CASE(CELL_SEARCH_ERROR_ALREADY_INITIALIZED);
			STR_CASE(CELL_SEARCH_ERROR_NOT_INITIALIZED);
			STR_CASE(CELL_SEARCH_ERROR_FINALIZING);
			STR_CASE(CELL_SEARCH_ERROR_NOT_SUPPORTED_SEARCH);
			STR_CASE(CELL_SEARCH_ERROR_CONTENT_OBSOLETE);
			STR_CASE(CELL_SEARCH_ERROR_CONTENT_NOT_FOUND);
			STR_CASE(CELL_SEARCH_ERROR_NOT_LIST);
			STR_CASE(CELL_SEARCH_ERROR_OUT_OF_RANGE);
			STR_CASE(CELL_SEARCH_ERROR_INVALID_SEARCHID);
			STR_CASE(CELL_SEARCH_ERROR_ALREADY_GOT_RESULT);
			STR_CASE(CELL_SEARCH_ERROR_NOT_SUPPORTED_CONTEXT);
			STR_CASE(CELL_SEARCH_ERROR_INVALID_CONTENTTYPE);
			STR_CASE(CELL_SEARCH_ERROR_DRM);
			STR_CASE(CELL_SEARCH_ERROR_TAG);
			STR_CASE(CELL_SEARCH_ERROR_GENERIC);
		}

		return unknown;
	});
}

namespace {
enum class search_state
{
	not_initialized = 0,
	idle,
	in_progress,
	initializing,
	canceling,
	finalizing,
};

struct search_info
{
	vm::ptr<CellSearchSystemCallback> func;
	vm::ptr<void> userData;

	atomic_t<search_state> state = search_state::not_initialized;

	shared_mutex links_mutex;
	std::unordered_map<std::string, std::string> content_links;
};

struct search_content_t
{
	CellSearchContentType type = CELL_SEARCH_CONTENTTYPE_NONE;
	CellSearchTimeInfo timeInfo;
	CellSearchContentInfoPath infoPath;
	union
	{
		CellSearchMusicInfo music;
		CellSearchPhotoInfo photo;
		CellSearchVideoInfo video;
		CellSearchMusicListInfo music_list;
		CellSearchPhotoListInfo photo_list;
		CellSearchVideoListInfo video_list;
		CellSearchVideoSceneInfo scene;
	} data;
};

using ContentIdType = std::pair<u64, std::shared_ptr<search_content_t>>;
using ContentIdMap = std::unordered_map<u64, std::shared_ptr<search_content_t>>;

struct search_object_t
{
	// TODO: Figured out the correct values to set here
	static const u32 id_base  = 1;
	static const u32 id_step  = 1;
	static const u32 id_count = 1024; // TODO
	static const u32 invalid  = 0xFFFFFFFF;

	std::vector<ContentIdType> content_ids;
};
}
error_code cellSearchInitialize(CellSearchMode mode, u32 container, vm::ptr<CellSearchSystemCallback> func, vm::ptr<void> userData)
{
	cellSearch.warning("cellSearchInitialize(mode=0x%x, container=0x%x, func=*0x%x, userData=*0x%x)", +mode, container, func, userData);

	if (!func)
	{
		return CELL_SEARCH_ERROR_PARAM;
	}

	if (mode != CELL_SEARCH_MODE_NORMAL)
	{
		return CELL_SEARCH_ERROR_UNKNOWN_MODE;
	}

	const auto search = g_fxo->get<search_info>();

	switch (search->state.compare_and_swap(search_state::not_initialized, search_state::initializing))
	{
	case search_state::not_initialized:
		break;
	case search_state::initializing:
		return CELL_SEARCH_ERROR_BUSY;
	case search_state::finalizing:
		return CELL_SEARCH_ERROR_FINALIZING;
	default:
		return CELL_SEARCH_ERROR_ALREADY_INITIALIZED;
	}

	search->func = func;
	search->userData = userData;

	sysutil_register_cb([=](ppu_thread& ppu) -> s32
	{
		search->state.store(search_state::idle);
		func(ppu, CELL_SEARCH_EVENT_INITIALIZE_RESULT, CELL_OK, vm::null, userData);
		return CELL_OK;
	});

	return CELL_OK;
}

error_code cellSearchFinalize()
{
	cellSearch.todo("cellSearchFinalize()");

	const auto search = g_fxo->get<search_info>();

	switch (search->state.compare_and_swap(search_state::idle, search_state::finalizing))
	{
	case search_state::idle:
		break;
	case search_state::not_initialized:
		return CELL_SEARCH_ERROR_NOT_INITIALIZED;
	case search_state::finalizing:
		return CELL_SEARCH_ERROR_FINALIZING;
	case search_state::in_progress:
	case search_state::initializing:
	case search_state::canceling:
		return CELL_SEARCH_ERROR_BUSY;
	default:
		return CELL_SEARCH_ERROR_GENERIC;
	}

	sysutil_register_cb([=](ppu_thread& ppu) -> s32
	{
		{
			std::lock_guard lock(search->links_mutex);
			search->content_links.clear();
		}
		search->state.store(search_state::not_initialized);
		search->func(ppu, CELL_SEARCH_EVENT_FINALIZE_RESULT, CELL_OK, vm::null, search->userData);
		return CELL_OK;
	});

	return CELL_OK;
}

error_code cellSearchStartListSearch(CellSearchListSearchType type, CellSearchSortOrder sortOrder, vm::ptr<CellSearchId> outSearchId)
{
	cellSearch.todo("cellSearchStartListSearch(type=0x%x, sortOrder=0x%x, outSearchId=*0x%x)", +type, +sortOrder, outSearchId);

	if (!outSearchId)
	{
		return CELL_SEARCH_ERROR_PARAM;
	}

	const char* media_dir;

	switch (type)
	{
	case CELL_SEARCH_LISTSEARCHTYPE_MUSIC_ALBUM:
	case CELL_SEARCH_LISTSEARCHTYPE_MUSIC_GENRE:
	case CELL_SEARCH_LISTSEARCHTYPE_MUSIC_ARTIST:
	case CELL_SEARCH_LISTSEARCHTYPE_MUSIC_PLAYLIST:
		media_dir = "music";
		break;
	case CELL_SEARCH_LISTSEARCHTYPE_PHOTO_YEAR:
	case CELL_SEARCH_LISTSEARCHTYPE_PHOTO_MONTH:
	case CELL_SEARCH_LISTSEARCHTYPE_PHOTO_ALBUM:
	case CELL_SEARCH_LISTSEARCHTYPE_PHOTO_PLAYLIST:
		media_dir = "photo";
		break;
	case CELL_SEARCH_LISTSEARCHTYPE_VIDEO_ALBUM:
		media_dir = "video";
		break;
	default:
		return CELL_SEARCH_ERROR_PARAM;
	}

	if (sortOrder != CELL_SEARCH_SORTORDER_ASCENDING && sortOrder != CELL_SEARCH_SORTORDER_DESCENDING)
	{
		return CELL_SEARCH_ERROR_PARAM;
	}

	const auto search = g_fxo->get<search_info>();

	switch (search->state.compare_and_swap(search_state::idle, search_state::in_progress))
	{
	case search_state::idle:
		break;
	case search_state::not_initialized:
	case search_state::initializing:
		return CELL_SEARCH_ERROR_NOT_INITIALIZED;
	case search_state::finalizing:
		return CELL_SEARCH_ERROR_FINALIZING;
	case search_state::in_progress:
	case search_state::canceling:
		return CELL_SEARCH_ERROR_BUSY;
	default:
		return CELL_SEARCH_ERROR_GENERIC;
	}

	const u32 id = *outSearchId = idm::make<search_object_t>();

	sysutil_register_cb([=, content_map = g_fxo->get<ContentIdMap>()](ppu_thread& ppu) -> s32
	{
		auto curr_search = idm::get<search_object_t>(id);
		vm::var<CellSearchResultParam> resultParam;
		resultParam->searchId = id;
		resultParam->resultNum = 0; // Set again later

		std::function<void(const std::string&)> searchInFolder = [&, type](const std::string& vpath)
		{
			// TODO: this is just a workaround. On a real PS3 the playlists seem to be stored in dev_hdd0/mms/db/metadata_db_hdd

			std::vector<fs::dir_entry> dirs_sorted;

			for (auto&& entry : fs::dir(vfs::get(vpath)))
			{
				entry.name = vfs::unescape(entry.name);

				if (entry.is_directory)
				{
					if (entry.name == "." || entry.name == "..")
					{
						continue; // these dirs are not included in the dir list
					}

					dirs_sorted.push_back(entry);
				}
			}

			// clang-format off
			std::sort(dirs_sorted.begin(), dirs_sorted.end(), [&](const fs::dir_entry& a, const fs::dir_entry& b) -> bool
			{
				switch (sortOrder)
				{
				case CELL_SEARCH_SORTORDER_ASCENDING:
					// Order alphabetically ascending
					return a.name < b.name;
				case CELL_SEARCH_SORTORDER_DESCENDING:
					// Order alphabetically descending
					return a.name > b.name;
				default:
				{
					return false;
				}
				}
			});
			// clang-format on

			for (auto&& item : dirs_sorted)
			{
				item.name = vfs::unescape(item.name);

				if (item.name == "." || item.name == "..")
				{
					continue;
				}

				if (!item.is_directory)
				{
					continue;
				}

				const std::string item_path(vpath + "/" + item.name);

				// Count files
				u32 numOfItems = 0;
				for (auto&& file : fs::dir(vfs::get(item_path)))
				{
					file.name = vfs::unescape(file.name);

					if (file.name == "." || file.name == ".." || file.is_directory)
					{
						continue;
					}

					numOfItems++;
				}

				const u64 hash = std::hash<std::string>()(item_path);
				auto found = content_map->find(hash);
				if (found == content_map->end()) // content isn't yet being tracked
				{
					//auto ext_offset = item.name.find_last_of('.'); // used later if no "Title" found

					std::shared_ptr<search_content_t> curr_find = std::make_shared<search_content_t>();
					if (item_path.length() > CELL_SEARCH_PATH_LEN_MAX)
					{
						// TODO: Create mapping which will be resolved to an actual hard link in VFS by cellSearchPrepareFile
						cellSearch.warning("cellSearchStartListSearch(): Directory-Path \"%s\" is too long and will be omitted: %i", item_path, item_path.length());
						continue;
						// std::string link = "/.tmp/" + std::to_string(hash) + item.name.substr(ext_offset);
						// strcpy_trunc(curr_find->infoPath.contentPath, link);

						// std::lock_guard lock(search->links_mutex);
						// search->content_links.emplace(std::move(link), item_path);
					}
					else
					{
						strcpy_trunc(curr_find->infoPath.contentPath, item_path);
					}

					if (item.name.size() > CELL_SEARCH_TITLE_LEN_MAX)
					{
						item.name.resize(CELL_SEARCH_TITLE_LEN_MAX);
					}

					switch (type)
					{
					case CELL_SEARCH_LISTSEARCHTYPE_MUSIC_ALBUM:
					case CELL_SEARCH_LISTSEARCHTYPE_MUSIC_GENRE:
					case CELL_SEARCH_LISTSEARCHTYPE_MUSIC_ARTIST:
					case CELL_SEARCH_LISTSEARCHTYPE_MUSIC_PLAYLIST:
					{
						curr_find->type = CELL_SEARCH_CONTENTTYPE_MUSICLIST;
						CellSearchMusicListInfo& info = curr_find->data.music_list;
						info.listType = type; // CellSearchListType matches CellSearchListSearchType
						info.numOfItems = numOfItems;
						info.duration = 0;
						strcpy_trunc(info.title, item.name);
						strcpy_trunc(info.artistName, "ARTIST NAME");
						break;
					}
					case CELL_SEARCH_LISTSEARCHTYPE_PHOTO_YEAR:
					case CELL_SEARCH_LISTSEARCHTYPE_PHOTO_MONTH:
					case CELL_SEARCH_LISTSEARCHTYPE_PHOTO_ALBUM:
					case CELL_SEARCH_LISTSEARCHTYPE_PHOTO_PLAYLIST:
					{
						curr_find->type = CELL_SEARCH_CONTENTTYPE_PHOTOLIST;
						CellSearchPhotoListInfo& info = curr_find->data.photo_list;
						info.listType = type; // CellSearchListType matches CellSearchListSearchType
						info.numOfItems = numOfItems;
						strcpy_trunc(info.title, item.name);
						break;
					}
					case CELL_SEARCH_LISTSEARCHTYPE_VIDEO_ALBUM:
					{
						curr_find->type = CELL_SEARCH_CONTENTTYPE_VIDEOLIST;
						CellSearchVideoListInfo& info = curr_find->data.video_list;
						info.listType = type; // CellSearchListType matches CellSearchListSearchType
						info.numOfItems = numOfItems;
						info.duration = 0;
						strcpy_trunc(info.title, item.name);
						break;
					}
					default:
					{
						break;
					}
					}

					content_map->emplace(hash, curr_find);
					curr_search->content_ids.emplace_back(hash, curr_find); // place this file's "ID" into the list of found types

					cellSearch.notice("cellSearchStartListSearch(): Content ID: %08X   Path: \"%s\"", hash, item_path);
				}
				else // list is already stored and tracked
				{
					// TODO
					// Perform checks to see if the identified list has been modified since last checked
					// In which case, update the stored content's properties
					// auto content_found = &content_map->at(content_id);
					// curr_search->content_ids.emplace_back(found->first, found->second);
				}
			}
		};

		searchInFolder(fmt::format("/dev_hdd0/%s", media_dir));
		resultParam->resultNum = ::narrow<s32>(curr_search->content_ids.size());

		search->state.store(search_state::idle);
		search->func(ppu, CELL_SEARCH_EVENT_LISTSEARCH_RESULT, CELL_OK, vm::cast(resultParam.addr()), search->userData);
		return CELL_OK;
	});

	return CELL_OK;
}

error_code cellSearchStartContentSearchInList(vm::cptr<CellSearchContentId> listId, CellSearchSortKey sortKey, CellSearchSortOrder sortOrder, vm::ptr<CellSearchId> outSearchId)
{
	cellSearch.todo("cellSearchStartContentSearchInList(listId=*0x%x, sortKey=0x%x, sortOrder=0x%x, outSearchId=*0x%x)", listId, +sortKey, +sortOrder, outSearchId);

	if (!listId || !outSearchId)
	{
		return CELL_SEARCH_ERROR_PARAM;
	}

	switch (sortKey)
	{
	case CELL_SEARCH_SORTKEY_DEFAULT:
	case CELL_SEARCH_SORTKEY_TITLE:
	case CELL_SEARCH_SORTKEY_ALBUMTITLE:
	case CELL_SEARCH_SORTKEY_GENRENAME:
	case CELL_SEARCH_SORTKEY_ARTISTNAME:
	case CELL_SEARCH_SORTKEY_IMPORTEDDATE:
	case CELL_SEARCH_SORTKEY_TRACKNUMBER:
	case CELL_SEARCH_SORTKEY_TAKENDATE:
	case CELL_SEARCH_SORTKEY_USERDEFINED:
	case CELL_SEARCH_SORTKEY_MODIFIEDDATE:
		break;
	default:
		return CELL_SEARCH_ERROR_PARAM;
	}

	if (sortOrder != CELL_SEARCH_SORTORDER_ASCENDING && sortOrder != CELL_SEARCH_SORTORDER_DESCENDING)
	{
		return CELL_SEARCH_ERROR_PARAM;
	}

	const auto search = g_fxo->get<search_info>();

	switch (search->state.compare_and_swap(search_state::idle, search_state::in_progress))
	{
	case search_state::idle:
		break;
	case search_state::not_initialized:
	case search_state::initializing:
		return CELL_SEARCH_ERROR_NOT_INITIALIZED;
	case search_state::finalizing:
		return CELL_SEARCH_ERROR_FINALIZING;
	case search_state::in_progress:
	case search_state::canceling:
		return CELL_SEARCH_ERROR_BUSY;
	default:
		return CELL_SEARCH_ERROR_GENERIC;
	}

	const auto content_map = g_fxo->get<ContentIdMap>();
	auto found = content_map->find(*reinterpret_cast<const u64*>(listId->data));
	if (found == content_map->end())
	{
		// content ID not found, perform a search first
		return CELL_SEARCH_ERROR_CONTENT_NOT_FOUND;
	}

	CellSearchContentSearchType type = CELL_SEARCH_CONTENTSEARCHTYPE_NONE;

	const auto& content_info = found->second;
	switch (content_info->type)
	{
	case CELL_SEARCH_CONTENTTYPE_MUSICLIST:
		type = CELL_SEARCH_CONTENTSEARCHTYPE_MUSIC_ALL;
		break;
	case CELL_SEARCH_CONTENTTYPE_PHOTOLIST:
		type = CELL_SEARCH_CONTENTSEARCHTYPE_PHOTO_ALL;
		break;
	case CELL_SEARCH_CONTENTTYPE_VIDEOLIST:
		type = CELL_SEARCH_CONTENTSEARCHTYPE_VIDEO_ALL;
		break;
	case CELL_SEARCH_CONTENTTYPE_MUSIC:
	case CELL_SEARCH_CONTENTTYPE_PHOTO:
	case CELL_SEARCH_CONTENTTYPE_VIDEO:
	case CELL_SEARCH_CONTENTTYPE_SCENE:
	default:
		return CELL_SEARCH_ERROR_NOT_LIST;
	}

	const u32 id = *outSearchId = idm::make<search_object_t>();

	sysutil_register_cb([=, list_path = std::string(content_info->infoPath.contentPath)](ppu_thread& ppu) -> s32
	{
		auto curr_search = idm::get<search_object_t>(id);
		vm::var<CellSearchResultParam> resultParam;
		resultParam->searchId = id;
		resultParam->resultNum = 0; // Set again later

		std::function<void(const std::string&)> searchInFolder = [&, type](const std::string& vpath)
		{
			std::vector<fs::dir_entry> files_sorted;

			for (auto&& entry : fs::dir(vfs::get(vpath)))
			{
				entry.name = vfs::unescape(entry.name);

				if (entry.is_directory || entry.name == "." || entry.name == "..")
				{
					continue;
				}

				files_sorted.push_back(entry);
			}

			// clang-format off
			std::sort(files_sorted.begin(), files_sorted.end(), [&](const fs::dir_entry& a, const fs::dir_entry& b) -> bool
			{
				switch (sortOrder)
				{
				case CELL_SEARCH_SORTORDER_ASCENDING:
					// Order alphabetically ascending
					return a.name < b.name;
				case CELL_SEARCH_SORTORDER_DESCENDING:
					// Order alphabetically descending
					return a.name > b.name;
				default:
				{
					return false;
				}
				}
			});
			// clang-format on

			// TODO: Use sortKey (CellSearchSortKey) to allow for sorting by category

			for (auto&& item : files_sorted)
			{
				// TODO
				// Perform first check that file is of desired type. For example, don't wanna go
				// identifying "AlbumArt.jpg" as an MP3. Hrm... Postpone this thought. Do games
				// perform their own checks? DIVA ignores anything without the MP3 extension.

				const std::string item_path(vpath + "/" + item.name);

				const u64 hash = std::hash<std::string>()(item_path);
				auto found = content_map->find(hash);
				if (found == content_map->end()) // content isn't yet being tracked
				{
					auto ext_offset = item.name.find_last_of('.'); // used later if no "Title" found

					std::shared_ptr<search_content_t> curr_find = std::make_shared<search_content_t>();
					if (item_path.length() > CELL_SEARCH_PATH_LEN_MAX)
					{
						// Create mapping which will be resolved to an actual hard link in VFS by cellSearchPrepareFile
						std::string link = "/.tmp/" + std::to_string(hash) + item.name.substr(ext_offset);
						strcpy_trunc(curr_find->infoPath.contentPath, link);

						std::lock_guard lock(search->links_mutex);
						search->content_links.emplace(std::move(link), item_path);
					}
					else
					{
						strcpy_trunc(curr_find->infoPath.contentPath, item_path);
					}
					// TODO - curr_find.infoPath.thumbnailPath
					if (type == CELL_SEARCH_CONTENTSEARCHTYPE_MUSIC_ALL)
					{
						curr_find->type = CELL_SEARCH_CONTENTTYPE_MUSIC;
						CellSearchMusicInfo& info = curr_find->data.music;

						// Only print FFMPEG errors, fatals and panics
						av_log_set_level(AV_LOG_ERROR);

						AVDictionary* av_dict_opts = nullptr;
						av_dict_set(&av_dict_opts, "probesize", "96", 0);
						AVFormatContext* av_format_ctx = nullptr;
						av_format_ctx = avformat_alloc_context();

						// Open input file
						if (avformat_open_input(&av_format_ctx, (vfs::get(vpath) + "/" + item.name).c_str(), 0, &av_dict_opts) < 0)
						{
							// Failed to open file
							av_dict_free(&av_dict_opts);
							avformat_free_context(av_format_ctx);
							continue;
						}
						av_dict_free(&av_dict_opts);

						// Find stream information
						if (avformat_find_stream_info(av_format_ctx, 0) < 0)
						{
							// Failed to load stream information
							avformat_free_context(av_format_ctx);
							continue;
						}

						// Derive first audio stream id from avformat context
						int stream_index = -1;
						for (uint i = 0; i < av_format_ctx->nb_streams; i++)
						{
							if (av_format_ctx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_AUDIO)
							{
								stream_index = i;
								break;
							}
						}
						if (stream_index == -1)
						{
							// Failed to find an audio stream
							avformat_free_context(av_format_ctx);
							continue;
						}

						AVStream* stream = av_format_ctx->streams[stream_index];
						AVCodecParameters* codec = stream->codecpar;

						info.bitrate = codec->bit_rate / 1000;  // TODO: Assumption, verify value
						info.quantizationBitrate = codec->bit_rate / 1000;  // TODO: Assumption, verify value
						info.samplingRate = codec->sample_rate;  // TODO: Assumption, verify value
						info.drmEncrypted = 0; // Needs to be 0 or it wont be accepted
						info.duration = av_format_ctx->duration / 1000;  // TODO: Assumption, verify value
						info.releasedYear = 0; // TODO: Use "date" id3 tag for this
						info.size = item.size;
						info.playCount = 0; // we do not track this for now
						info.lastPlayedDate = 0;  // we do not track this for now
						info.importedDate = 0; // we do not track this for now
						info.status = CELL_SEARCH_CONTENTSTATUS_AVAILABLE; // CellSearchContentStatus

						switch (codec->codec_id) // AVCodecID
						{
						case AV_CODEC_ID_MP3:
							info.codec = CELL_SEARCH_CODEC_MP3; // CellSearchCodec
							break;
						case AV_CODEC_ID_AAC:
							info.codec = CELL_SEARCH_CODEC_AAC; // CellSearchCodec
							break;
						case AV_CODEC_ID_AC3:
							info.codec = CELL_SEARCH_CODEC_AC3; // CellSearchCodec
							break;
						case AV_CODEC_ID_WMAV1:
						case AV_CODEC_ID_WMAV2:
							info.codec = CELL_SEARCH_CODEC_WMA; // CellSearchCodec
							break;
						case AV_CODEC_ID_ATRAC3:
							info.codec = CELL_SEARCH_CODEC_AT3; // CellSearchCodec
							break;
						case AV_CODEC_ID_ATRAC3P:
							info.codec = CELL_SEARCH_CODEC_AT3P; // CellSearchCodec
							break;
						default:
							info.codec = CELL_SEARCH_CODEC_UNKNOWN; // CellSearchCodec
							info.status = CELL_SEARCH_CONTENTSTATUS_NOT_SUPPORTED; // CellSearchContentStatus
							break;
						}

						AVDictionaryEntry *tag;
						std::string value;

						info.trackNumber = 0;
						tag = av_dict_get(av_format_ctx->metadata, "track", 0, AV_DICT_IGNORE_SUFFIX);
						if (tag != nullptr)
						{
							std::string tmp(tag->value);
							info.trackNumber = stoi(tmp.substr(0, tmp.find("/")));
						}

						tag = av_dict_get(av_format_ctx->metadata, "album", 0, AV_DICT_IGNORE_SUFFIX);
						if (tag != nullptr)
						{
							value = tag->value;
							if (value.size() > CELL_SEARCH_TITLE_LEN_MAX)
							{
								value.resize(CELL_SEARCH_TITLE_LEN_MAX);
							}
							strcpy_trunc(info.albumTitle, value);
						}
						else
						{
							strcpy_trunc(info.albumTitle, "Unknown Album");
						}

						tag = av_dict_get(av_format_ctx->metadata, "title", 0, AV_DICT_IGNORE_SUFFIX);
						if (tag != nullptr)
						{
							value = tag->value;
							if (value.size() > CELL_SEARCH_TITLE_LEN_MAX)
							{
								value.resize(CELL_SEARCH_TITLE_LEN_MAX);
							}
							strcpy_trunc(info.title, value);
						}
						else
						{
							// Fall back to filename
							value = item.name.substr(0, ext_offset);
							if (value.size() > CELL_SEARCH_TITLE_LEN_MAX)
							{
								value.resize(CELL_SEARCH_TITLE_LEN_MAX);
								strcpy_trunc(info.title, value);
							}
							else
							{
								strcpy_trunc(info.title, value);
							}
						}

						tag = av_dict_get(av_format_ctx->metadata, "artist", 0, AV_DICT_IGNORE_SUFFIX);
						if (tag != nullptr)
						{
							value = tag->value;
							if (value.size() > CELL_SEARCH_TITLE_LEN_MAX)
							{
								value.resize(CELL_SEARCH_TITLE_LEN_MAX);
							}
							strcpy_trunc(info.artistName, value);
						}
						else
						{
							strcpy_trunc(info.artistName, "Unknown Artist");
						}

						tag = av_dict_get(av_format_ctx->metadata, "genre", 0, AV_DICT_IGNORE_SUFFIX);
						if (tag != nullptr)
						{
							value = tag->value;
							if (value.size() > CELL_SEARCH_TITLE_LEN_MAX)
							{
								value.resize(CELL_SEARCH_TITLE_LEN_MAX);
							}
							strcpy_trunc(info.genreName, value);
						}
						else
						{
							strcpy_trunc(info.genreName, "Unknown Genre");
						}

						avformat_close_input(&av_format_ctx);
						avformat_free_context(av_format_ctx);
					}
					else if (type == CELL_SEARCH_CONTENTSEARCHTYPE_PHOTO_ALL)
					{
						curr_find->type = CELL_SEARCH_CONTENTTYPE_PHOTO;
						CellSearchPhotoInfo& info = curr_find->data.photo;
						// TODO - Some kinda file photo analysis and assign the values as such
						info.size = item.size;
						info.importedDate = 0;
						info.takenDate = 0;
						info.width = 0;
						info.height = 0;
						info.orientation = 0;  // CellSearchOrientation
						info.codec = 0;        // CellSearchCodec
						info.status = 0;       // CellSearchContentStatus
						strcpy_trunc(info.title, item.name.substr(0, ext_offset));
						strcpy_trunc(info.albumTitle, "ALBUM TITLE");
					}
					else if (type == CELL_SEARCH_CONTENTSEARCHTYPE_VIDEO_ALL)
					{
						curr_find->type = CELL_SEARCH_CONTENTTYPE_VIDEO;
						CellSearchVideoInfo& info = curr_find->data.video;
						// TODO - Some kinda file video analysis and assign the values as such
						info.duration = 0;
						info.size = item.size;
						info.importedDate = 0;
						info.takenDate = 0;
						info.videoBitrate = 0;
						info.audioBitrate = 0;
						info.playCount = 0;
						info.drmEncrypted = 0;
						info.videoCodec = 0; // CellSearchCodec
						info.audioCodec = 0; // CellSearchCodec
						info.status = 0;     // CellSearchContentStatus
						strcpy_trunc(info.title, item.name.substr(0, ext_offset)); // it'll do for the moment...
						strcpy_trunc(info.albumTitle, "ALBUM TITLE");
					}

					content_map->emplace(hash, curr_find);
					curr_search->content_ids.emplace_back(hash, curr_find); // place this file's "ID" into the list of found types

					cellSearch.notice("cellSearchStartContentSearchInList(): Content ID: %08X   Path: \"%s\"", hash, item_path);
				}
				else // file is already stored and tracked
				{
					// TODO
					// Perform checks to see if the identified file has been modified since last checked
					// In which case, update the stored content's properties
					// auto content_found = &content_map->at(content_id);
					curr_search->content_ids.emplace_back(found->first, found->second);
				}
			}
		};

		searchInFolder(list_path);
		resultParam->resultNum = ::narrow<s32>(curr_search->content_ids.size());

		search->state.store(search_state::idle);
		search->func(ppu, CELL_SEARCH_EVENT_CONTENTSEARCH_INLIST_RESULT, CELL_OK, vm::cast(resultParam.addr()), search->userData);
		return CELL_OK;
	});

	return CELL_OK;
}

error_code cellSearchStartContentSearch(CellSearchContentSearchType type, CellSearchSortKey sortKey, CellSearchSortOrder sortOrder, vm::ptr<CellSearchId> outSearchId)
{
	cellSearch.todo("cellSearchStartContentSearch(type=0x%x, sortKey=0x%x, sortOrder=0x%x, outSearchId=*0x%x)", +type, +sortKey, +sortOrder, outSearchId);

	if (!outSearchId)
	{
		return CELL_SEARCH_ERROR_PARAM;
	}

	switch (sortKey)
	{
	case CELL_SEARCH_SORTKEY_DEFAULT:
	case CELL_SEARCH_SORTKEY_TITLE:
	case CELL_SEARCH_SORTKEY_ALBUMTITLE:
	case CELL_SEARCH_SORTKEY_GENRENAME:
	case CELL_SEARCH_SORTKEY_ARTISTNAME:
	case CELL_SEARCH_SORTKEY_IMPORTEDDATE:
	case CELL_SEARCH_SORTKEY_TRACKNUMBER:
	case CELL_SEARCH_SORTKEY_TAKENDATE:
	case CELL_SEARCH_SORTKEY_USERDEFINED:
	case CELL_SEARCH_SORTKEY_MODIFIEDDATE:
		break;
	default:
		return CELL_SEARCH_ERROR_PARAM;
	}

	if (sortOrder != CELL_SEARCH_SORTORDER_ASCENDING && sortOrder != CELL_SEARCH_SORTORDER_DESCENDING)
	{
		return CELL_SEARCH_ERROR_PARAM;
	}

	const char* media_dir;
	switch (type)
	{
	case CELL_SEARCH_CONTENTSEARCHTYPE_MUSIC_ALL: media_dir = "music"; break;
	case CELL_SEARCH_CONTENTSEARCHTYPE_PHOTO_ALL: media_dir = "photo"; break;
	case CELL_SEARCH_CONTENTSEARCHTYPE_VIDEO_ALL: media_dir = "video"; break;
	default:
		return CELL_SEARCH_ERROR_PARAM;
	}

	const auto search = g_fxo->get<search_info>();

	switch (search->state.compare_and_swap(search_state::idle, search_state::in_progress))
	{
	case search_state::idle:
		break;
	case search_state::not_initialized:
	case search_state::initializing:
		return CELL_SEARCH_ERROR_NOT_INITIALIZED;
	case search_state::finalizing:
		return CELL_SEARCH_ERROR_FINALIZING;
	case search_state::in_progress:
	case search_state::canceling:
		return CELL_SEARCH_ERROR_BUSY;
	default:
		return CELL_SEARCH_ERROR_GENERIC;
	}

	const u32 id = *outSearchId = idm::make<search_object_t>();

	sysutil_register_cb([=, content_map = g_fxo->get<ContentIdMap>()](ppu_thread& ppu) -> s32
	{
		auto curr_search = idm::get<search_object_t>(id);
		vm::var<CellSearchResultParam> resultParam;
		resultParam->searchId = id;
		resultParam->resultNum = 0; // Set again later

		std::function<void(const std::string&, const std::string&)> searchInFolder = [&, type](const std::string& vpath, const std::string& prev)
		{
			const std::string relative_vpath = (!prev.empty() ? prev + "/" : "") + vpath;

			for (auto&& item : fs::dir(vfs::get(relative_vpath)))
			{
				item.name = vfs::unescape(item.name);

				if (item.name == "." || item.name == "..")
				{
					continue;
				}

				if (item.is_directory)
				{
					searchInFolder(item.name, relative_vpath);
					continue;
				}

				// TODO
				// Perform first check that file is of desired type. For example, don't wanna go
				// identifying "AlbumArt.jpg" as an MP3. Hrm... Postpone this thought. Do games
				// perform their own checks? DIVA ignores anything without the MP3 extension.

				// TODO - Identify sorting method and insert the appropriate values where applicable
				const std::string item_path(relative_vpath + "/" + item.name);

				const u64 hash = std::hash<std::string>()(item_path);
				auto found = content_map->find(hash);
				if (found == content_map->end()) // content isn't yet being tracked
				{
					auto ext_offset = item.name.find_last_of('.'); // used later if no "Title" found

					std::shared_ptr<search_content_t> curr_find = std::make_shared<search_content_t>();
					if( item_path.length() > CELL_SEARCH_PATH_LEN_MAX )
					{
						// Create mapping which will be resolved to an actual hard link in VFS by cellSearchPrepareFile
						std::string link = "/.tmp/" + std::to_string(hash) + item.name.substr(ext_offset);
						strcpy_trunc(curr_find->infoPath.contentPath, link);

						std::lock_guard lock(search->links_mutex);
						search->content_links.emplace(std::move(link), item_path);
					}
					else
					{
						strcpy_trunc(curr_find->infoPath.contentPath, item_path);
					}
					// TODO - curr_find.infoPath.thumbnailPath
					if (type == CELL_SEARCH_CONTENTSEARCHTYPE_MUSIC_ALL)
					{
						curr_find->type = CELL_SEARCH_CONTENTTYPE_MUSIC;
						CellSearchMusicInfo& info = curr_find->data.music;
						// TODO - Some kinda file music analysis and assign the values as such
						info.duration = 0;
						info.size = item.size;
						info.importedDate = 0;
						info.lastPlayedDate = 0;
						info.releasedYear = 0;
						info.trackNumber = 0;
						info.bitrate = 0;
						info.samplingRate = 0;
						info.quantizationBitrate = 0;
						info.playCount = 0;
						info.drmEncrypted = 0;
						info.codec = 0;  // CellSearchCodec
						info.status = 0; // CellSearchContentStatus
						strcpy_trunc(info.title, item.name.substr(0, ext_offset)); // it'll do for the moment...
						strcpy_trunc(info.albumTitle, "ALBUM TITLE");
						strcpy_trunc(info.artistName, "ARTIST NAME");
						strcpy_trunc(info.genreName, "GENRE NAME");
					}
					else if (type == CELL_SEARCH_CONTENTSEARCHTYPE_PHOTO_ALL)
					{
						curr_find->type = CELL_SEARCH_CONTENTTYPE_PHOTO;
						CellSearchPhotoInfo& info = curr_find->data.photo;
						// TODO - Some kinda file photo analysis and assign the values as such
						info.size = item.size;
						info.importedDate = 0;
						info.takenDate = 0;
						info.width = 0;
						info.height = 0;
						info.orientation = 0;  // CellSearchOrientation
						info.codec = 0;        // CellSearchCodec
						info.status = 0;       // CellSearchContentStatus
						strcpy_trunc(info.title, item.name.substr(0, ext_offset));
						strcpy_trunc(info.albumTitle, "ALBUM TITLE");
					}
					else if (type == CELL_SEARCH_CONTENTSEARCHTYPE_VIDEO_ALL)
					{
						curr_find->type = CELL_SEARCH_CONTENTTYPE_VIDEO;
						CellSearchVideoInfo& info = curr_find->data.video;
						// TODO - Some kinda file video analysis and assign the values as such
						info.duration = 0;
						info.size = item.size;
						info.importedDate = 0;
						info.takenDate = 0;
						info.videoBitrate = 0;
						info.audioBitrate = 0;
						info.playCount = 0;
						info.drmEncrypted = 0;
						info.videoCodec = 0; // CellSearchCodec
						info.audioCodec = 0; // CellSearchCodec
						info.status = 0;     // CellSearchContentStatus
						strcpy_trunc(info.title, item.name.substr(0, ext_offset)); // it'll do for the moment...
						strcpy_trunc(info.albumTitle, "ALBUM TITLE");
					}

					content_map->emplace(hash, curr_find);
					curr_search->content_ids.emplace_back(hash, curr_find); // place this file's "ID" into the list of found types

					cellSearch.notice("cellSearchStartContentSearch(): Content ID: %08X   Path: \"%s\"", hash, item_path);
				}
				else // file is already stored and tracked
				{ // TODO
					// Perform checks to see if the identified file has been modified since last checked
					// In which case, update the stored content's properties
					// auto content_found = &content_map->at(content_id);
					curr_search->content_ids.emplace_back(found->first, found->second);
				}
			}
		};

		searchInFolder(fmt::format("/dev_hdd0/%s", media_dir), "");
		resultParam->resultNum = ::narrow<s32>(curr_search->content_ids.size());

		search->state.store(search_state::idle);
		search->func(ppu, CELL_SEARCH_EVENT_CONTENTSEARCH_RESULT, CELL_OK, vm::cast(resultParam.addr()), search->userData);
		return CELL_OK;
	});

	return CELL_OK;
}

error_code cellSearchStartSceneSearchInVideo(vm::cptr<CellSearchContentId> videoId, CellSearchSceneSearchType searchType, CellSearchSortOrder sortOrder, vm::ptr<CellSearchId> outSearchId)
{
	cellSearch.todo("cellSearchStartSceneSearchInVideo(videoId=*0x%x, searchType=0x%x, sortOrder=0x%x, outSearchId=*0x%x)", videoId, +searchType, +sortOrder, outSearchId);

	if (!videoId || !outSearchId)
	{
		return CELL_SEARCH_ERROR_PARAM;
	}

	switch (searchType)
	{
	case CELL_SEARCH_SCENESEARCHTYPE_NONE:
	case CELL_SEARCH_SCENESEARCHTYPE_CHAPTER:
	case CELL_SEARCH_SCENESEARCHTYPE_CLIP_HIGHLIGHT:
	case CELL_SEARCH_SCENESEARCHTYPE_CLIP_USER:
	case CELL_SEARCH_SCENESEARCHTYPE_CLIP:
	case CELL_SEARCH_SCENESEARCHTYPE_ALL:
		break;
	default:
		return CELL_SEARCH_ERROR_PARAM;
	}

	if (sortOrder != CELL_SEARCH_SORTORDER_ASCENDING && sortOrder != CELL_SEARCH_SORTORDER_DESCENDING)
	{
		return CELL_SEARCH_ERROR_PARAM;
	}

	const auto search = g_fxo->get<search_info>();

	switch (search->state.compare_and_swap(search_state::idle, search_state::in_progress))
	{
	case search_state::idle:
		break;
	case search_state::not_initialized:
	case search_state::initializing:
		return CELL_SEARCH_ERROR_NOT_INITIALIZED;
	case search_state::finalizing:
		return CELL_SEARCH_ERROR_FINALIZING;
	case search_state::in_progress:
	case search_state::canceling:
		return CELL_SEARCH_ERROR_BUSY;
	default:
		return CELL_SEARCH_ERROR_GENERIC;
	}

	const u32 id = *outSearchId = idm::make<search_object_t>();

	sysutil_register_cb([=](ppu_thread& ppu) -> s32
	{
		vm::var<CellSearchResultParam> resultParam;
		resultParam->searchId = id;
		resultParam->resultNum = 0; // TODO

		search->state.store(search_state::idle);
		search->func(ppu, CELL_SEARCH_EVENT_SCENESEARCH_INVIDEO_RESULT, CELL_OK, vm::cast(resultParam.addr()), search->userData);
		return CELL_OK;
	});

	return CELL_OK;
}

error_code cellSearchStartSceneSearch(CellSearchSceneSearchType searchType, vm::cptr<char> gameTitle, vm::cpptr<char> tags, u32 tagNum, CellSearchSortKey sortKey, CellSearchSortOrder sortOrder, vm::ptr<CellSearchId> outSearchId)
{
	cellSearch.todo("cellSearchStartSceneSearch(searchType=0x%x, gameTitle=%s, tags=**0x%x, tagNum=0x%x, sortKey=0x%x, sortOrder=0x%x, outSearchId=*0x%x)", +searchType, gameTitle, tags, tagNum, +sortKey, +sortOrder, outSearchId);

	if (!gameTitle || !outSearchId)
	{
		return CELL_SEARCH_ERROR_PARAM;
	}

	switch (searchType)
	{
	case CELL_SEARCH_SCENESEARCHTYPE_NONE:
	case CELL_SEARCH_SCENESEARCHTYPE_CHAPTER:
	case CELL_SEARCH_SCENESEARCHTYPE_CLIP_HIGHLIGHT:
	case CELL_SEARCH_SCENESEARCHTYPE_CLIP_USER:
	case CELL_SEARCH_SCENESEARCHTYPE_CLIP:
	case CELL_SEARCH_SCENESEARCHTYPE_ALL:
		break;
	default:
		return CELL_SEARCH_ERROR_PARAM;
	}

	const auto search = g_fxo->get<search_info>();

	switch (search->state.compare_and_swap(search_state::idle, search_state::in_progress))
	{
	case search_state::idle:
		break;
	case search_state::not_initialized:
	case search_state::initializing:
		return CELL_SEARCH_ERROR_NOT_INITIALIZED;
	case search_state::finalizing:
		return CELL_SEARCH_ERROR_FINALIZING;
	case search_state::in_progress:
	case search_state::canceling:
		return CELL_SEARCH_ERROR_BUSY;
	default:
		return CELL_SEARCH_ERROR_GENERIC;
	}

	const u32 id = *outSearchId = idm::make<search_object_t>();

	sysutil_register_cb([=](ppu_thread& ppu) -> s32
	{
		vm::var<CellSearchResultParam> resultParam;
		resultParam->searchId = id;
		resultParam->resultNum = 0; // TODO

		search->state.store(search_state::idle);
		search->func(ppu, CELL_SEARCH_EVENT_SCENESEARCH_RESULT, CELL_OK, vm::cast(resultParam.addr()), search->userData);
		return CELL_OK;
	});

	return CELL_OK;
}

error_code cellSearchGetContentInfoByOffset(CellSearchId searchId, s32 offset, vm::ptr<void> infoBuffer, vm::ptr<CellSearchContentType> outContentType, vm::ptr<CellSearchContentId> outContentId)
{
	cellSearch.warning("cellSearchGetContentInfoByOffset(searchId=0x%x, offset=0x%x, infoBuffer=*0x%x, outContentType=*0x%x, outContentId=*0x%x)", searchId, offset, infoBuffer, outContentType, outContentId);

	if (!outContentType)
	{
		return CELL_SEARCH_ERROR_PARAM;
	}

	const auto searchObject = idm::get<search_object_t>(searchId);

	if (!searchObject)
	{
		return CELL_SEARCH_ERROR_INVALID_SEARCHID;
	}

	switch (g_fxo->get<search_info>()->state.load())
	{
	case search_state::idle:
		break;
	case search_state::not_initialized:
	case search_state::initializing:
		return CELL_SEARCH_ERROR_NOT_INITIALIZED;
	case search_state::finalizing:
		return CELL_SEARCH_ERROR_FINALIZING;
	case search_state::in_progress:
	case search_state::canceling:
		return CELL_SEARCH_ERROR_BUSY;
	default:
		return CELL_SEARCH_ERROR_GENERIC;
	}

	if (offset >= 0 && offset + 0u < searchObject->content_ids.size())
	{
		const auto& content_id = searchObject->content_ids[offset];
		const auto& content_info = content_id.second;
		switch (content_info->type)
		{
		case CELL_SEARCH_CONTENTTYPE_MUSIC:
			std::memcpy(infoBuffer.get_ptr(), &content_info->data.music, sizeof(content_info->data.music));
			break;
		case CELL_SEARCH_CONTENTTYPE_PHOTO:
			std::memcpy(infoBuffer.get_ptr(), &content_info->data.photo, sizeof(content_info->data.photo));
			break;
		case CELL_SEARCH_CONTENTTYPE_VIDEO:
			std::memcpy(infoBuffer.get_ptr(), &content_info->data.video, sizeof(content_info->data.photo));
			break;
		case CELL_SEARCH_CONTENTTYPE_MUSICLIST:
			std::memcpy(infoBuffer.get_ptr(), &content_info->data.music_list, sizeof(content_info->data.music_list));
			break;
		case CELL_SEARCH_CONTENTTYPE_PHOTOLIST:
			std::memcpy(infoBuffer.get_ptr(), &content_info->data.photo_list, sizeof(content_info->data.photo_list));
			break;
		case CELL_SEARCH_CONTENTTYPE_VIDEOLIST:
			std::memcpy(infoBuffer.get_ptr(), &content_info->data.video_list, sizeof(content_info->data.video_list));
			break;
		case CELL_SEARCH_CONTENTTYPE_SCENE:
			std::memcpy(infoBuffer.get_ptr(), &content_info->data.scene, sizeof(content_info->data.scene));
			break;
		default:
			return CELL_SEARCH_ERROR_GENERIC;
		}

		const u128 content_id_128 = content_id.first;
		*outContentType = content_info->type;
		std::memcpy(outContentId->data, &content_id_128, CELL_SEARCH_CONTENT_ID_SIZE);
	}
	else // content ID not found, perform a search first
	{
		return CELL_SEARCH_ERROR_OUT_OF_RANGE;
	}

	return CELL_OK;
}

error_code cellSearchGetContentInfoByContentId(vm::cptr<CellSearchContentId> contentId, vm::ptr<void> infoBuffer, vm::ptr<CellSearchContentType> outContentType)
{
	cellSearch.warning("cellSearchGetContentInfoByContentId(contentId=*0x%x, infoBuffer=*0x%x, outContentType=*0x%x)", contentId, infoBuffer, outContentType);

	if (!outContentType)
	{
		return CELL_SEARCH_ERROR_PARAM;
	}

	switch (g_fxo->get<search_info>()->state.load())
	{
	case search_state::idle:
		break;
	case search_state::not_initialized:
	case search_state::initializing:
		return CELL_SEARCH_ERROR_NOT_INITIALIZED;
	case search_state::finalizing:
		return CELL_SEARCH_ERROR_FINALIZING;
	case search_state::in_progress:
	case search_state::canceling:
		return CELL_SEARCH_ERROR_BUSY;
	default:
		return CELL_SEARCH_ERROR_GENERIC;
	}

	const auto content_map = g_fxo->get<ContentIdMap>();
	auto found = content_map->find(*reinterpret_cast<const u64*>(contentId->data));
	if (found != content_map->end())
	{
		const auto& content_info = found->second;
		switch (content_info->type)
		{
		case CELL_SEARCH_CONTENTTYPE_MUSIC:
			std::memcpy(infoBuffer.get_ptr(), &content_info->data.music, sizeof(content_info->data.music));
			break;
		case CELL_SEARCH_CONTENTTYPE_PHOTO:
			std::memcpy(infoBuffer.get_ptr(), &content_info->data.photo, sizeof(content_info->data.photo));
			break;
		case CELL_SEARCH_CONTENTTYPE_VIDEO:
			std::memcpy(infoBuffer.get_ptr(), &content_info->data.video, sizeof(content_info->data.photo));
			break;
		case CELL_SEARCH_CONTENTTYPE_MUSICLIST:
			std::memcpy(infoBuffer.get_ptr(), &content_info->data.music_list, sizeof(content_info->data.music_list));
			break;
		case CELL_SEARCH_CONTENTTYPE_PHOTOLIST:
			std::memcpy(infoBuffer.get_ptr(), &content_info->data.photo_list, sizeof(content_info->data.photo_list));
			break;
		case CELL_SEARCH_CONTENTTYPE_VIDEOLIST:
			std::memcpy(infoBuffer.get_ptr(), &content_info->data.video_list, sizeof(content_info->data.video_list));
			break;
		case CELL_SEARCH_CONTENTTYPE_SCENE:
			std::memcpy(infoBuffer.get_ptr(), &content_info->data.scene, sizeof(content_info->data.scene));
			break;
		default:
			return CELL_SEARCH_ERROR_GENERIC;
		}

		*outContentType = content_info->type;
	}
	else // content ID not found, perform a search first
	{
		return CELL_SEARCH_ERROR_CONTENT_NOT_FOUND;
	}

	return CELL_OK;
}

error_code cellSearchGetOffsetByContentId(CellSearchId searchId, vm::cptr<CellSearchContentId> contentId, vm::ptr<s32> outOffset)
{
	cellSearch.warning("cellSearchGetOffsetByContentId(searchId=0x%x, contentId=*0x%x, outOffset=*0x%x)", searchId, contentId, outOffset);

	if (!outOffset)
	{
		return CELL_SEARCH_ERROR_PARAM;
	}

	switch (g_fxo->get<search_info>()->state.load())
	{
	case search_state::idle:
		break;
	case search_state::not_initialized:
	case search_state::initializing:
		return CELL_SEARCH_ERROR_NOT_INITIALIZED;
	case search_state::finalizing:
		return CELL_SEARCH_ERROR_FINALIZING;
	case search_state::in_progress:
	case search_state::canceling:
		return CELL_SEARCH_ERROR_BUSY;
	default:
		return CELL_SEARCH_ERROR_GENERIC;
	}

	const auto searchObject = idm::get<search_object_t>(searchId);

	if (!searchObject)
	{
		return CELL_SEARCH_ERROR_INVALID_SEARCHID;
	}

	s32 i = 0;
	const u64 content_hash = *reinterpret_cast<const u64*>(contentId->data);
	for (auto& content_id : searchObject->content_ids)
	{
		if (content_id.first == content_hash)
		{
			*outOffset = i;
			return CELL_OK;
		}
		++i;
	}

	return CELL_SEARCH_ERROR_CONTENT_NOT_FOUND;
}

error_code cellSearchGetContentIdByOffset(CellSearchId searchId, s32 offset, vm::ptr<CellSearchContentType> outContentType, vm::ptr<CellSearchContentId> outContentId, vm::ptr<CellSearchTimeInfo> outTimeInfo)
{
	cellSearch.todo("cellSearchGetContentIdByOffset(searchId=0x%x, offset=0x%x, outContentType=*0x%x, outContentId=*0x%x, outTimeInfo=*0x%x)", searchId, offset, outContentType, outContentId, outTimeInfo);

	if (!outContentType || !outContentId)
	{
		return CELL_SEARCH_ERROR_PARAM;
	}

	const auto searchObject = idm::get<search_object_t>(searchId);

	if (!searchObject)
	{
		return CELL_SEARCH_ERROR_INVALID_SEARCHID;
	}

	switch (g_fxo->get<search_info>()->state.load())
	{
	case search_state::idle:
		break;
	case search_state::not_initialized:
	case search_state::initializing:
		return CELL_SEARCH_ERROR_NOT_INITIALIZED;
	case search_state::finalizing:
		return CELL_SEARCH_ERROR_FINALIZING;
	case search_state::in_progress:
	case search_state::canceling:
		return CELL_SEARCH_ERROR_BUSY;
	default:
		return CELL_SEARCH_ERROR_GENERIC;
	}

	if (offset >= 0 && offset + 0u < searchObject->content_ids.size())
	{
		auto& content_id = searchObject->content_ids.at(offset);
		const u128 content_id_128 = content_id.first;
		*outContentType = content_id.second->type;
		std::memcpy(outContentId->data, &content_id_128, CELL_SEARCH_CONTENT_ID_SIZE);

		if (outTimeInfo)
		{
			std::memcpy(outTimeInfo.get_ptr(), &content_id.second->timeInfo, sizeof(content_id.second->timeInfo));
		}

	}
	else // content ID not found, perform a search first
	{
		return CELL_SEARCH_ERROR_OUT_OF_RANGE;
	}

	return CELL_OK;
}

error_code cellSearchGetContentInfoGameComment(vm::cptr<CellSearchContentId> contentId, vm::ptr<char> gameComment)
{
	cellSearch.todo("cellSearchGetContentInfoGameComment(contentId=*0x%x, gameComment=*0x%x)", contentId, gameComment);

	if (!gameComment)
	{
		return CELL_SEARCH_ERROR_PARAM;
	}

	return CELL_OK;
}

error_code cellSearchGetMusicSelectionContext(CellSearchId searchId, vm::cptr<CellSearchContentId> contentId, CellSearchRepeatMode repeatMode, CellSearchContextOption option, vm::ptr<CellMusicSelectionContext> outContext)
{
	cellSearch.todo("cellSearchGetMusicSelectionContext(searchId=0x%x, contentId=*0x%x, repeatMode=0x%x, option=0x%x, outContext=*0x%x)", searchId, contentId, +repeatMode, +option, outContext);

	if (!outContext)
	{
		return CELL_SEARCH_ERROR_PARAM;
	}

	const auto searchObject = idm::get<search_object_t>(searchId);

	if (!searchObject)
	{
		return CELL_SEARCH_ERROR_INVALID_SEARCHID;
	}

	return CELL_OK;
}

error_code cellSearchGetMusicSelectionContextOfSingleTrack(vm::cptr<CellSearchContentId> contentId, vm::ptr<CellMusicSelectionContext> outContext)
{
	cellSearch.todo("cellSearchGetMusicSelectionContextOfSingleTrack(contentId=*0x%x, outContext=*0x%x)", contentId, outContext);

	if (!contentId || !outContext)
	{
		return CELL_SEARCH_ERROR_PARAM;
	}

	return CELL_OK;
}

error_code cellSearchGetContentInfoPath(vm::cptr<CellSearchContentId> contentId, vm::ptr<CellSearchContentInfoPath> infoPath)
{
	cellSearch.todo("cellSearchGetContentInfoPath(contentId=*0x%x, infoPath=*0x%x)", contentId, infoPath);

	if (!infoPath)
	{
		return CELL_SEARCH_ERROR_PARAM;
	}

	switch (g_fxo->get<search_info>()->state.load())
	{
	case search_state::idle:
		break;
	case search_state::not_initialized:
	case search_state::initializing:
		return CELL_SEARCH_ERROR_NOT_INITIALIZED;
	case search_state::finalizing:
		return CELL_SEARCH_ERROR_FINALIZING;
	case search_state::in_progress:
	case search_state::canceling:
		return CELL_SEARCH_ERROR_BUSY;
	default:
		return CELL_SEARCH_ERROR_GENERIC;
	}

	const u64 id = *reinterpret_cast<const u64*>(contentId->data);
	const auto content_map  = g_fxo->get<ContentIdMap>();
	auto found = content_map->find(id);
	if(found != content_map->end())
	{
		std::memcpy(infoPath.get_ptr(), &found->second->infoPath, sizeof(found->second->infoPath));
	}
	else
	{
		cellSearch.error("cellSearchGetContentInfoPath(): ID not found : 0x%08X", id);
		return CELL_SEARCH_ERROR_CONTENT_NOT_FOUND;
	}

	cellSearch.success("contentId = %08X  contentPath = \"%s\"", id, infoPath->contentPath);

	return CELL_OK;
}

error_code cellSearchGetContentInfoPathMovieThumb(vm::cptr<CellSearchContentId> contentId, vm::ptr<CellSearchContentInfoPathMovieThumb> infoMt)
{
	cellSearch.todo("cellSearchGetContentInfoPathMovieThumb(contentId=*0x%x, infoMt=*0x%x)", contentId, infoMt);

	if (!infoMt)
	{
		return CELL_SEARCH_ERROR_PARAM;
	}

	return CELL_OK;
}

error_code cellSearchPrepareFile(vm::cptr<char> path)
{
	cellSearch.todo("cellSearchPrepareFile(path=%s)", path);

	if (!path)
	{
		return CELL_SEARCH_ERROR_PARAM;
	}

	const auto search = g_fxo->get<search_info>();
	switch (search->state.load())
	{
	case search_state::idle:
		break;
	case search_state::not_initialized:
	case search_state::initializing:
		return CELL_SEARCH_ERROR_NOT_INITIALIZED;
	case search_state::finalizing:
		return CELL_SEARCH_ERROR_FINALIZING;
	case search_state::in_progress:
	case search_state::canceling:
		return CELL_SEARCH_ERROR_BUSY;
	default:
		return CELL_SEARCH_ERROR_GENERIC;
	}

	reader_lock lock(search->links_mutex);
	auto found = search->content_links.find(path.get_ptr());
	if (found != search->content_links.end())
	{
		vfs::mount(found->first, vfs::get(found->second));
	}

	return CELL_OK;
}

error_code cellSearchGetContentInfoDeveloperData(vm::cptr<CellSearchContentId> contentId, vm::ptr<char> developerData)
{
	cellSearch.todo("cellSearchGetContentInfoDeveloperData(contentId=*0x%x, developerData=*0x%x)", contentId, developerData);

	if (!contentId || !developerData)
	{
		return CELL_SEARCH_ERROR_PARAM;
	}

	return CELL_OK;
}

error_code cellSearchGetContentInfoSharable(vm::cptr<CellSearchContentId> contentId, vm::ptr<CellSearchSharableType> sharable)
{
	cellSearch.todo("cellSearchGetContentInfoSharable(contentId=*0x%x, sharable=*0x%x)", contentId, sharable);

	if (!contentId || !sharable)
	{
		return CELL_SEARCH_ERROR_PARAM;
	}

	return CELL_OK;
}

error_code cellSearchCancel(CellSearchId searchId)
{
	cellSearch.todo("cellSearchCancel(searchId=0x%x)", searchId);

	const auto searchObject = idm::get<search_object_t>(searchId);

	if (!searchObject)
	{
		return CELL_SEARCH_ERROR_INVALID_SEARCHID;
	}

	switch (g_fxo->get<search_info>()->state.load())
	{
	case search_state::in_progress:
		break;
	case search_state::not_initialized:
	case search_state::initializing:
		return CELL_SEARCH_ERROR_NOT_INITIALIZED;
	case search_state::finalizing:
		return CELL_SEARCH_ERROR_FINALIZING;
	case search_state::canceling:
		return CELL_SEARCH_ERROR_BUSY;
	case search_state::idle:
		return CELL_SEARCH_ERROR_ALREADY_GOT_RESULT;
	default:
		return CELL_SEARCH_ERROR_GENERIC;
	}

	// TODO

	return CELL_OK;
}

error_code cellSearchEnd(CellSearchId searchId)
{
	cellSearch.todo("cellSearchEnd(searchId=0x%x)", searchId);

	switch (g_fxo->get<search_info>()->state.load())
	{
	case search_state::idle:
		break;
	case search_state::not_initialized:
		return CELL_SEARCH_ERROR_NOT_INITIALIZED;
	case search_state::finalizing:
		return CELL_SEARCH_ERROR_FINALIZING;
	case search_state::in_progress:
	case search_state::initializing:
	case search_state::canceling:
		return CELL_SEARCH_ERROR_BUSY;
	default:
		return CELL_SEARCH_ERROR_GENERIC;
	}

	const auto searchObject = idm::get<search_object_t>(searchId);

	if (!searchObject)
	{
		return CELL_SEARCH_ERROR_INVALID_SEARCHID;
	}

	idm::remove<search_object_t>(searchId);

	return CELL_OK;
}

DECLARE(ppu_module_manager::cellSearch)("cellSearchUtility", []()
{
	REG_FUNC(cellSearchUtility, cellSearchInitialize);
	REG_FUNC(cellSearchUtility, cellSearchFinalize);
	REG_FUNC(cellSearchUtility, cellSearchStartListSearch);
	REG_FUNC(cellSearchUtility, cellSearchStartContentSearchInList);
	REG_FUNC(cellSearchUtility, cellSearchStartContentSearch);
	REG_FUNC(cellSearchUtility, cellSearchStartSceneSearchInVideo);
	REG_FUNC(cellSearchUtility, cellSearchStartSceneSearch);
	REG_FUNC(cellSearchUtility, cellSearchGetContentInfoByOffset);
	REG_FUNC(cellSearchUtility, cellSearchGetContentInfoByContentId);
	REG_FUNC(cellSearchUtility, cellSearchGetOffsetByContentId);
	REG_FUNC(cellSearchUtility, cellSearchGetContentIdByOffset);
	REG_FUNC(cellSearchUtility, cellSearchGetContentInfoGameComment);
	REG_FUNC(cellSearchUtility, cellSearchGetMusicSelectionContext);
	REG_FUNC(cellSearchUtility, cellSearchGetMusicSelectionContextOfSingleTrack);
	REG_FUNC(cellSearchUtility, cellSearchGetContentInfoPath);
	REG_FUNC(cellSearchUtility, cellSearchGetContentInfoPathMovieThumb);
	REG_FUNC(cellSearchUtility, cellSearchPrepareFile);
	REG_FUNC(cellSearchUtility, cellSearchGetContentInfoDeveloperData);
	REG_FUNC(cellSearchUtility, cellSearchGetContentInfoSharable);
	REG_FUNC(cellSearchUtility, cellSearchCancel);
	REG_FUNC(cellSearchUtility, cellSearchEnd);
});
