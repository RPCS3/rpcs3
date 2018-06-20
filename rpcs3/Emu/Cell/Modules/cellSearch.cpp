#include "stdafx.h"
#include "Emu/System.h"
#include "Emu/IdManager.h"
#include "Emu/Cell/PPUModule.h"
#include "cellMusic.h"
#include "cellSysutil.h"

#include "cellSearch.h"
#include "xxhash.h"

logs::channel cellSearch("cellSearch");

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

struct search_t
{
	vm::ptr<CellSearchSystemCallback> func;
	vm::ptr<void> userData;
};

struct search_content_t : std::enable_shared_from_this<search_content_t>
{
	CellSearchContentType type = CELL_SEARCH_CONTENTTYPE_NONE;
	CellSearchTimeInfo timeInfo;
	CellSearchContentInfoPath infoPath;
	std::unique_ptr<u8[]> data;

	std::shared_ptr<search_content_t> get_shared()
	{
		return shared_from_this();
	}
};

using ContentIdType = std::pair<u64, std::shared_ptr<search_content_t>>;
using ContentIdMap = std::unordered_map<u64, std::shared_ptr<search_content_t>>;

struct search_object_t
{
	// TODO: Figured out the correct values to set here
	static const u32 id_base  = 1;
	static const u32 id_step  = 1;
	static const u32 id_count = 64;
	static const u32 invalid  = 0xFFFFFFFF;

	std::vector<ContentIdType> content_ids;
};

enum SearchState : u8
{
	SEARCH_STATE_NOT_INITIALIZED  = 0,
	SEARCH_STATE_IDLE,
	SEARCH_STATE_IN_PROGRESS,
	SEARCH_STATE_INITIALIZING,
	SEARCH_STATE_CANCELING,
	SEARCH_STATE_FINALIZING,
};

static SearchState s_search_state = SEARCH_STATE_NOT_INITIALIZED; 

error_code cellSearchInitialize(CellSearchMode mode, u32 container, vm::ptr<CellSearchSystemCallback> func, vm::ptr<void> userData)
{
	cellSearch.warning("cellSearchInitialize(mode=0x%x, container=0x%x, func=*0x%x, userData=*0x%x)", (u32) mode, container, func, userData);

	if (!func)
	{
		return CELL_SEARCH_ERROR_PARAM;
	}

	switch(s_search_state)
	{
		case SEARCH_STATE_NOT_INITIALIZED:
			break;
		case SEARCH_STATE_INITIALIZING:
			return CELL_SEARCH_ERROR_BUSY;
		case SEARCH_STATE_FINALIZING:
			return CELL_SEARCH_ERROR_FINALIZING;
		default:
			return CELL_SEARCH_ERROR_ALREADY_INITIALIZED;
	}

	if (mode != CELL_SEARCH_MODE_NORMAL)
	{
		return CELL_SEARCH_ERROR_UNKNOWN_MODE;
	}

	const auto search = fxm::make_always<search_t>();
	search->func = func;
	search->userData = userData;

	s_search_state = SEARCH_STATE_INITIALIZING;
	sysutil_register_cb([=](ppu_thread& ppu) -> s32
	{
		s_search_state = SEARCH_STATE_IDLE;
		func(ppu, CELL_SEARCH_EVENT_INITIALIZE_RESULT, CELL_OK, vm::null, userData);
		return CELL_OK;
	});

	return CELL_OK;
}

error_code cellSearchFinalize()
{
	cellSearch.todo("cellSearchFinalize()");

	switch(s_search_state)
	{
		case SEARCH_STATE_IDLE:
			break;
		case SEARCH_STATE_NOT_INITIALIZED:
			return CELL_SEARCH_ERROR_NOT_INITIALIZED;
		case SEARCH_STATE_FINALIZING:
			return CELL_SEARCH_ERROR_FINALIZING;
		case SEARCH_STATE_IN_PROGRESS:
		case SEARCH_STATE_INITIALIZING:
		case SEARCH_STATE_CANCELING:
			return CELL_SEARCH_ERROR_BUSY;
		default:
			return CELL_SEARCH_ERROR_GENERIC;
	}

	s_search_state = SEARCH_STATE_FINALIZING;
	sysutil_register_cb([=](ppu_thread& ppu) -> s32
	{
		const auto search = fxm::get_always<search_t>();

		s_search_state = SEARCH_STATE_NOT_INITIALIZED;
		search->func(ppu, CELL_SEARCH_EVENT_FINALIZE_RESULT, CELL_OK, vm::null, search->userData);
		return CELL_OK;
	});

	return CELL_OK;
}

