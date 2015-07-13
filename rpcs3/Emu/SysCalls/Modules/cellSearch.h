// Error Codes
enum
{
	CELL_SEARCH_OK                          = 0,
	CELL_SEARCH_CANCELED                    = 1,
	CELL_SEARCH_ERROR_PARAM                 = 0x8002C801,
	CELL_SEARCH_ERROR_BUSY                  = 0x8002C802,
	CELL_SEARCH_ERROR_NO_MEMORY             = 0x8002C803,
	CELL_SEARCH_ERROR_UNKNOWN_MODE          = 0x8002C804,
	CELL_SEARCH_ERROR_ALREADY_INITIALIZED   = 0x8002C805,
	CELL_SEARCH_ERROR_NOT_INITIALIZED       = 0x8002C806,
	CELL_SEARCH_ERROR_FINALIZING            = 0x8002C807,
	CELL_SEARCH_ERROR_NOT_SUPPORTED_SEARCH  = 0x8002C808,
	CELL_SEARCH_ERROR_CONTENT_OBSOLETE      = 0x8002C809,
	CELL_SEARCH_ERROR_CONTENT_NOT_FOUND     = 0x8002C80A,
	CELL_SEARCH_ERROR_NOT_LIST              = 0x8002C80B,
	CELL_SEARCH_ERROR_OUT_OF_RANGE          = 0x8002C80C,
	CELL_SEARCH_ERROR_INVALID_SEARCHID      = 0x8002C80D,
	CELL_SEARCH_ERROR_ALREADY_GOT_RESULT    = 0x8002C80E,
	CELL_SEARCH_ERROR_NOT_SUPPORTED_CONTEXT = 0x8002C80F,
	CELL_SEARCH_ERROR_INVALID_CONTENTTYPE   = 0x8002C810,
	CELL_SEARCH_ERROR_DRM                   = 0x8002C811,
	CELL_SEARCH_ERROR_TAG                   = 0x8002C812,
	CELL_SEARCH_ERROR_GENERIC               = 0x8002C8FF,
};

// Constants
enum
{
	CELL_SEARCH_CONTENT_ID_SIZE       = 16,
	CELL_SEARCH_TITLE_LEN_MAX         = 384,
	CELL_SEARCH_TAG_NUM_MAX           = 6,
	CELL_SEARCH_TAG_LEN_MAX           = 63,
	CELL_MUSIC_SELECTION_CONTEXT_SIZE = 2048,
	CELL_SEARCH_PATH_LEN_MAX          = 63,
	CELL_SEARCH_MTOPTION_LEN_MAX      = 63,
};

// Sort keys
enum
{
	CELL_SEARCH_SORTKEY_NONE         = 0,
	CELL_SEARCH_SORTKEY_DEFAULT      = 1,
	CELL_SEARCH_SORTKEY_TITLE        = 2,
	CELL_SEARCH_SORTKEY_ALBUMTITLE   = 3,
	CELL_SEARCH_SORTKEY_GENRENAME    = 4,
	CELL_SEARCH_SORTKEY_ARTISTNAME   = 5,
	CELL_SEARCH_SORTKEY_IMPORTEDDATE = 6,
	CELL_SEARCH_SORTKEY_TRACKNUMBER  = 7,
	CELL_SEARCH_SORTKEY_TAKENDATE    = 8,
	CELL_SEARCH_SORTKEY_USERDEFINED  = 9,
	CELL_SEARCH_SORTKEY_MODIFIEDDATE = 10,
};

// Sort order
enum
{
	CELL_SEARCH_SORTORDER_NONE       = 0,
	CELL_SEARCH_SORTORDER_ASCENDING  = 1,
	CELL_SEARCH_SOFTORDER_DESCENDING = 2,
};

// Content types
enum
{
	CELL_SEARCH_CONTENTTYPE_NONE      = 0,
	CELL_SEARCH_CONTENTTYPE_MUSIC     = 1,
	CELL_SEARCH_CONTENTTYPE_MUSICLIST = 2,
	CELL_SEARCH_CONTENTTYPE_PHOTO     = 3,
	CELL_SEARCH_CONTENTTYPE_PHOTOLIST = 4,
	CELL_SEARCH_CONTENTTYPE_VIDEO     = 5,
	CELL_SEARCH_CONTENTTYPE_VIDEOLIST = 6,
	CELL_SEARCH_CONTENTTYPE_SCENE     = 7,
};

