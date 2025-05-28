#include <exec/types.h>
#include <cstdint>
#include <WebKitLegacy/morphos/WebView.h>
#include <WebKitLegacy/morphos/WebFrame.h>
#include <wtf/RunLoop.h>

#include <proto/exec.h>
#include <proto/dos.h>
#include <cairo.h>
#include <stdio.h>

unsigned long __stack = 2 * 1024 * 1024;

extern "C" {
	void dprintf(const char *fmt, ...);
};

struct Library *FreetypeBase;

int main(void)
{
	printf("Hello\n");
	FreetypeBase = OpenLibrary("freetype.library", 0);
	if (FreetypeBase)
	{
		// Hack to make sure cairo mutex are initialized
		// TODO: fix this in the fucking cairo
		cairo_surface_t *dummysurface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, 4, 4);
		if (dummysurface)
			cairo_surface_destroy(dummysurface);

        dprintf("Creating webview...\n");
		WebView *view = new WebView();
		dprintf("page %p\n", view->page());
		//view->go("http://www.google.com/");
		view->go("file:///System:test.html");
		dprintf("issued...\n");
		
		for (int i = 0; i < 1000; i++)
		{
			WTF::RunLoop::iterate();
			dprintf("iteration %d\n", i);
			Delay(1);
		}

		printf("deleting\n");
		delete view;
		printf("deleted...\n");
		CloseLibrary(FreetypeBase);
	}
	return 0;
}

