

typedef struct {
    HINSTANCE havcodec;
    void (__cdecl* avcodec_init)(void);
    void (__cdecl* avcodec_register_all)(void);
    AVCodec *(__cdecl* avcodec_find_encoder)(enum CodecID id);
    AVCodecContext *(__cdecl* avcodec_alloc_context)(void);
    AVFrame *(__cdecl* avcodec_alloc_frame)(void);
    int (__cdecl* avcodec_open)(AVCodecContext *avctx, AVCodec *codec);
    int (__cdecl* img_convert)(AVPicture *dst, int dst_pix_fmt, AVPicture *src, int pix_fmt, int width, int height);
    int (__cdecl* avcodec_encode_video)(AVCodecContext *avctx, uint8_t *buf, int buf_size, const AVFrame *pict);
    int (__cdecl* avcodec_close)(AVCodecContext *avctx);
} RECPLUGIN;

extern RECPLUGIN recplugin;