// Codecs
enum CellSearchCodec
{
	CELL_SEARCH_CODEC_UNKNOWN      = 0,
	CELL_SEARCH_CODEC_MPEG2        = 1,
	CELL_SEARCH_CODEC_MPEG4        = 2,
	CELL_SEARCH_CODEC_AVC          = 3,
	CELL_SEARCH_CODEC_MPEG1        = 4,
	CELL_SEARCH_CODEC_AT3          = 5,
	CELL_SEARCH_CODEC_AT3P         = 6,
	CELL_SEARCH_CODEC_ATALL        = 7,
	CELL_SEARCH_CODEC_MP3          = 8,
	CELL_SEARCH_CODEC_AAC          = 9,
	CELL_SEARCH_CODEC_LPCM         = 10,
	CELL_SEARCH_CODEC_WAV          = 11,
	CELL_SEARCH_CODEC_WMA          = 12,
	CELL_SEARCH_CODEC_JPEG         = 13,
	CELL_SEARCH_CODEC_PNG          = 14,
	CELL_SEARCH_CODEC_TIFF         = 15,
	CELL_SEARCH_CODEC_BMP          = 16,
	CELL_SEARCH_CODEC_GIF          = 17,
	CELL_SEARCH_CODEC_MPEG2_TS     = 18,
	CELL_SEARCH_CODEC_DSD          = 19,
	CELL_SEARCH_CODEC_AC3          = 20,
	CELL_SEARCH_CODEC_MPEG1_LAYER1 = 21,
	CELL_SEARCH_CODEC_MPEG1_LAYER2 = 22,
	CELL_SEARCH_CODEC_MPEG1_LAYER3 = 23,
	CELL_SEARCH_CODEC_MPEG2_LAYER1 = 24,
	CELL_SEARCH_CODEC_MPEG2_LAYER2 = 25,
	CELL_SEARCH_CODEC_MPEG2_LAYER3 = 26,
	CELL_SEARCH_CODEC_MOTIONJPEG   = 27,
	CELL_SEARCH_CODEC_MPO          = 28,
};

// Scene types
enum CellSearchSceneType
{
	CELL_SEARCH_SCENETYPE_NONE           = 0,
	CELL_SEARCH_SCENETYPE_CHAPTER        = 1,
	CELL_SEARCH_SCENETYPE_CLIP_HIGHLIGHT = 2,
	CELL_SEARCH_SCENETYPE_CLIP_USER      = 3,
};

// List types
enum CellSearchListType
{
	CELL_SEARCH_LISTTYPE_MUSIC_ALBUM    = 1,
	CELL_SEARCH_LISTTYPE_MUSIC_GENRE    = 2,
	CELL_SEARCH_LISTTYPE_MUSIC_ARTIST   = 3,
	CELL_SEARCH_LISTTYPE_MUSIC_PLAYLIST = 9,
};

// Content status
enum CellSearchContentStatus
{
	CELL_SEARCH_CONTENTSTATUS_AVAILABLE,
	CELL_SEARCH_CONTENTSTATUS_NOT_SUPPORTED,
	CELL_SEARCH_CONTENTSTATUS_BROKEN,
};

// Search orientation
enum CellSearchOrientation
{
	CELL_SEARCH_ORIENTATION_UNKNOWN,
	CELL_SEARCH_ORIENTATION_TOP_LEFT,
	CELL_SEARCH_ORIENTATION_TOP_RIGHT,
	CELL_SEARCH_ORIENTATION_BOTTOM_RIGHT,
	CELL_SEARCH_ORIENTATION_BOTTOM_LEFT,
};

// Search modes
enum CellSearchMode
{
	CELL_SEARCH_MODE_NORMAL = 0,
};

// Search events
enum CellSearchEvent
{
	CELL_SEARCH_EVENT_NOTIFICATION = 0,
	CELL_SEARCH_EVENT_INITIALIZE_RESULT,
	CELL_SEARCH_EVENT_FINALIZE_RESULT,
	CELL_SEARCH_EVENT_LISTSEARCH_RESULT,
	CELL_SEARCH_EVENT_CONTENTSEARCH_INLIST_RESULT,
	CELL_SEARCH_EVENT_CONTENTSEARCH_RESULT,
	CELL_SEARCH_EVENT_SCENESEARCH_INVIDEO_RESULT,
	CELL_SEARCH_EVENT_SCENESEARCH_RESULT,
};

typedef void(CellSearchSystemCallback)(CellSearchEvent event, s32 result, vm::ptr<const u32> param, vm::ptr<u32> userData);

