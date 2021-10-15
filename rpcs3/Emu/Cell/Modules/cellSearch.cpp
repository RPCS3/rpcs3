#include "stdafx.h"
#include "Emu/VFS.h"
#include "Emu/IdManager.h"
#include "Emu/Cell/PPUModule.h"
#include "cellMusic.h"
#include "cellSysutil.h"
#include <string>

#include "cellSearch.h"
#include "Utilities/StrUtil.h"
#include "util/media_utils.h"

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

using content_id_type = std::pair<u64, std::shared_ptr<search_content_t>>;

struct content_id_map
{
	std::unordered_map<u64, std::shared_ptr<search_content_t>> map;

	shared_mutex mutex;
};

struct search_object_t
{
	// TODO: Figured out the correct values to set here
	static const u32 id_base  = 1;
	static const u32 id_step  = 1;
	static const u32 id_count = 1024; // TODO

	std::vector<content_id_type> content_ids;
};

template <typename D>
void parse_metadata(D& dst, const utils::media_info& mi, const std::string& key, const std::string& def, usz max_length)
{
	std::string value = mi.get_metadata<std::string>(key, def);
	if (value.size() > max_length)
	{
		value.resize(max_length);
	}
	strcpy_trunc(dst, value);
};

error_code check_search_state(search_state state, search_state action)
{
	switch (action)
	{
	case search_state::initializing:
		switch (state)
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
		break;
	case search_state::finalizing:
		switch (state)
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
		break;
	case search_state::canceling:
		switch (state)
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
		break;
	case search_state::in_progress:
	default:
		switch (state)
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
		break;
	}

	return CELL_OK;
}

void populate_music_info(CellSearchMusicInfo& info, const utils::media_info& mi, const fs::dir_entry& item)
{
	parse_metadata(info.artistName, mi, "artist", "Unknown Artist", CELL_SEARCH_TITLE_LEN_MAX);
	parse_metadata(info.albumTitle, mi, "album", "Unknown Album", CELL_SEARCH_TITLE_LEN_MAX);
	parse_metadata(info.genreName, mi, "genre", "Unknown Genre", CELL_SEARCH_TITLE_LEN_MAX);
	parse_metadata(info.title, mi, "title", item.name.substr(0, item.name.find_last_of('.')), CELL_SEARCH_TITLE_LEN_MAX);
	parse_metadata(info.diskNumber, mi, "disc", "1/1", sizeof(info.diskNumber) - 1);

	// Special case: track is usually stored as e.g. 2/11
	const std::string tmp = mi.get_metadata<std::string>("track", ""s);
	s64 value{};
	const bool success = try_to_int64(&value, tmp.substr(0, tmp.find('/')).c_str(), s32{smin}, s32{smax});
	info.trackNumber = success ? static_cast<s32>(value) : -1;

	info.size = item.size;
	info.releasedYear = static_cast<s32>(mi.get_metadata<s64>("date", -1));
	info.duration = mi.duration_us / 1000; // we need microseconds
	info.samplingRate = mi.sample_rate;
	info.bitrate = mi.audio_bitrate_bps;
	info.quantizationBitrate = mi.audio_bitrate_bps; // TODO: Assumption, verify value
	info.playCount = 0; // we do not track this for now
	info.lastPlayedDate = -1; // we do not track this for now
	info.importedDate = -1; // we do not track this for now
	info.drmEncrypted = 0; // TODO: Needs to be 1 if it's encrypted
	info.status = CELL_SEARCH_CONTENTSTATUS_AVAILABLE;

	// Convert AVCodecID to CellSearchCodec
	switch (mi.audio_av_codec_id)
	{
	case 86017: // AV_CODEC_ID_MP3
		info.codec = CELL_SEARCH_CODEC_MP3;
		break;
	case 86018: // AV_CODEC_ID_AAC
		info.codec = CELL_SEARCH_CODEC_AAC;
		break;
	case 86019: // AV_CODEC_ID_AC3
		info.codec = CELL_SEARCH_CODEC_AC3;
		break;
	case 86023: // AV_CODEC_ID_WMAV1
	case 86024: // AV_CODEC_ID_WMAV2
		info.codec = CELL_SEARCH_CODEC_WMA;
		break;
	case 86047: // AV_CODEC_ID_ATRAC3
		info.codec = CELL_SEARCH_CODEC_AT3;
		break;
	case 86055: // AV_CODEC_ID_ATRAC3P
		info.codec = CELL_SEARCH_CODEC_AT3P;
		break;
	case 88078: // AV_CODEC_ID_ATRAC3AL
	//case 88079: // AV_CODEC_ID_ATRAC3PAL TODO: supported ?
		info.codec = CELL_SEARCH_CODEC_ATALL;
		break;
	// TODO: Find out if any of this works
	//case 88069: // AV_CODEC_ID_DSD_LSBF
	//case 88070: // AV_CODEC_ID_DSD_MSBF
	//case 88071: // AV_CODEC_ID_DSD_LSBF_PLANAR
	//case 88072: // AV_CODEC_ID_DSD_MSBF_PLANAR
	//	info.codec = CELL_SEARCH_CODEC_DSD;
	//	break;
	//case ???:
	//	info.codec = CELL_SEARCH_CODEC_WAV;
	//	break;
	default:
		info.codec = CELL_SEARCH_CODEC_UNKNOWN;
		info.status = CELL_SEARCH_CONTENTSTATUS_NOT_SUPPORTED;
		break;
	}
}

