
#ifdef __cplusplus
extern "C"
{
#endif


void video_init(uint32_t width,uint32_t height, uint32_t f);
void video_close();
void video_draw(uint16_t *pixels);
#ifdef __cplusplus
}
#endif