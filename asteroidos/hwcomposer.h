
#ifdef __cplusplus
extern "C"
{
#endif


extern uint32_t screen_width;
extern uint32_t screen_height;

extern EGLDisplay display;
extern EGLSurface surface;
extern EGLContext context;

void hwcomposer_init();
#ifdef __cplusplus
}
#endif