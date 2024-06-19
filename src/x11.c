#include "windowsystem.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xatom.h>

#include <string.h>

Display *Con;
Atom NetActiveWindow;

struct window {
    Window window;
    char *title;
    char *class;
    char *instance;
} ActiveWindow;

int init_window_system(void)
{
    Con = XOpenDisplay(NULL);
    if (Con == NULL) {
        fprintf(stderr, "Failed opening X display\n");
        return -1;
    }
    NetActiveWindow = XInternAtom(Con, "_NET_ACTIVE_WINDOW", False);
    if (NetActiveWindow == None) {
        return -1;
    }
    return 0;
}

void *get_active_window(void)
{
    XFree(ActiveWindow.title);
    XFree(ActiveWindow.class);
    XFree(ActiveWindow.instance);

    unsigned long nitems;
    Atom type;
    int format;
    unsigned long bytesAfter;
    unsigned char *prop = NULL;
    int result = XGetWindowProperty(Con, XDefaultRootWindow(Con),
                                    NetActiveWindow, 0, sizeof(Window), False,
                                    XA_WINDOW, &type, &format, &nitems, &bytesAfter, &prop);
    if (result == Success && prop != NULL) {
        ActiveWindow.window = *(Window*) prop;
        XFree(prop);
    } else {
        return NULL;
    }

    XTextProperty text;
    if (XGetWMName(Con, ActiveWindow.window, &text)) {
        char **list = NULL;
        int count;
        if (Xutf8TextPropertyToTextList(Con, &text, &list, &count) >=
                Success && count > 0 && list != NULL) {
            ActiveWindow.title = strdup(list[0]);
            XFreeStringList(list);
        }
        XFree(text.value);
    }

    XClassHint hint;
    if (XGetClassHint(Con, ActiveWindow.window, &hint)) {
        ActiveWindow.class = hint.res_class;
        ActiveWindow.instance = hint.res_name;
    }
    return ActiveWindow.title;
}