error_code cellSearchStartListSearch(CellSearchListSearchType type, CellSearchSortOrder sortOrder, vm::ptr<CellSearchId> outSearchId)
{
	cellSearch.todo("cellSearchStartListSearch(type=0x%x, sortOrder=0x%x, outSearchId=*0x%x)", (u32) type, (u32) sortOrder, outSearchId);

	if (!outSearchId)
	{
		return CELL_SEARCH_ERROR_PARAM;
	}

	switch (type)
	{
		case CELL_SEARCH_LISTSEARCHTYPE_MUSIC_ALBUM:
		case CELL_SEARCH_LISTSEARCHTYPE_MUSIC_GENRE:
		case CELL_SEARCH_LISTSEARCHTYPE_MUSIC_ARTIST:
		case CELL_SEARCH_LISTSEARCHTYPE_PHOTO_YEAR:
		case CELL_SEARCH_LISTSEARCHTYPE_PHOTO_MONTH:
		case CELL_SEARCH_LISTSEARCHTYPE_PHOTO_ALBUM:
		case CELL_SEARCH_LISTSEARCHTYPE_PHOTO_PLAYLIST:
		case CELL_SEARCH_LISTSEARCHTYPE_VIDEO_ALBUM:
		case CELL_SEARCH_LISTSEARCHTYPE_MUSIC_PLAYLIST:
			break;
		default:
			return CELL_SEARCH_ERROR_PARAM;
	}

	if (sortOrder != CELL_SEARCH_SORTORDER_ASCENDING && sortOrder != CELL_SEARCH_SORTORDER_DESCENDING)
	{
		return CELL_SEARCH_ERROR_PARAM;
	}

	switch(s_search_state)
	{
		case SEARCH_STATE_IDLE:
			break;
		case SEARCH_STATE_NOT_INITIALIZED:
		case SEARCH_STATE_INITIALIZING:
			return CELL_SEARCH_ERROR_NOT_INITIALIZED;
		case SEARCH_STATE_FINALIZING:
			return CELL_SEARCH_ERROR_FINALIZING;
		case SEARCH_STATE_IN_PROGRESS:
		case SEARCH_STATE_CANCELING:
			return CELL_SEARCH_ERROR_BUSY;
		default:
			return CELL_SEARCH_ERROR_GENERIC;
	}

	*outSearchId = idm::make<search_object_t>();

	s_search_state = SEARCH_STATE_IN_PROGRESS;
	sysutil_register_cb([=](ppu_thread& ppu) -> s32
	{
		const auto search = fxm::get_always<search_t>();

		vm::var<CellSearchResultParam> resultParam;
		resultParam->searchId = *outSearchId;
		resultParam->resultNum = 0; // TODO

		s_search_state = SEARCH_STATE_IDLE;
		search->func(ppu, CELL_SEARCH_EVENT_LISTSEARCH_RESULT, CELL_OK, vm::cast(resultParam.addr()), search->userData);
		return CELL_OK;
	});

	return CELL_OK;
}

error_code cellSearchStartContentSearchInList(vm::cptr<CellSearchContentId> listId, CellSearchSortKey sortKey, CellSearchSortOrder sortOrder, vm::ptr<CellSearchId> outSearchId)
{
	cellSearch.todo("cellSearchStartContentSearchInList(listId=*0x%x, sortKey=0x%x, sortOrder=0x%x, outSearchId=*0x%x)", listId, (u32) sortKey, (u32) sortOrder, outSearchId);

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

	switch(s_search_state)
	{
		case SEARCH_STATE_IDLE:
			break;
		case SEARCH_STATE_NOT_INITIALIZED:
		case SEARCH_STATE_INITIALIZING:
			return CELL_SEARCH_ERROR_NOT_INITIALIZED;
		case SEARCH_STATE_FINALIZING:
			return CELL_SEARCH_ERROR_FINALIZING;
		case SEARCH_STATE_IN_PROGRESS:
		case SEARCH_STATE_CANCELING:
			return CELL_SEARCH_ERROR_BUSY;
		default:
			return CELL_SEARCH_ERROR_GENERIC;
	}

	*outSearchId = idm::make<search_object_t>();

	s_search_state = SEARCH_STATE_IN_PROGRESS;
	sysutil_register_cb([=](ppu_thread& ppu) -> s32
	{
		const auto search = fxm::get_always<search_t>();

		vm::var<CellSearchResultParam> resultParam;
		resultParam->searchId = *outSearchId;
		resultParam->resultNum = 0; // TODO

		s_search_state = SEARCH_STATE_IDLE;
		search->func(ppu, CELL_SEARCH_EVENT_CONTENTSEARCH_INLIST_RESULT, CELL_OK, vm::cast(resultParam.addr()), search->userData);
		return CELL_OK;
	});

	return CELL_OK;
}

