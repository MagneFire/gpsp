/*

This file is based on Portable ZX-Spectrum emulator.
Copyright (C) 2001-2012 SMT, Dexus, Alone Coder, deathsoft, djdron, scor
	
C++ to C code conversion by Pate

Modified by DPR for gpsp for Raspberry Pi

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.

*/

#include "GLES/gl.h"
#include "EGL/egl.h"
#include "EGL/eglext.h"
#include "GLES2/gl2.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include <unistd.h>
#include <hwcomposer_window.h>
#include <hardware/hardware.h>
#include <hardware/hwcomposer.h>
#include <malloc.h>
#include <sync/sync.h>

#include "gles_video.h"

static uint32_t frame_width = 0;
static uint32_t frame_height = 0;


#define	SHOW_ERROR		gles_show_error();

static void SetOrtho(GLfloat m[4][4], GLfloat left, GLfloat right, GLfloat bottom, GLfloat top, GLfloat near, GLfloat far, GLfloat scale_x, GLfloat scale_y);

static const char* vertex_shader =
	"uniform mat4 u_vp_matrix;								\n"
	"attribute vec4 a_position;								\n"
	"attribute vec2 a_texcoord;								\n"
	"varying mediump vec2 v_texcoord;						\n"
	"void main()											\n"
	"{														\n"
	"	v_texcoord = a_texcoord;							\n"
	"	gl_Position = u_vp_matrix * a_position;				\n"
	"}														\n";

static const char* fragment_shader =
	"varying mediump vec2 v_texcoord;						\n"
	"uniform sampler2D u_texture;							\n"
	"void main()											\n"
	"{														\n"
	"	gl_FragColor = texture2D(u_texture, v_texcoord);	\n"
	"}														\n";
/*
static const GLfloat vertices[] =
{
	-0.5f, -0.5f, 0.0f,
	+0.5f, -0.5f, 0.0f,
	+0.5f, +0.5f, 0.0f,
	-0.5f, +0.5f, 0.0f,
};
*/
static const GLfloat vertices[] =
{
	-0.5f, -0.5f, 0.0f,
	-0.5f, +0.5f, 0.0f,
	+0.5f, +0.5f, 0.0f,
	+0.5f, -0.5f, 0.0f,
};

#define	TEX_WIDTH	1024
#define	TEX_HEIGHT	512

static GLfloat uvs[8];

static const GLushort indices[] =
{
	0, 1, 2,
	0, 2, 3,
};

static const int kVertexCount = 4;
static const int kIndexCount = 6;

class HWComposer : public HWComposerNativeWindow
{
	private:
		hwc_layer_1_t *fblayer;
		hwc_composer_device_1_t *hwcdevice;
		hwc_display_contents_1_t **mlist;
	protected:
		void present(HWComposerNativeWindowBuffer *buffer);

	public:

		HWComposer(unsigned int width, unsigned int height, unsigned int format, hwc_composer_device_1_t *device, hwc_display_contents_1_t **mList, hwc_layer_1_t *layer);
		void set();	
};

HWComposer::HWComposer(unsigned int width, unsigned int height, unsigned int format, hwc_composer_device_1_t *device, hwc_display_contents_1_t **mList, hwc_layer_1_t *layer) : HWComposerNativeWindow(width, height, format)
{
	fblayer = layer;
	hwcdevice = device;
	mlist = mList;
}

void HWComposer::present(HWComposerNativeWindowBuffer *buffer)
{
	int oldretire = mlist[0]->retireFenceFd;
	mlist[0]->retireFenceFd = -1;
	fblayer->handle = buffer->handle;
	fblayer->acquireFenceFd = getFenceBufferFd(buffer);
	fblayer->releaseFenceFd = -1;
	int err = hwcdevice->prepare(hwcdevice, HWC_NUM_DISPLAY_TYPES, mlist);
	assert(err == 0);

	err = hwcdevice->set(hwcdevice, HWC_NUM_DISPLAY_TYPES, mlist);
	// in android surfaceflinger ignores the return value as not all display types may be supported
	setFenceBufferFd(buffer, fblayer->releaseFenceFd);

	if (oldretire != -1)
	{   
		//sync_wait(oldretire, -1);
		close(oldretire);
	}
} 

