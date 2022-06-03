#include "GLES/gl.h"
#include "EGL/egl.h"
#include "EGL/eglext.h"
#include "GLES2/gl2.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include <unistd.h>
#include <malloc.h>

#include <SDL2/SDL.h>
#include <SDL2/SDL_syswm.h>
#include <wayland-egl.h>

#include "hwcomposer.h"

uint32_t screen_width = 0;
uint32_t screen_height = 0;

EGLDisplay display = NULL;
EGLSurface surface = NULL;
EGLContext context = NULL;

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

	SDL_Window* win = SDL_CreateWindow("GPSP", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 200, 200, SDL_WINDOW_FULLSCREEN);

	SDL_GetWindowSize(win, (int*)&screen_width, (int*)&screen_height);

	display = eglGetDisplay(EGL_DEFAULT_DISPLAY);
	assert(eglGetError() == EGL_SUCCESS);
	assert(display != EGL_NO_DISPLAY);

	EGLint majorVersion, minorVersion;
	rv = eglInitialize(display, &majorVersion, &minorVersion);
	assert(eglGetError() == EGL_SUCCESS);
	assert(rv == EGL_TRUE);

	eglChooseConfig((EGLDisplay) display, attr, &ecfg, 1, &num_config);
	assert(eglGetError() == EGL_SUCCESS);

	EGLNativeWindowType nativeWindow = (EGLNativeWindowType) win;
	struct SDL_SysWMinfo wmInfo;
	SDL_VERSION(&wmInfo.version);
	if (SDL_GetWindowWMInfo(win, &wmInfo)) {
		printf("Using Wayland window\n");
		nativeWindow = (EGLNativeWindowType)wl_egl_window_create(wmInfo.info.wl.surface, screen_width, screen_height);
	}

	static const EGLint SURFACE_ATTRIBUTES[] = { EGL_NONE };
	surface = eglCreateWindowSurface((EGLDisplay) display, ecfg, nativeWindow, SURFACE_ATTRIBUTES);
	assert(eglGetError() == EGL_SUCCESS);
	assert(surface != EGL_NO_SURFACE);

	context = eglCreateContext((EGLDisplay) display, ecfg, EGL_NO_CONTEXT, ctxattr);
	assert(eglGetError() == EGL_SUCCESS);
	assert(context != EGL_NO_CONTEXT);

	assert(eglMakeCurrent((EGLDisplay) display, surface, surface, context) == EGL_TRUE);

	// Some platforms need this called as the window would otherwise not become visible.
	eglSwapBuffers((EGLDisplay) display, surface);
	const char *version = (const char *)glGetString(GL_VERSION);
	assert(version);
	printf("%s\n",version);
}