void populate_video_info(CellSearchVideoInfo& info, const utils::media_info& mi, const fs::dir_entry& item)
{
	parse_metadata(info.albumTitle, mi, "album", "Unknown Album", CELL_SEARCH_TITLE_LEN_MAX);
	parse_metadata(info.title, mi, "title", item.name.substr(0, item.name.find_last_of('.')), CELL_SEARCH_TITLE_LEN_MAX);

	info.size = item.size;
	info.duration = mi.duration_us / 1000; // we need microseconds
	info.audioBitrate = mi.audio_bitrate_bps;
	info.videoBitrate = mi.video_bitrate_bps;
	info.playCount = 0; // we do not track this for now
	info.importedDate = -1; // we do not track this for now
	info.takenDate = -1; // we do not track this for now
	info.drmEncrypted = 0; // TODO: Needs to be 1 if it's encrypted
	info.status = CELL_SEARCH_CONTENTSTATUS_AVAILABLE;

	// Convert Video AVCodecID to CellSearchCodec
	switch (mi.video_av_codec_id)
	{
	case 1: // AV_CODEC_ID_MPEG1VIDEO
		info.videoCodec = CELL_SEARCH_CODEC_MPEG1;
		break;
	case 2: // AV_CODEC_ID_MPEG2VIDEO
		info.videoCodec = CELL_SEARCH_CODEC_MPEG2;
		break;
	case 12: // AV_CODEC_ID_MPEG4
		info.videoCodec = CELL_SEARCH_CODEC_MPEG4;
		break;
	case 27: // AV_CODEC_ID_H264
		info.videoCodec = CELL_SEARCH_CODEC_AVC;
		break;
	default:
		info.videoCodec = CELL_SEARCH_CODEC_UNKNOWN;
		info.status = CELL_SEARCH_CONTENTSTATUS_NOT_SUPPORTED;
		break;
	}

	// Convert Audio AVCodecID to CellSearchCodec
	switch (mi.audio_av_codec_id)
	{
	// Let's ignore this due to CELL_SEARCH_CODEC_MPEG1_LAYER3
	//case 86017: // AV_CODEC_ID_MP3
	//	info.audioCodec = CELL_SEARCH_CODEC_MP3;
	//	break;
	case 86018: // AV_CODEC_ID_AAC
		info.audioCodec = CELL_SEARCH_CODEC_AAC;
		break;
	case 86019: // AV_CODEC_ID_AC3
		info.audioCodec = CELL_SEARCH_CODEC_AC3;
		break;
	case 86023: // AV_CODEC_ID_WMAV1
	case 86024: // AV_CODEC_ID_WMAV2
		info.audioCodec = CELL_SEARCH_CODEC_WMA;
		break;
	case 86047: // AV_CODEC_ID_ATRAC3
		info.audioCodec = CELL_SEARCH_CODEC_AT3;
		break;
	case 86055: // AV_CODEC_ID_ATRAC3P
		info.audioCodec = CELL_SEARCH_CODEC_AT3P;
		break;
	case 88078: // AV_CODEC_ID_ATRAC3AL
	//case 88079: // AV_CODEC_ID_ATRAC3PAL TODO: supported ?
		info.audioCodec = CELL_SEARCH_CODEC_ATALL;
		break;
	// TODO: Find out if any of this works
	//case 88069: // AV_CODEC_ID_DSD_LSBF
	//case 88070: // AV_CODEC_ID_DSD_MSBF
	//case 88071: // AV_CODEC_ID_DSD_LSBF_PLANAR
	//case 88072: // AV_CODEC_ID_DSD_MSBF_PLANAR
	//	info.audioCodec = CELL_SEARCH_CODEC_DSD;
	//	break;
	//case ???:
	//	info.audioCodec = CELL_SEARCH_CODEC_WAV;
	//	break;
	case 86058: // AV_CODEC_ID_MP1
		info.audioCodec = CELL_SEARCH_CODEC_MPEG1_LAYER1;
		break;
	case 86016: // AV_CODEC_ID_MP2
		info.audioCodec = CELL_SEARCH_CODEC_MPEG1_LAYER2;
		break;
	case 86017: // AV_CODEC_ID_MP3
		info.audioCodec = CELL_SEARCH_CODEC_MPEG1_LAYER3;
		break;
	//case ???:
	//	info.audioCodec = CELL_SEARCH_CODEC_MPEG2_LAYER1;
	//	break;
	//case ???:
	//	info.audioCodec = CELL_SEARCH_CODEC_MPEG2_LAYER2;
	//	break;
	//case ???:
	//	info.audioCodec = CELL_SEARCH_CODEC_MPEG2_LAYER3;
	//	break;
	default:
		info.audioCodec = CELL_SEARCH_CODEC_UNKNOWN;
		info.status = CELL_SEARCH_CONTENTSTATUS_NOT_SUPPORTED;
		break;
	}
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

	auto& search = g_fxo->get<search_info>();

	if (error_code error = check_search_state(search.state.compare_and_swap(search_state::not_initialized, search_state::initializing), search_state::initializing))
	{
		return error;
	}

	search.func = func;
	search.userData = userData;

	sysutil_register_cb([=, &search](ppu_thread& ppu) -> s32
	{
		search.state.store(search_state::idle);
		func(ppu, CELL_SEARCH_EVENT_INITIALIZE_RESULT, CELL_OK, vm::null, userData);
		return CELL_OK;
	});

	return CELL_OK;
}