error_code cellSearchStartContentSearch(CellSearchContentSearchType type, CellSearchSortKey sortKey, CellSearchSortOrder sortOrder, vm::ptr<CellSearchId> outSearchId)
{
	cellSearch.todo("cellSearchStartContentSearch(type=0x%x, sortKey=0x%x, sortOrder=0x%x, outSearchId=*0x%x)", (u32) type, (u32) sortKey, (u32) sortOrder, outSearchId);

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

	char* media_dir = (char*)malloc(6);
	switch (type)
	{
		case CELL_SEARCH_CONTENTSEARCHTYPE_MUSIC_ALL: strcpy(media_dir, "music"); break;
		case CELL_SEARCH_CONTENTSEARCHTYPE_PHOTO_ALL: strcpy(media_dir, "photo"); break;
		case CELL_SEARCH_CONTENTSEARCHTYPE_VIDEO_ALL: strcpy(media_dir, "video"); break;
		default:
			return CELL_SEARCH_ERROR_PARAM;
	}

	switch(s_search_state)
	{
		case SEARCH_STATE_IDLE:
			break;
		case SEARCH_STATE_NOT_INITIALIZED:
		case SEARCH_STATE_INITIALIZING:
			return CELL_SEARCH_ERROR_NOT_INITIALIZED;
		case SEARCH_STATE_FINALIZING:
			return CELL_SEARCH_ERROR_FINALIZING;
		case SEARCH_STATE_IN_PROGRESS:
		case SEARCH_STATE_CANCELING:
			return CELL_SEARCH_ERROR_BUSY;
		default:
			return CELL_SEARCH_ERROR_GENERIC;
	}

	*outSearchId = idm::make<search_object_t>();

	s_search_state = SEARCH_STATE_IN_PROGRESS;
	sysutil_register_cb([=](ppu_thread& ppu) -> s32
	{
		const auto search = fxm::get_always<search_t>();
		const auto content_map = fxm::get_always<ContentIdMap>();
		auto curr_search = idm::get<search_object_t>(*outSearchId);

		vm::var<CellSearchResultParam> resultParam;
		resultParam->searchId = *outSearchId;
		resultParam->resultNum = 0; // Set again later

		std::function<void(std::string&, const std::string&)> searchInFolder = [&, type](std::string& vpath, const std::string& prev)
		{
			std::string relative_vpath = (!prev.empty() ? prev + "/" : "") + vpath;

			for (auto&& item : fs::dir(vfs::get(relative_vpath)))
			{
				item.name = vfs::escape(item.name);
				std::string item_path(relative_vpath + "/" + item.name);

				//if (!(std::strcmp(item.name.c_str(), ".") && std::strcmp(item.name.c_str(), "..")))
				if (item.name.length() < 3) // "." and ".." SHOULD be the only cases, and this is a simpler condition to check
				{
					continue;
				}

				if (item.is_directory)
				{
					searchInFolder(item.name, relative_vpath);
					continue;
				}

				// TODO
				/* Perform first check that file is of desired type. For example, don't wanna go
				 * identifying "AlbumArt.jpg" as an MP3. Hrm... Postpone this thought. Do games
				 * perform their own checks? DIVA ignores anything without the MP3 extension.
				 */

				// TODO - Identify sorting method and insert the appropriate values where applicable

				std::string link_path;
				u64 hash = XXH64(item_path.c_str(), item_path.length(), 0);
				auto found = content_map->find(hash);
				if (found == content_map->end()) // content isn't yet being tracked
				{
					if (!vfs::link(item_path, link_path)) // This is actually the job of cellSearchPrepareFile!!
					{
						cellSearch.error("cellSearchStartContentSearch(): error creating a link!");
						continue;
					}

					auto ext_offset = item.name.find_last_of("."); // used later if no "Title" found

					std::shared_ptr<search_content_t> curr_find = std::make_shared<search_content_t>();
					std::strcpy(curr_find->infoPath.contentPath, link_path.c_str());
					// TODO - curr_find.infoPath.thumbnailPath
					if (type == CELL_SEARCH_CONTENTSEARCHTYPE_MUSIC_ALL)
					{
						curr_find->type = CELL_SEARCH_CONTENTTYPE_MUSIC;
						curr_find->data.reset(new u8[sizeof(CellSearchMusicInfo)]);
						CellSearchMusicInfo& info = *(CellSearchMusicInfo*)curr_find->data.get();
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
						std::strcpy(info.title, item.name.substr(0, ext_offset).c_str()); // it'll do for the moment...
						std::strcpy(info.albumTitle, "ALBUM TITLE"); // std::strcpy(info->albumTitle, ALBUM TITLE );
						std::strcpy(info.artistName, "ARTIST NAME"); // std::strcpy(info->artistName, ARTIST NAME );
						std::strcpy(info.genreName, "GENRE NAME");   // std::strcpy(info->genreName,  GENRE NAME  );
					}
					else if (type == CELL_SEARCH_CONTENTSEARCHTYPE_PHOTO_ALL)
					{
						curr_find->type = CELL_SEARCH_CONTENTTYPE_PHOTO;
						curr_find->data.reset(new u8[sizeof(CellSearchPhotoInfo)]);
						CellSearchPhotoInfo& info = *(CellSearchPhotoInfo*)curr_find->data.get();
						// TODO - Some kinda file photo analysis and assign the values as such
						info.size = item.size;
						info.importedDate = 0;
						info.takenDate = 0;
						info.width = 0;
						info.height = 0;
						info.orientation = 0;  //CellSearchOrientation
						info.codec = 0;        //CellSearchCodec
						info.status = 0;       //CellSearchContentStatus
						std::strcpy(info.title, item.name.substr(0, ext_offset).c_str()); // it'll do for the moment...
						std::strcpy(info.albumTitle, "ALBUM TITLE"); // std::strcpy(info->albumTitle, ALBUM TITLE );
					}
					else if (type == CELL_SEARCH_CONTENTSEARCHTYPE_VIDEO_ALL)
					{
						curr_find->type = CELL_SEARCH_CONTENTTYPE_VIDEO;
						curr_find->data.reset(new u8[sizeof(CellSearchVideoInfo)]);
						CellSearchVideoInfo& info = *(CellSearchVideoInfo*)curr_find->data.get();
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
						std::strcpy(info.title, item.name.substr(0, ext_offset).c_str()); // it'll do for the moment...
						std::strcpy(info.albumTitle, "ALBUM TITLE"); // std::strcpy(info->albumTitle, ALBUM TITLE );
					}

					content_map->emplace(hash, curr_find->get_shared());
					curr_search->content_ids.push_back({hash, curr_find->get_shared()}); // place this file's "ID" into the list of found types

					cellSearch.notice("cellSearchStartContentSearch(): Content ID: %08X   Path: \"%s\"", hash, link_path);
				}
				else // file is already stored and tracked
				{ // TODO
					// Perform checks to see if the identified file has been modified since last checked
					// In which case, update the stored content's properties
					// auto content_found = &content_map->at(content_id);
					curr_search->content_ids.push_back({found->first, found->second->get_shared()});
				}
			}
		};
		searchInFolder(fmt::format("/dev_hdd0/%s", media_dir), "");
		resultParam->resultNum = curr_search->content_ids.size();

		s_search_state = SEARCH_STATE_IDLE;
		search->func(ppu, CELL_SEARCH_EVENT_CONTENTSEARCH_RESULT, CELL_OK, vm::cast(resultParam.addr()), search->userData);
		return CELL_OK;
	});

	return CELL_OK;
}