inline static uint32_t interpreted_version(hw_device_t *hwc_device)
{
	uint32_t version = hwc_device->version;

	if ((version & 0xffff0000) == 0) {
		// Assume header version is always 1
		uint32_t header_version = 1;

		// Legacy version encoding
		version = (version << 16) | header_version;
	}
	return version;
}

void Create_uvs(GLfloat * matrix, GLfloat max_u, GLfloat max_v) {
    memset(matrix,0,sizeof(GLfloat)*8);
    matrix[3]=max_v;
    matrix[4]=max_u;
    matrix[5]=max_v;
    matrix[6]=max_u;

}

void gles_show_error()
{
	GLenum error = GL_NO_ERROR;
    error = glGetError();
    if (GL_NO_ERROR != error)
        printf("GL Error %x encountered!\n", error);
}

static GLuint CreateShader(GLenum type, const char *shader_src)
{
	GLuint shader = glCreateShader(type);
	if(!shader)
		return 0;

	// Load and compile the shader source
	glShaderSource(shader, 1, &shader_src, NULL);
	glCompileShader(shader);

	// Check the compile status
	GLint compiled = 0;
	glGetShaderiv(shader, GL_COMPILE_STATUS, &compiled);
	if(!compiled)
	{
		GLint info_len = 0;
		glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &info_len);
		if(info_len > 1)
		{
			char* info_log = (char *)malloc(sizeof(char) * info_len);
			glGetShaderInfoLog(shader, info_len, NULL, info_log);
			// TODO(dspringer): We could really use a logging API.
			printf("Error compiling shader:\n%s\n", info_log);
			free(info_log);
		}
		glDeleteShader(shader);
		return 0;
	}
	return shader;
}

static GLuint CreateProgram(const char *vertex_shader_src, const char *fragment_shader_src)
{
	GLuint vertex_shader = CreateShader(GL_VERTEX_SHADER, vertex_shader_src);
	if(!vertex_shader)
		return 0;
	GLuint fragment_shader = CreateShader(GL_FRAGMENT_SHADER, fragment_shader_src);
	if(!fragment_shader)
	{
		glDeleteShader(vertex_shader);
		return 0;
	}

	GLuint program_object = glCreateProgram();
	if(!program_object)
		return 0;
	glAttachShader(program_object, vertex_shader);
	glAttachShader(program_object, fragment_shader);

	// Link the program
	glLinkProgram(program_object);

	// Check the link status
	GLint linked = 0;
	glGetProgramiv(program_object, GL_LINK_STATUS, &linked);
	if(!linked)
	{
		GLint info_len = 0;
		glGetProgramiv(program_object, GL_INFO_LOG_LENGTH, &info_len);
		if(info_len > 1)
		{
			char* info_log = (char *)malloc(info_len);
			glGetProgramInfoLog(program_object, info_len, NULL, info_log);
			// TODO(dspringer): We could really use a logging API.
			printf("Error linking program:\n%s\n", info_log);
			free(info_log);
		}
		glDeleteProgram(program_object);
		return 0;
	}
	// Delete these here because they are attached to the program object.
	glDeleteShader(vertex_shader);
	glDeleteShader(fragment_shader);
	return program_object;
}

typedef	struct ShaderInfo {
		GLuint program;
		GLint a_position;
		GLint a_texcoord;
		GLint u_vp_matrix;
		GLint u_texture;
} ShaderInfo;

static ShaderInfo shader;
static ShaderInfo shader_filtering;
static GLuint buffers[3];
static GLuint textures[2];


static void gles2_create()
{
	memset(&shader, 0, sizeof(ShaderInfo));
	shader.program = CreateProgram(vertex_shader, fragment_shader);
	if(shader.program)
	{
		shader.a_position	= glGetAttribLocation(shader.program,	"a_position");
		shader.a_texcoord	= glGetAttribLocation(shader.program,	"a_texcoord");
		shader.u_vp_matrix	= glGetUniformLocation(shader.program,	"u_vp_matrix");
		shader.u_texture	= glGetUniformLocation(shader.program,	"u_texture");
	}
	glGenTextures(1, textures);
	glBindTexture(GL_TEXTURE_2D, textures[0]);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, TEX_WIDTH, TEX_HEIGHT, 0, GL_RGB, GL_UNSIGNED_SHORT_5_6_5, NULL);

	Create_uvs(uvs, (float)frame_width/TEX_WIDTH, (float)frame_height/TEX_HEIGHT);

	glGenBuffers(3, buffers);
	glBindBuffer(GL_ARRAY_BUFFER, buffers[0]);
	glBufferData(GL_ARRAY_BUFFER, kVertexCount * sizeof(GLfloat) * 3, vertices, GL_STATIC_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, buffers[1]);
	glBufferData(GL_ARRAY_BUFFER, kVertexCount * sizeof(GLfloat) * 2, uvs, GL_STATIC_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, buffers[2]);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, kIndexCount * sizeof(GL_UNSIGNED_SHORT), indices, GL_STATIC_DRAW);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
	glDisable(GL_DEPTH_TEST);
	glDisable(GL_BLEND);
	glDisable(GL_DITHER);
}