struct CellSearchContentId
{
	char data[CELL_SEARCH_CONTENT_ID_SIZE];
};

struct CellSearchResultParam
{
	be_t<s32> searchId;
	be_t<u32> resultNum;
};

struct CellSearchMusicListInfo
{
	be_t<CellSearchListType> listType;
	be_t<u32> numOfItems;
	be_t<s64> duration;
	char title[CELL_SEARCH_TITLE_LEN_MAX + 1];
	char reserved[3];
	char artistName[CELL_SEARCH_TITLE_LEN_MAX + 1];
	char reserved2[3];
};

struct CellSearchPhotoListInfo
{
	be_t<CellSearchListType> listType;
	be_t<u32> numOfItems;
	char title[CELL_SEARCH_TITLE_LEN_MAX + 1];
	char reserved[3];
};

struct CellSearchVideoListInfo
{
	be_t<CellSearchListType> listType;
	be_t<u32> numOfItems;
	be_t<s64> duration;
	char title[CELL_SEARCH_TITLE_LEN_MAX + 1];
	char reserved[3];
};

struct CellSearchMusicInfo
{
	be_t<s64> duration;
	be_t<s64> size;
	be_t<s64> importedDate;
	be_t<s64> lastPlayedDate;
	be_t<s32> releasedYear;
	be_t<s32> trackNumber;
	be_t<s32> bitrate;
	be_t<s32> samplingRate;
	be_t<s32> quantizationBitrate;
	be_t<s32> playCount;
	be_t<s32> drmEncrypted;
	be_t<CellSearchCodec> codec;
	be_t<CellSearchContentStatus> status;
	char diskNumber[8];
	char title[CELL_SEARCH_TITLE_LEN_MAX + 1];
	char reserved[3];
	char albumTitle[CELL_SEARCH_TITLE_LEN_MAX + 1];
	char reserved2[3];
	char artistName[CELL_SEARCH_TITLE_LEN_MAX + 1];
	char reserved3[3];
	char genreName[CELL_SEARCH_TITLE_LEN_MAX + 1];
	char reserved4[3];
};

struct CellSearchPhotoInfo
{
	be_t<s64> size;
	be_t<s64> importedDate;
	be_t<s64> takenDate;
	be_t<s32> width;
	be_t<s32> height;
	be_t<CellSearchOrientation> orientation;
	be_t<CellSearchCodec> codec;
	be_t<CellSearchContentStatus> status;
	char title[CELL_SEARCH_TITLE_LEN_MAX + 1];
	char reserved[3];
	char albumTitle[CELL_SEARCH_TITLE_LEN_MAX + 1];
	char reserved2[3];
};

struct CellSearchVideoInfo
{
	be_t<s64> duration;
	be_t<s64> size;
	be_t<s64> importedDate;
	be_t<s64> takenDate;
	be_t<s32> videoBitrate;
	be_t<s32> audioBitrate;
	be_t<s32> playCount;
	be_t<s32> drmEncrypted;
	be_t<CellSearchCodec> videoCodec;
	be_t<CellSearchCodec> audioCodec;
	be_t<CellSearchContentStatus> status;
	char title[CELL_SEARCH_TITLE_LEN_MAX + 1];
	char reserved[3];
	char albumTitle[CELL_SEARCH_TITLE_LEN_MAX + 1];
	char reserved2[3];
};

struct CellSearchVideoSceneInfo
{
	be_t<CellSearchSceneType> sceneType;
	be_t<s64> startTime_ms;
	be_t<s64> endTime_ms;
	CellSearchContentId videoId;
	char title[CELL_SEARCH_TITLE_LEN_MAX + 1];
	char reserved[3];
	char tags[CELL_SEARCH_TAG_NUM_MAX][CELL_SEARCH_TAG_LEN_MAX];
};

struct CellMusicSelectionContext
{
	char data[CELL_MUSIC_SELECTION_CONTEXT_SIZE];
};

struct CellSearchContentInfoPath
{
	char contentPath[CELL_SEARCH_PATH_LEN_MAX + 1];
	char thumbnailPath[CELL_SEARCH_PATH_LEN_MAX + 1];
};

struct CellSearchContentInfoPathMovieThumb
{
	char movieThumbnailPath[CELL_SEARCH_PATH_LEN_MAX + 1];
	char movieThumbnailOption[CELL_SEARCH_MTOPTION_LEN_MAX + 1];
};

struct CellSearchTimeInfo
{
	be_t<s64> takenDate;
	be_t<s64> importedDate;
	be_t<s64> modifiedDate;
};