error_code cellSearchStartSceneSearchInVideo(vm::cptr<CellSearchContentId> videoId, CellSearchSceneSearchType searchType, CellSearchSortOrder sortOrder, vm::ptr<CellSearchId> outSearchId)
{
	cellSearch.todo("cellSearchStartSceneSearchInVideo(videoId=*0x%x, searchType=0x%x, sortOrder=0x%x, outSearchId=*0x%x)", videoId, (u32) searchType, (u32) sortOrder, outSearchId);

	if (!videoId || !outSearchId)
	{
		return CELL_SEARCH_ERROR_PARAM;
	}

	switch(searchType)
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

	switch(s_search_state)
	{
		case SEARCH_STATE_IDLE:
			break;
		case SEARCH_STATE_NOT_INITIALIZED:
		case SEARCH_STATE_INITIALIZING:
			return CELL_SEARCH_ERROR_NOT_INITIALIZED;
		case SEARCH_STATE_FINALIZING:
			return CELL_SEARCH_ERROR_FINALIZING;
		case SEARCH_STATE_IN_PROGRESS:
		case SEARCH_STATE_CANCELING:
			return CELL_SEARCH_ERROR_BUSY;
		default:
			return CELL_SEARCH_ERROR_GENERIC;
	}

	*outSearchId = idm::make<search_object_t>();

	s_search_state = SEARCH_STATE_IN_PROGRESS;
	sysutil_register_cb([=](ppu_thread& ppu) -> s32
	{
		const auto search = fxm::get_always<search_t>();

		vm::var<CellSearchResultParam> resultParam;
		resultParam->searchId = *outSearchId;
		resultParam->resultNum = 0; // TODO

		s_search_state = SEARCH_STATE_IDLE;
		search->func(ppu, CELL_SEARCH_EVENT_SCENESEARCH_INVIDEO_RESULT, CELL_OK, vm::cast(resultParam.addr()), search->userData);
		return CELL_OK;
	});

	return CELL_OK;
}