static uint32_t screen_width = 0;
static uint32_t screen_height = 0;

static EGLDisplay display = NULL;
static EGLSurface surface = NULL;
static EGLContext context = NULL;
static HWComposer *win = NULL;

static GLfloat proj[4][4];
static GLint filter_min;
static GLint filter_mag;

void hwcomposer_init() {
	EGLConfig ecfg;
	EGLint num_config;
	EGLint attr[] = {       // some attributes to set up our egl-interface
		EGL_BUFFER_SIZE, 32,
		EGL_RENDERABLE_TYPE,
		EGL_OPENGL_ES2_BIT,
		EGL_NONE
	};
	EGLint ctxattr[] = {
		EGL_CONTEXT_CLIENT_VERSION, 2,
		EGL_NONE
	};

	EGLBoolean rv;

	int err;	

        hw_module_t const* module = NULL;
        err = hw_get_module(GRALLOC_HARDWARE_MODULE_ID, &module);
        assert(err == 0);
        //framebuffer_device_t* fbDev = NULL;
        //framebuffer_open(module, &fbDev);

	hw_module_t *hwcModule = 0;
	hwc_composer_device_1_t *hwcDevicePtr = 0;

	err = hw_get_module(HWC_HARDWARE_MODULE_ID, (const hw_module_t **) &hwcModule);
	assert(err == 0);
	
	err = hwc_open_1(hwcModule, &hwcDevicePtr);
	assert(err == 0);

	hw_device_t *hwcDevice = &hwcDevicePtr->common;

	uint32_t hwc_version = interpreted_version(hwcDevice);

#ifdef HWC_DEVICE_API_VERSION_1_4
	if (hwc_version == HWC_DEVICE_API_VERSION_1_4) {
		hwcDevicePtr->setPowerMode(hwcDevicePtr, 0, HWC_POWER_MODE_NORMAL);
	} else
#endif
#ifdef HWC_DEVICE_API_VERSION_1_5
	if (hwc_version == HWC_DEVICE_API_VERSION_1_5) {
		hwcDevicePtr->setPowerMode(hwcDevicePtr, 0, HWC_POWER_MODE_NORMAL);
	} else
#endif
		hwcDevicePtr->blank(hwcDevicePtr, 0, 0);

	uint32_t configs[5];
	size_t numConfigs = 5;

	err = hwcDevicePtr->getDisplayConfigs(hwcDevicePtr, 0, configs, &numConfigs);
	assert (err == 0);

	int32_t attr_values[2];
	uint32_t attributes[] = { HWC_DISPLAY_WIDTH, HWC_DISPLAY_HEIGHT, HWC_DISPLAY_NO_ATTRIBUTE }; 

	hwcDevicePtr->getDisplayAttributes(hwcDevicePtr, 0,
			configs[0], attributes, attr_values);

	printf("width: %i height: %i\n", attr_values[0], attr_values[1]);

	size_t size = sizeof(hwc_display_contents_1_t) + 2 * sizeof(hwc_layer_1_t);
	hwc_display_contents_1_t *list = (hwc_display_contents_1_t *) malloc(size);
	hwc_display_contents_1_t **mList = (hwc_display_contents_1_t **) malloc(HWC_NUM_DISPLAY_TYPES * sizeof(hwc_display_contents_1_t *));
	const hwc_rect_t r = { 0, 0, attr_values[0], attr_values[1] };

	int counter = 0;
	for (; counter < HWC_NUM_DISPLAY_TYPES; counter++)
		mList[counter] = NULL;

	// Assign the layer list only to the first display,
	// otherwise HWC might freeze if others are disconnected
	mList[0] = list;

	hwc_layer_1_t *layer = &list->hwLayers[0];
	memset(layer, 0, sizeof(hwc_layer_1_t));
	layer->compositionType = HWC_FRAMEBUFFER;
	layer->hints = 0;
	layer->flags = 0;
	layer->handle = 0;
	layer->transform = 0;
	layer->blending = HWC_BLENDING_NONE;
#ifdef HWC_DEVICE_API_VERSION_1_3
	layer->sourceCropf.top = 0.0f;
	layer->sourceCropf.left = 0.0f;
	layer->sourceCropf.bottom = (float) attr_values[1];
	layer->sourceCropf.right = (float) attr_values[0];
#else
	layer->sourceCrop = r;
#endif
	layer->displayFrame = r;
	layer->visibleRegionScreen.numRects = 1;
	layer->visibleRegionScreen.rects = &layer->displayFrame;
	layer->acquireFenceFd = -1;
	layer->releaseFenceFd = -1;
#if (ANDROID_VERSION_MAJOR >= 4) && (ANDROID_VERSION_MINOR >= 3) || (ANDROID_VERSION_MAJOR >= 5)
	// We've observed that qualcomm chipsets enters into compositionType == 6
	// (HWC_BLIT), an undocumented composition type which gives us rendering
	// glitches and warnings in logcat. By setting the planarAlpha to non-
	// opaque, we attempt to force the HWC into using HWC_FRAMEBUFFER for this
	// layer so the HWC_FRAMEBUFFER_TARGET layer actually gets used.
	bool tryToForceGLES = getenv("QPA_HWC_FORCE_GLES") != NULL;
	layer->planeAlpha = tryToForceGLES ? 1 : 255;
#endif
#ifdef HWC_DEVICE_API_VERSION_1_5
	layer->surfaceDamage.numRects = 0;
#endif

	layer = &list->hwLayers[1];
	memset(layer, 0, sizeof(hwc_layer_1_t));
	layer->compositionType = HWC_FRAMEBUFFER_TARGET;
	layer->hints = 0;
	layer->flags = 0;
	layer->handle = 0;
	layer->transform = 0;
	layer->blending = HWC_BLENDING_NONE;
#ifdef HWC_DEVICE_API_VERSION_1_3
	layer->sourceCropf.top = 0.0f;
	layer->sourceCropf.left = 0.0f;
	layer->sourceCropf.bottom = (float) attr_values[1];
	layer->sourceCropf.right = (float) attr_values[0];
#else
	layer->sourceCrop = r;
#endif
	layer->displayFrame = r;
	layer->visibleRegionScreen.numRects = 1;
	layer->visibleRegionScreen.rects = &layer->displayFrame;
	layer->acquireFenceFd = -1;
	layer->releaseFenceFd = -1;
#if (ANDROID_VERSION_MAJOR >= 4) && (ANDROID_VERSION_MINOR >= 3) || (ANDROID_VERSION_MAJOR >= 5)
	layer->planeAlpha = 0xff;
#endif
#ifdef HWC_DEVICE_API_VERSION_1_5
	layer->surfaceDamage.numRects = 0;
#endif

	list->retireFenceFd = -1;
	list->flags = HWC_GEOMETRY_CHANGED;
	list->numHwLayers = 2;


	screen_width = attr_values[0];
	screen_height = attr_values[1];

	win = new HWComposer(attr_values[0], attr_values[1], HAL_PIXEL_FORMAT_RGBA_8888, hwcDevicePtr, mList, &list->hwLayers[1]);

	display = eglGetDisplay(NULL);
	assert(eglGetError() == EGL_SUCCESS);
	assert(display != EGL_NO_DISPLAY);

	rv = eglInitialize(display, 0, 0);
	assert(eglGetError() == EGL_SUCCESS);
	assert(rv == EGL_TRUE);

	eglChooseConfig((EGLDisplay) display, attr, &ecfg, 1, &num_config);
	assert(eglGetError() == EGL_SUCCESS);
	assert(rv == EGL_TRUE);



	surface = eglCreateWindowSurface((EGLDisplay) display, ecfg, (EGLNativeWindowType) static_cast<ANativeWindow *> (win), NULL);
	assert(eglGetError() == EGL_SUCCESS);
	assert(surface != EGL_NO_SURFACE);

	context = eglCreateContext((EGLDisplay) display, ecfg, EGL_NO_CONTEXT, ctxattr);
	assert(eglGetError() == EGL_SUCCESS);
	assert(context != EGL_NO_CONTEXT);

	assert(eglMakeCurrent((EGLDisplay) display, surface, surface, context) == EGL_TRUE);

	const char *version = (const char *)glGetString(GL_VERSION);
	assert(version);
	printf("%s\n",version);
}

