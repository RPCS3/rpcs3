#define RSX(ctx) ctx->rsxthr
#define REGS(ctx) ctx->register_state
#define RSX_CAPTURE_EVENT(name) if (RSX(ctx)->capture_current_frame) { RSX(ctx)->capture_frame(name); }