error_code cellSearchStartSceneSearch(CellSearchSceneSearchType searchType, vm::cptr<char> gameTitle, vm::cpptr<char> tags, u32 tagNum, CellSearchSortKey sortKey, CellSearchSortOrder sortOrder, vm::ptr<CellSearchId> outSearchId)
{
	cellSearch.todo("cellSearchStartSceneSearch(searchType=0x%x, gameTitle=%s, tags=**0x%x, tagNum=0x%x, sortKey=0x%x, sortOrder=0x%x, outSearchId=*0x%x)", (u32) searchType, gameTitle, tags, tagNum, (u32) sortKey, (u32) sortOrder, outSearchId);

	if (!gameTitle || !outSearchId)
	{
		return CELL_SEARCH_ERROR_PARAM;
	}

	switch(searchType)
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

	switch(s_search_state)
	{
		case SEARCH_STATE_IDLE:
			break;
		case SEARCH_STATE_NOT_INITIALIZED:
		case SEARCH_STATE_INITIALIZING:
			return CELL_SEARCH_ERROR_NOT_INITIALIZED;
		case SEARCH_STATE_FINALIZING:
			return CELL_SEARCH_ERROR_FINALIZING;
		case SEARCH_STATE_IN_PROGRESS:
		case SEARCH_STATE_CANCELING:
			return CELL_SEARCH_ERROR_BUSY;
		default:
			return CELL_SEARCH_ERROR_GENERIC;
	}

	*outSearchId = idm::make<search_object_t>();

	s_search_state = SEARCH_STATE_IN_PROGRESS;
	sysutil_register_cb([=](ppu_thread& ppu) -> s32
	{
		const auto search = fxm::get_always<search_t>();

		vm::var<CellSearchResultParam> resultParam;
		resultParam->searchId = *outSearchId;
		resultParam->resultNum = 0; // TODO

		s_search_state = SEARCH_STATE_IDLE;
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

	switch(s_search_state)
	{
		case SEARCH_STATE_IDLE:
			break;
		case SEARCH_STATE_NOT_INITIALIZED:
		case SEARCH_STATE_INITIALIZING:
			return CELL_SEARCH_ERROR_NOT_INITIALIZED;
		case SEARCH_STATE_FINALIZING:
			return CELL_SEARCH_ERROR_FINALIZING;
		case SEARCH_STATE_IN_PROGRESS:
		case SEARCH_STATE_CANCELING:
			return CELL_SEARCH_ERROR_BUSY;
		default:
			return CELL_SEARCH_ERROR_GENERIC;
	}

	if (offset > -1 && offset < searchObject->content_ids.size())
	{
		auto& content_id = searchObject->content_ids.at(offset);
		auto& content_info = content_id.second;
		switch(content_info->type)
		{
			case CELL_SEARCH_CONTENTTYPE_MUSIC:
				std::memcpy(infoBuffer.get_ptr(), (void*)content_info->data.get(), sizeof(CellSearchMusicInfo));
				break;
			case CELL_SEARCH_CONTENTTYPE_PHOTO:
				std::memcpy(infoBuffer.get_ptr(), (void*)content_info->data.get(), sizeof(CellSearchPhotoInfo));
				break;
			case CELL_SEARCH_CONTENTTYPE_VIDEO:
				std::memcpy(infoBuffer.get_ptr(), (void*)content_info->data.get(), sizeof(CellSearchVideoInfo));
				break;
			case CELL_SEARCH_CONTENTTYPE_MUSICLIST:
				std::memcpy(infoBuffer.get_ptr(), (void*)content_info->data.get(), sizeof(CellSearchMusicListInfo));
				break;
			case CELL_SEARCH_CONTENTTYPE_PHOTOLIST:
				std::memcpy(infoBuffer.get_ptr(), (void*)content_info->data.get(), sizeof(CellSearchPhotoListInfo));
				break;
			case CELL_SEARCH_CONTENTTYPE_VIDEOLIST:
				std::memcpy(infoBuffer.get_ptr(), (void*)content_info->data.get(), sizeof(CellSearchVideoListInfo));
				break;
			case CELL_SEARCH_CONTENTTYPE_SCENE:
				std::memcpy(infoBuffer.get_ptr(), (void*)content_info->data.get(), sizeof(CellSearchVideoSceneInfo));
				break;
			default:
				return CELL_SEARCH_ERROR_GENERIC;
		}

		*outContentType = content_info->type;
		std::memcpy((void*)outContentId->data, (void*)content_id.first, CELL_SEARCH_CONTENT_ID_SIZE);
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

	switch(s_search_state)
	{
		case SEARCH_STATE_IDLE:
			break;
		case SEARCH_STATE_NOT_INITIALIZED:
		case SEARCH_STATE_INITIALIZING:
			return CELL_SEARCH_ERROR_NOT_INITIALIZED;
		case SEARCH_STATE_FINALIZING:
			return CELL_SEARCH_ERROR_FINALIZING;
		case SEARCH_STATE_IN_PROGRESS:
		case SEARCH_STATE_CANCELING:
			return CELL_SEARCH_ERROR_BUSY;
		default:
			return CELL_SEARCH_ERROR_GENERIC;
	}

	const auto content_map = fxm::get_always<ContentIdMap>();
	auto found = content_map->find(*(u64*)contentId->data);
	if (found != content_map->end())
	{
		auto content_info = found->second;
		switch (content_info->type)
		{
			case CELL_SEARCH_CONTENTTYPE_MUSIC:
				std::memcpy(infoBuffer.get_ptr(), (void*)content_info->data.get(), sizeof(CellSearchMusicInfo));
				break;
			case CELL_SEARCH_CONTENTTYPE_PHOTO:
				std::memcpy(infoBuffer.get_ptr(), (void*)content_info->data.get(), sizeof(CellSearchPhotoInfo));
				break;
			case CELL_SEARCH_CONTENTTYPE_VIDEO:
				std::memcpy(infoBuffer.get_ptr(), (void*)content_info->data.get(), sizeof(CellSearchVideoInfo));
				break;
			case CELL_SEARCH_CONTENTTYPE_MUSICLIST:
				std::memcpy(infoBuffer.get_ptr(), (void*)content_info->data.get(), sizeof(CellSearchMusicListInfo));
				break;
			case CELL_SEARCH_CONTENTTYPE_PHOTOLIST:
				std::memcpy(infoBuffer.get_ptr(), (void*)content_info->data.get(), sizeof(CellSearchPhotoListInfo));
				break;
			case CELL_SEARCH_CONTENTTYPE_VIDEOLIST:
				std::memcpy(infoBuffer.get_ptr(), (void*)content_info->data.get(), sizeof(CellSearchVideoListInfo));
				break;
			case CELL_SEARCH_CONTENTTYPE_SCENE:
				std::memcpy(infoBuffer.get_ptr(), (void*)content_info->data.get(), sizeof(CellSearchVideoSceneInfo));
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

	switch(s_search_state)
	{
		case SEARCH_STATE_IDLE:
			break;
		case SEARCH_STATE_NOT_INITIALIZED:
		case SEARCH_STATE_INITIALIZING:
			return CELL_SEARCH_ERROR_NOT_INITIALIZED;
		case SEARCH_STATE_FINALIZING:
			return CELL_SEARCH_ERROR_FINALIZING;
		case SEARCH_STATE_IN_PROGRESS:
		case SEARCH_STATE_CANCELING:
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
	u64 content_hash = *(u64*)contentId->data;
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

	switch(s_search_state)
	{
		case SEARCH_STATE_IDLE:
			break;
		case SEARCH_STATE_NOT_INITIALIZED:
		case SEARCH_STATE_INITIALIZING:
			return CELL_SEARCH_ERROR_NOT_INITIALIZED;
		case SEARCH_STATE_FINALIZING:
			return CELL_SEARCH_ERROR_FINALIZING;
		case SEARCH_STATE_IN_PROGRESS:
		case SEARCH_STATE_CANCELING:
			return CELL_SEARCH_ERROR_BUSY;
		default:
			return CELL_SEARCH_ERROR_GENERIC;
	}

	if (offset > -1 && offset < searchObject->content_ids.size())
	{
		auto& content_id = searchObject->content_ids.at(offset);
		*outContentType = content_id.second->type;
		std::memcpy((void*)outContentId->data, (void*)content_id.first, CELL_SEARCH_CONTENT_ID_SIZE);

		if (outTimeInfo)
		{
			std::memcpy(outTimeInfo.get_ptr(), (void*)&content_id.second->timeInfo, sizeof(CellSearchTimeInfo));
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
	cellSearch.todo("cellSearchGetMusicSelectionContext(searchId=0x%x, contentId=*0x%x, repeatMode=0x%x, option=0x%x, outContext=*0x%x)", searchId, contentId, (u32)repeatMode, (u32)option, outContext);

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

	switch(s_search_state)
	{
		case SEARCH_STATE_IDLE:
			break;
		case SEARCH_STATE_NOT_INITIALIZED:
		case SEARCH_STATE_INITIALIZING:
			return CELL_SEARCH_ERROR_NOT_INITIALIZED;
		case SEARCH_STATE_FINALIZING:
			return CELL_SEARCH_ERROR_FINALIZING;
		case SEARCH_STATE_IN_PROGRESS:
		case SEARCH_STATE_CANCELING:
			return CELL_SEARCH_ERROR_BUSY;
		default:
			return CELL_SEARCH_ERROR_GENERIC;
	}

	u64 id = *(u64*)contentId->data;
	const auto content_map  = fxm::get_always<ContentIdMap>();
	if(content_map->find(id) != content_map->end())
	{
		auto found = content_map->at(id);
		std::memcpy(infoPath.get_ptr(), (void*)&found->infoPath, sizeof(CellSearchContentInfoPath));
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

	switch(s_search_state)
	{
		case SEARCH_STATE_IDLE:
			break;
		case SEARCH_STATE_NOT_INITIALIZED:
		case SEARCH_STATE_INITIALIZING:
			return CELL_SEARCH_ERROR_NOT_INITIALIZED;
		case SEARCH_STATE_FINALIZING:
			return CELL_SEARCH_ERROR_FINALIZING;
		case SEARCH_STATE_IN_PROGRESS:
		case SEARCH_STATE_CANCELING:
			return CELL_SEARCH_ERROR_BUSY;
		default:
			return CELL_SEARCH_ERROR_GENERIC;
	}

	// 
	// Do nothing. The behaviour here is fulfilled during cellSearchStartContentSearch
	// 

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

	switch(s_search_state)
	{
		case SEARCH_STATE_IN_PROGRESS:
			break;
		case SEARCH_STATE_NOT_INITIALIZED:
		case SEARCH_STATE_INITIALIZING:
			return CELL_SEARCH_ERROR_NOT_INITIALIZED;
		case SEARCH_STATE_FINALIZING:
			return CELL_SEARCH_ERROR_FINALIZING;
		case SEARCH_STATE_CANCELING:
			return CELL_SEARCH_ERROR_BUSY;
		case SEARCH_STATE_IDLE:
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

	switch(s_search_state)
	{
		case SEARCH_STATE_IDLE:
			break;
		case SEARCH_STATE_NOT_INITIALIZED:
			return CELL_SEARCH_ERROR_NOT_INITIALIZED;
		case SEARCH_STATE_FINALIZING:
			return CELL_SEARCH_ERROR_FINALIZING;
		case SEARCH_STATE_IN_PROGRESS:
		case SEARCH_STATE_INITIALIZING:
		case SEARCH_STATE_CANCELING:
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

DECLARE(ppu_module_manager::cellSearch)
("cellSearchUtility", []()
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