void video_set_filter(uint32_t filter) {
	if (filter==0) {
	    filter_min = GL_NEAREST;
	    filter_mag = GL_NEAREST;
	} else  {
	    filter_min = GL_LINEAR;
	    filter_mag = GL_LINEAR;
	}
}

void video_init(uint32_t _width, uint32_t _height, uint32_t filter)
{
	if ((_width==0)||(_height==0))
		return;

	frame_width = _width;
	frame_height = _height;

	hwcomposer_init();

	gles2_create();

	int rr=(screen_height*10/frame_height);
	int h = (frame_height*rr)/10;
	int w = (frame_width*rr)/10;
	if (w>screen_width) {
	    rr = (screen_width*10/frame_width);
	    h = (frame_height*rr)/10;
	    w = (frame_width*rr)/10;
	}
	glViewport((screen_width-w)/2, (screen_height-h)/2, w, h);
	SetOrtho(proj, -0.5f, +0.5f, +0.5f, -0.5f, -1.0f, 1.0f, 1.0f ,1.0f );
	video_set_filter(filter);
}

static void gles2_destroy()
{
	if(!shader.program)
		return;
	glDeleteBuffers(3, buffers); SHOW_ERROR
	glDeleteProgram(shader.program); SHOW_ERROR
}

static void SetOrtho(GLfloat m[4][4], GLfloat left, GLfloat right, GLfloat bottom, GLfloat top, GLfloat near, GLfloat far, GLfloat scale_x, GLfloat scale_y)
{
	memset(m, 0, 4*4*sizeof(GLfloat));
	m[0][0] = 2.0f/(right - left)*scale_x;
	m[1][1] = 2.0f/(top - bottom)*scale_y;
	m[2][2] = -2.0f/(far - near);
	m[3][0] = -(right + left)/(right - left);
	m[3][1] = -(top + bottom)/(top - bottom);
	m[3][2] = -(far + near)/(far - near);
	m[3][3] = 1;
}
#define RGB15(r, g, b)  (((r) << (5+6)) | ((g) << 6) | (b))