error_code cellSearchFinalize()
{
	cellSearch.todo("cellSearchFinalize()");

	auto& search = g_fxo->get<search_info>();

	if (error_code error = check_search_state(search.state.compare_and_swap(search_state::idle, search_state::finalizing), search_state::finalizing))
	{
		return error;
	}

	sysutil_register_cb([&search](ppu_thread& ppu) -> s32
	{
		{
			std::lock_guard lock(search.links_mutex);
			search.content_links.clear();
		}
		search.state.store(search_state::not_initialized);
		search.func(ppu, CELL_SEARCH_EVENT_FINALIZE_RESULT, CELL_OK, vm::null, search.userData);
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

	auto& search = g_fxo->get<search_info>();

	if (error_code error = check_search_state(search.state.compare_and_swap(search_state::idle, search_state::in_progress), search_state::in_progress))
	{
		return error;
	}

	const u32 id = *outSearchId = idm::make<search_object_t>();

	sysutil_register_cb([=, &content_map = g_fxo->get<content_id_map>(), &search](ppu_thread& ppu) -> s32
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

				if (item.name == "." || item.name == ".." || !item.is_directory)
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
				auto found = content_map.map.find(hash);
				if (found == content_map.map.end()) // content isn't yet being tracked
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

						// std::lock_guard lock(search.links_mutex);
						// search.content_links.emplace(std::move(link), item_path);
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

					content_map.map.emplace(hash, curr_find);
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

		search.state.store(search_state::idle);
		search.func(ppu, CELL_SEARCH_EVENT_LISTSEARCH_RESULT, CELL_OK, vm::cast(resultParam.addr()), search.userData);
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

	auto& search = g_fxo->get<search_info>();

	if (error_code error = check_search_state(search.state.compare_and_swap(search_state::idle, search_state::in_progress), search_state::in_progress))
	{
		return error;
	}

	auto& content_map = g_fxo->get<content_id_map>();
	auto found = content_map.map.find(*reinterpret_cast<const u64*>(listId->data));
	if (found == content_map.map.end())
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

	sysutil_register_cb([=, list_path = std::string(content_info->infoPath.contentPath), &search, &content_map](ppu_thread& ppu) -> s32
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
				auto found = content_map.map.find(hash);
				if (found == content_map.map.end()) // content isn't yet being tracked
				{
					auto ext_offset = item.name.find_last_of('.'); // used later if no "Title" found

					std::shared_ptr<search_content_t> curr_find = std::make_shared<search_content_t>();
					if (item_path.length() > CELL_SEARCH_PATH_LEN_MAX)
					{
						// Create mapping which will be resolved to an actual hard link in VFS by cellSearchPrepareFile
						std::string link = "/.tmp/" + std::to_string(hash) + item.name.substr(ext_offset);
						strcpy_trunc(curr_find->infoPath.contentPath, link);

						std::lock_guard lock(search.links_mutex);
						search.content_links.emplace(std::move(link), item_path);
					}
					else
					{
						strcpy_trunc(curr_find->infoPath.contentPath, item_path);
					}

					// TODO - curr_find.infoPath.thumbnailPath

					if (type == CELL_SEARCH_CONTENTSEARCHTYPE_MUSIC_ALL)
					{
						curr_find->type = CELL_SEARCH_CONTENTTYPE_MUSIC;

						const std::string path = vfs::get(vpath) + "/" + item.name;
						const auto [success, mi] = utils::get_media_info(path, 1); // AVMEDIA_TYPE_AUDIO
						if (!success)
						{
							continue;
						}

						populate_music_info(curr_find->data.music, mi, item);
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

						const std::string path = vfs::get(vpath) + "/" + item.name;
						const auto [success, mi] = utils::get_media_info(path, 0); // AVMEDIA_TYPE_VIDEO
						if (!success)
						{
							continue;
						}

						populate_video_info(curr_find->data.video, mi, item);
					}

					content_map.map.emplace(hash, curr_find);
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

		search.state.store(search_state::idle);
		search.func(ppu, CELL_SEARCH_EVENT_CONTENTSEARCH_INLIST_RESULT, CELL_OK, vm::cast(resultParam.addr()), search.userData);
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

	auto& search = g_fxo->get<search_info>();

	if (error_code error = check_search_state(search.state.compare_and_swap(search_state::idle, search_state::in_progress), search_state::in_progress))
	{
		return error;
	}

	const u32 id = *outSearchId = idm::make<search_object_t>();

	sysutil_register_cb([=, &content_map = g_fxo->get<content_id_map>(), &search](ppu_thread& ppu) -> s32
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
				auto found = content_map.map.find(hash);
				if (found == content_map.map.end()) // content isn't yet being tracked
				{
					auto ext_offset = item.name.find_last_of('.'); // used later if no "Title" found

					std::shared_ptr<search_content_t> curr_find = std::make_shared<search_content_t>();
					if (item_path.length() > CELL_SEARCH_PATH_LEN_MAX)
					{
						// Create mapping which will be resolved to an actual hard link in VFS by cellSearchPrepareFile
						std::string link = "/.tmp/" + std::to_string(hash) + item.name.substr(ext_offset);
						strcpy_trunc(curr_find->infoPath.contentPath, link);

						std::lock_guard lock(search.links_mutex);
						search.content_links.emplace(std::move(link), item_path);
					}
					else
					{
						strcpy_trunc(curr_find->infoPath.contentPath, item_path);
					}

					// TODO - curr_find.infoPath.thumbnailPath

					if (type == CELL_SEARCH_CONTENTSEARCHTYPE_MUSIC_ALL)
					{
						curr_find->type = CELL_SEARCH_CONTENTTYPE_MUSIC;

						const std::string path = vfs::get(vpath) + "/" + item.name;
						const auto [success, mi] = utils::get_media_info(path, 1); // AVMEDIA_TYPE_AUDIO
						if (!success)
						{
							continue;
						}

						populate_music_info(curr_find->data.music, mi, item);
					}
					else if (type == CELL_SEARCH_CONTENTSEARCHTYPE_PHOTO_ALL)
					{
						curr_find->type = CELL_SEARCH_CONTENTTYPE_PHOTO;
						CellSearchPhotoInfo& info = curr_find->data.photo;
						// TODO - Some kinda file photo analysis and assign the values as such
						info.size = item.size;
						info.importedDate = -1;
						info.takenDate = -1;
						info.width = 0;
						info.height = 0;
						info.orientation = CELL_SEARCH_ORIENTATION_UNKNOWN;
						info.codec = CELL_SEARCH_CODEC_UNKNOWN;
						info.status = CELL_SEARCH_CONTENTSTATUS_AVAILABLE;
						strcpy_trunc(info.title, item.name.substr(0, ext_offset));
						strcpy_trunc(info.albumTitle, "ALBUM TITLE");
					}
					else if (type == CELL_SEARCH_CONTENTSEARCHTYPE_VIDEO_ALL)
					{
						curr_find->type = CELL_SEARCH_CONTENTTYPE_VIDEO;

						const std::string path = vfs::get(vpath) + "/" + item.name;
						const auto [success, mi] = utils::get_media_info(path, 0); // AVMEDIA_TYPE_VIDEO
						if (!success)
						{
							continue;
						}

						populate_video_info(curr_find->data.video, mi, item);
					}

					content_map.map.emplace(hash, curr_find);
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

		search.state.store(search_state::idle);
		search.func(ppu, CELL_SEARCH_EVENT_CONTENTSEARCH_RESULT, CELL_OK, vm::cast(resultParam.addr()), search.userData);
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

	auto& search = g_fxo->get<search_info>();

	if (error_code error = check_search_state(search.state.compare_and_swap(search_state::idle, search_state::in_progress), search_state::in_progress))
	{
		return error;
	}

	auto& content_map = g_fxo->get<content_id_map>();
	auto found = content_map.map.find(*reinterpret_cast<const u64*>(videoId->data));
	if (found == content_map.map.end())
	{
		// content ID not found, perform a search first
		return CELL_SEARCH_ERROR_CONTENT_NOT_FOUND;
	}

	const auto& content_info = found->second;
	if (content_info->type != CELL_SEARCH_CONTENTTYPE_VIDEO)
	{
		return CELL_SEARCH_ERROR_INVALID_CONTENTTYPE;
	}

	const u32 id = *outSearchId = idm::make<search_object_t>();

	sysutil_register_cb([=, &search](ppu_thread& ppu) -> s32
	{
		vm::var<CellSearchResultParam> resultParam;
		resultParam->searchId = id;
		resultParam->resultNum = 0; // TODO

		search.state.store(search_state::idle);
		search.func(ppu, CELL_SEARCH_EVENT_SCENESEARCH_INVIDEO_RESULT, CELL_OK, vm::cast(resultParam.addr()), search.userData);
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

	if (tagNum) // TODO: find out if this is the correct location for these checks 
	{
		if (tagNum > CELL_SEARCH_TAG_NUM_MAX || !tags)
		{
			return CELL_SEARCH_ERROR_TAG;
		}

		for (u32 n = 0; n < tagNum; n++)
		{
			if (!tags[tagNum] || !memchr(&tags[tagNum], '\0', CELL_SEARCH_TAG_LEN_MAX))
			{
				return CELL_SEARCH_ERROR_TAG;
			}
		}
	}

	auto& search = g_fxo->get<search_info>();

	if (error_code error = check_search_state(search.state.compare_and_swap(search_state::idle, search_state::in_progress), search_state::in_progress))
	{
		return error;
	}

	const u32 id = *outSearchId = idm::make<search_object_t>();

	sysutil_register_cb([=, &search](ppu_thread& ppu) -> s32
	{
		vm::var<CellSearchResultParam> resultParam;
		resultParam->searchId = id;
		resultParam->resultNum = 0; // TODO

		search.state.store(search_state::idle);
		search.func(ppu, CELL_SEARCH_EVENT_SCENESEARCH_RESULT, CELL_OK, vm::cast(resultParam.addr()), search.userData);
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

	if (error_code error = check_search_state(g_fxo->get<search_info>().state.load(), search_state::in_progress))
	{
		return error;
	}

	if (offset >= 0 && offset + 0u < searchObject->content_ids.size())
	{
		const auto& content_id = searchObject->content_ids[offset];
		const auto& content_info = content_id.second;

		switch (content_info->type)
		{
		case CELL_SEARCH_CONTENTTYPE_MUSIC:
			if (infoBuffer) std::memcpy(infoBuffer.get_ptr(), &content_info->data.music, sizeof(content_info->data.music));
			break;
		case CELL_SEARCH_CONTENTTYPE_PHOTO:
			if (infoBuffer) std::memcpy(infoBuffer.get_ptr(), &content_info->data.photo, sizeof(content_info->data.photo));
			break;
		case CELL_SEARCH_CONTENTTYPE_VIDEO:
			if (infoBuffer) std::memcpy(infoBuffer.get_ptr(), &content_info->data.video, sizeof(content_info->data.photo));
			break;
		case CELL_SEARCH_CONTENTTYPE_MUSICLIST:
			if (infoBuffer) std::memcpy(infoBuffer.get_ptr(), &content_info->data.music_list, sizeof(content_info->data.music_list));
			break;
		case CELL_SEARCH_CONTENTTYPE_PHOTOLIST:
			if (infoBuffer) std::memcpy(infoBuffer.get_ptr(), &content_info->data.photo_list, sizeof(content_info->data.photo_list));
			break;
		case CELL_SEARCH_CONTENTTYPE_VIDEOLIST:
			if (infoBuffer) std::memcpy(infoBuffer.get_ptr(), &content_info->data.video_list, sizeof(content_info->data.video_list));
			break;
		case CELL_SEARCH_CONTENTTYPE_SCENE:
			if (infoBuffer) std::memcpy(infoBuffer.get_ptr(), &content_info->data.scene, sizeof(content_info->data.scene));
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

	if (error_code error = check_search_state(g_fxo->get<search_info>().state.load(), search_state::in_progress))
	{
		return error;
	}

	auto& content_map = g_fxo->get<content_id_map>();
	auto found = content_map.map.find(*reinterpret_cast<const u64*>(contentId->data));
	if (found != content_map.map.end())
	{
		const auto& content_info = found->second;
		switch (content_info->type)
		{
		case CELL_SEARCH_CONTENTTYPE_MUSIC:
			if (infoBuffer) std::memcpy(infoBuffer.get_ptr(), &content_info->data.music, sizeof(content_info->data.music));
			break;
		case CELL_SEARCH_CONTENTTYPE_PHOTO:
			if (infoBuffer) std::memcpy(infoBuffer.get_ptr(), &content_info->data.photo, sizeof(content_info->data.photo));
			break;
		case CELL_SEARCH_CONTENTTYPE_VIDEO:
			if (infoBuffer) std::memcpy(infoBuffer.get_ptr(), &content_info->data.video, sizeof(content_info->data.photo));
			break;
		case CELL_SEARCH_CONTENTTYPE_MUSICLIST:
			if (infoBuffer) std::memcpy(infoBuffer.get_ptr(), &content_info->data.music_list, sizeof(content_info->data.music_list));
			break;
		case CELL_SEARCH_CONTENTTYPE_PHOTOLIST:
			if (infoBuffer) std::memcpy(infoBuffer.get_ptr(), &content_info->data.photo_list, sizeof(content_info->data.photo_list));
			break;
		case CELL_SEARCH_CONTENTTYPE_VIDEOLIST:
			if (infoBuffer) std::memcpy(infoBuffer.get_ptr(), &content_info->data.video_list, sizeof(content_info->data.video_list));
			break;
		case CELL_SEARCH_CONTENTTYPE_SCENE:
			if (infoBuffer) std::memcpy(infoBuffer.get_ptr(), &content_info->data.scene, sizeof(content_info->data.scene));
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

	if (error_code error = check_search_state(g_fxo->get<search_info>().state.load(), search_state::in_progress))
	{
		return error;
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

	if (error_code error = check_search_state(g_fxo->get<search_info>().state.load(), search_state::in_progress))
	{
		return error;
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

	if (!contentId || !gameComment)
	{
		return CELL_SEARCH_ERROR_PARAM;
	}

	// TODO: find out if this check is correct
	if (error_code error = check_search_state(g_fxo->get<search_info>().state.load(), search_state::in_progress))
	{
		return error;
	}

	auto& content_map = g_fxo->get<content_id_map>();
	auto found = content_map.map.find(*reinterpret_cast<const u64*>(contentId->data));
	if (found == content_map.map.end())
	{
		// content ID not found, perform a search first
		return CELL_SEARCH_ERROR_CONTENT_NOT_FOUND;
	}

	const auto& content_info = found->second;
	switch (content_info->type)
	{
	case CELL_SEARCH_CONTENTTYPE_MUSIC:
	case CELL_SEARCH_CONTENTTYPE_PHOTO:
	case CELL_SEARCH_CONTENTTYPE_VIDEO:
		break;
	default:
		return CELL_SEARCH_ERROR_INVALID_CONTENTTYPE;
	}

	// TODO: retrieve gameComment

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

	// TODO: find out if this check is correct
	if (error_code error = check_search_state(g_fxo->get<search_info>().state.load(), search_state::in_progress))
	{
		return error;
	}

	if (searchObject->content_ids.empty())
	{
		return CELL_SEARCH_ERROR_CONTENT_NOT_FOUND;
	}

	// Use the first track in order to get info about this search
	const auto& first_content_id = searchObject->content_ids[0];
	const auto& first_content = first_content_id.second;
	ensure(first_content);

	if (contentId)
	{
		// Try to find the specified content
		const u64 content_hash = *reinterpret_cast<const u64*>(contentId->data);
		auto content = std::find_if(searchObject->content_ids.begin(), searchObject->content_ids.end(), [&content_hash](const content_id_type& cid){ return cid.first == content_hash; });
		if (content != searchObject->content_ids.cend() && content->second)
		{
			// Check if the type of the found content is correct
			if (content->second->type != first_content->type)
			{
				return CELL_SEARCH_ERROR_NOT_SUPPORTED_CONTEXT;
			}

			// TODO: Use the found content
			// first track/playlist of context = content->second;
		}
		else if (first_content->type == CELL_SEARCH_CONTENTTYPE_MUSICLIST)
		{
			// Abort if we can't find the playlist.
			return CELL_SEARCH_ERROR_CONTENT_NOT_FOUND;
		}
		else
		{
			// TODO: Select the first track by default
			// first track of context = first_content;
		}
	}
	else if (first_content->type == CELL_SEARCH_CONTENTTYPE_MUSICLIST)
	{
		// Abort if we don't have the necessary info to select a playlist.
		return CELL_SEARCH_ERROR_NOT_SUPPORTED_CONTEXT;
	}
	else
	{
		// TODO: Select the first track by default
		// first track of context = first_content;
	}

	// TODO: Use repeatMode and option in our context, depending on the type (list vs single)
	// TODO: assign outContext

	return CELL_OK;
}

error_code cellSearchGetMusicSelectionContextOfSingleTrack(vm::cptr<CellSearchContentId> contentId, vm::ptr<CellMusicSelectionContext> outContext)
{
	cellSearch.todo("cellSearchGetMusicSelectionContextOfSingleTrack(contentId=*0x%x, outContext=*0x%x)", contentId, outContext);

	if (!contentId || !outContext)
	{
		return CELL_SEARCH_ERROR_PARAM;
	}

	// TODO: find out if this check is correct
	if (error_code error = check_search_state(g_fxo->get<search_info>().state.load(), search_state::in_progress))
	{
		return error;
	}

	auto& content_map = g_fxo->get<content_id_map>();
	auto found = content_map.map.find(*reinterpret_cast<const u64*>(contentId->data));
	if (found == content_map.map.end())
	{
		// content ID not found, perform a search first
		return CELL_SEARCH_ERROR_CONTENT_NOT_FOUND;
	}

	const auto& content_info = found->second;
	if (content_info->type != CELL_SEARCH_CONTENTTYPE_MUSIC)
	{
		return CELL_SEARCH_ERROR_INVALID_CONTENTTYPE;
	}

	// TODO: assign outContext

	return CELL_OK;
}

error_code cellSearchGetContentInfoPath(vm::cptr<CellSearchContentId> contentId, vm::ptr<CellSearchContentInfoPath> infoPath)
{
	cellSearch.todo("cellSearchGetContentInfoPath(contentId=*0x%x, infoPath=*0x%x)", contentId, infoPath);

	if (!infoPath)
	{
		return CELL_SEARCH_ERROR_PARAM;
	}

	if (error_code error = check_search_state(g_fxo->get<search_info>().state.load(), search_state::in_progress))
	{
		return error;
	}

	const u64 id = *reinterpret_cast<const u64*>(contentId->data);
	auto& content_map = g_fxo->get<content_id_map>();
	auto found = content_map.map.find(id);
	if(found != content_map.map.end())
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

	if (!contentId || !infoMt)
	{
		return CELL_SEARCH_ERROR_PARAM;
	}

	// TODO: find out if this check is correct
	if (error_code error = check_search_state(g_fxo->get<search_info>().state.load(), search_state::in_progress))
	{
		return error;
	}

	auto& content_map = g_fxo->get<content_id_map>();
	auto found = content_map.map.find(*reinterpret_cast<const u64*>(contentId->data));
	if (found == content_map.map.end())
	{
		// content ID not found, perform a search first
		return CELL_SEARCH_ERROR_CONTENT_NOT_FOUND;
	}

	const auto& content_info = found->second;
	if (content_info->type != CELL_SEARCH_CONTENTTYPE_VIDEO)
	{
		return CELL_SEARCH_ERROR_INVALID_CONTENTTYPE;
	}

	strcpy_trunc(infoMt->movieThumbnailPath, content_info->infoPath.thumbnailPath);
	// TODO: set infoMt->movieThumbnailOption

	return CELL_OK;
}

error_code cellSearchPrepareFile(vm::cptr<char> path)
{
	cellSearch.todo("cellSearchPrepareFile(path=%s)", path);

	if (!path)
	{
		return CELL_SEARCH_ERROR_PARAM;
	}

	auto& search = g_fxo->get<search_info>();

	if (error_code error = check_search_state(search.state.load(), search_state::in_progress))
	{
		return error;
	}

	reader_lock lock(search.links_mutex);
	auto found = search.content_links.find(path.get_ptr());
	if (found != search.content_links.end())
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

	// TODO: find out if this check is correct
	if (error_code error = check_search_state(g_fxo->get<search_info>().state.load(), search_state::in_progress))
	{
		return error;
	}

	auto& content_map = g_fxo->get<content_id_map>();
	auto found = content_map.map.find(*reinterpret_cast<const u64*>(contentId->data));
	if (found == content_map.map.end())
	{
		// content ID not found, perform a search first
		return CELL_SEARCH_ERROR_CONTENT_NOT_FOUND;
	}

	const auto& content_info = found->second;
	switch (content_info->type)
	{
	case CELL_SEARCH_CONTENTTYPE_VIDEO:
	case CELL_SEARCH_CONTENTTYPE_SCENE:
		break;
	default:
		return CELL_SEARCH_ERROR_INVALID_CONTENTTYPE;
	}

	// TODO: retrieve developerData

	return CELL_OK;
}

error_code cellSearchGetContentInfoSharable(vm::cptr<CellSearchContentId> contentId, vm::ptr<CellSearchSharableType> sharable)
{
	cellSearch.todo("cellSearchGetContentInfoSharable(contentId=*0x%x, sharable=*0x%x)", contentId, sharable);

	if (!contentId || !sharable)
	{
		return CELL_SEARCH_ERROR_PARAM;
	}

	// TODO: find out if this check is correct
	if (error_code error = check_search_state(g_fxo->get<search_info>().state.load(), search_state::in_progress))
	{
		return error;
	}

	auto& content_map = g_fxo->get<content_id_map>();
	auto found = content_map.map.find(*reinterpret_cast<const u64*>(contentId->data));
	if (found == content_map.map.end())
	{
		// content ID not found, perform a search first
		return CELL_SEARCH_ERROR_CONTENT_NOT_FOUND;
	}

	const auto& content_info = found->second;
	if (content_info->type != CELL_SEARCH_CONTENTTYPE_VIDEO)
	{
		return CELL_SEARCH_ERROR_INVALID_CONTENTTYPE;
	}

	// TODO: retrieve sharable
	*sharable = CELL_SEARCH_SHARABLETYPE_PROHIBITED;

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

	if (error_code error = check_search_state(g_fxo->get<search_info>().state.load(), search_state::canceling))
	{
		return error;
	}

	// TODO

	return CELL_OK;
}

error_code cellSearchEnd(CellSearchId searchId)
{
	cellSearch.todo("cellSearchEnd(searchId=0x%x)", searchId);

	if (error_code error = check_search_state(g_fxo->get<search_info>().state.load(), search_state::finalizing))
	{
		return error;
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