static void gles2_Draw( uint16_t *pixels)
{
	if(!shader.program)
		return;

	glClear(GL_COLOR_BUFFER_BIT);

	glUseProgram(shader.program);

        glBindTexture(GL_TEXTURE_2D, textures[0]);
	glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, frame_width, frame_height, GL_RGB, GL_UNSIGNED_SHORT_5_6_5, pixels);
	glActiveTexture(GL_TEXTURE0);
	glUniform1i(shader.u_texture, 0);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, filter_mag);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, filter_min);
	glGenerateMipmap(GL_TEXTURE_2D);

	glBindBuffer(GL_ARRAY_BUFFER, buffers[0]);
	glVertexAttribPointer(shader.a_position, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(GLfloat), NULL);
	glEnableVertexAttribArray(shader.a_position);

	glBindBuffer(GL_ARRAY_BUFFER, buffers[1]);
	glVertexAttribPointer(shader.a_texcoord, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(GLfloat), NULL);
	glEnableVertexAttribArray(shader.a_texcoord);

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, buffers[2]);
	glUniformMatrix4fv(shader.u_vp_matrix, 1, GL_FALSE, (const GLfloat * )&proj);
	glDrawElements(GL_TRIANGLES, kIndexCount, GL_UNSIGNED_SHORT, 0);

	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
	//glFlush();
}

void video_close()
{
	gles2_destroy();
	// Release OpenGL resources
	eglMakeCurrent( display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT );
	eglDestroySurface( display, surface );
	eglDestroyContext( display, context );
	eglTerminate( display );
}

void video_draw(uint16_t *pixels)
{
	gles2_Draw (pixels);
	eglSwapBuffers((EGLDisplay) display, surface);
}
