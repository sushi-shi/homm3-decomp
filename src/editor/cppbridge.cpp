// cppbridge.cpp - Loki h3maped object 51: the C bridge between the glade
// interface and the editor's MFC-shaped classes. libglade connects the
// signal handlers by name, so they and the helpers are extern "C" inside the
// anonymous namespace that also holds <gtk/gtk.h> ("bool
// {anonymous}::doSave(char *)"); _isspace is the one C++ function. The
// assert lines come from the retail immediates. Names of the file statics
// are not proven.
#include "editor/stdafx.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <string>

#include "editor/cppbridge.h"

// libglade's interface tree, loaded by the C startup code.
typedef struct _GladeXML GladeXML;
extern "C" GtkWidget* glade_xml_get_widget(GladeXML* self, const char* name);
extern "C" GladeXML* xml_obj;

GdkColor _m_white;
GdkColor _m_black;
GdkColor _m_red = { 0, 0xffff, 0, 0 };
GdkColor _m_yellow = { 0, 0xffff, 0xffff, 0 };

namespace {

static volatile bool msgboxDone = false;
static bool yesnoAnswer = false;
static volatile bool yesnoDone = false;
static volatile bool h3pathDialogOpen = false;
static char* homm3Path = NULL;
static char homm3ExecName[4096];

}

bool _isspace(string text)
{
    return isAllSpace(text.c_str());
}

namespace {
extern "C" {

GtkWidget* _widget(char* name)
{
    GtkWidget* retVal = glade_xml_get_widget(xml_obj, name);
#line 128
    assert(retVal != NULL);
    return retVal;
}

int getImgPitch(GdkImage* img)
{
    return img->width * 2;
}

guchar* getImgLine(GdkImage* img, int line)
{
    guchar* mem = (guchar*)img->mem;
    int pitch = getImgPitch(img);
    return mem + pitch * line;
}

guchar* advanceImgLine(guchar* p, int pitch, int lines)
{
    guchar* next = p + lines * pitch;
    return next;
}

bool isAllSpace(const char* text)
{
    while (*text) {
        if (*text != ' ')
            return false;
        text++;
    }
    return true;
}

int getCurrentSelection(GtkList* list)
{
    GList* item;
    int index = 0;
    if (!list->children || !list->selection)
        return -1;
    item = list->children;
    while (item->data != list->selection->data) {
        index++;
        item = item->next;
    }
    return index;
}

void setCurrentSelection(GtkList* l, int index)
{
#line 185
    assert(l != NULL);
    if (index < 0) {
        int current = getCurrentSelection(l);
        if (current != -1)
            gtk_list_unselect_item(l, current);
    } else
        gtk_list_select_item(l, index);
}

void emptyList(GtkList* list)
{
    if (!list || !list->children)
        return;
    int count = g_list_length(list->children);
    if (count > 0)
        gtk_list_clear_items(list, 0, count);
}

}
}

void doMessageBox(const char* message)
{
    GtkWidget* msgbox = glade_xml_get_widget(xml_obj, "ok_msgbox");
    if (!msgbox) {
        g_warning("MESSAGE BOX NOT READY. HERE'S THE MESSAGE:\n   \"%s\"\n", message);
        return;
    }
    GtkLabel* label = GTK_LABEL(_widget("ok_msgbox_label"));
    gtk_label_set_text(label, message);
    GtkWidget* button = _widget("ok_msgbox_button");
    gtk_widget_show(msgbox);
    gtk_widget_grab_focus(button);
    msgboxDone = false;
    while (!msgboxDone)
        gtk_main_iteration();
    gtk_widget_hide(msgbox);
}

namespace {
extern "C" {

bool isChecked(char* name)
{
    GtkToggleButton* button = GTK_TOGGLE_BUTTON(_widget(name));
    return gtk_toggle_button_get_active(button) == TRUE;
}

void enableWidget(char* name, gboolean bEnable)
{
    GtkWidget* widget = _widget(name);
    gtk_widget_set_sensitive(widget, bEnable);
}

void handleQueuedEvents()
{
    while (gtk_events_pending())
        gtk_main_iteration();
}

static void allocColors()
{
    GdkColormap* colormap = gdk_colormap_get_system();
    gdk_color_black(colormap, &_m_black);
    gdk_color_white(colormap, &_m_white);
    gdk_color_alloc(colormap, &_m_red);
    gdk_color_alloc(colormap, &_m_yellow);
}

bool askYesNoQuestion(char* question)
{
    GtkLabel* label = GTK_LABEL(_widget("yesno_label"));
    gtk_label_set_text(label, question);
    GtkWidget* dialog = _widget("yesno_dlg");
    gtk_widget_show(dialog);
    yesnoDone = false;
    while (!yesnoDone)
        gtk_main_iteration();
    gtk_widget_hide(dialog);
    return yesnoAnswer;
}

void on_yesno_yes_button_clicked(GtkButton* button, gpointer user_data)
{
    yesnoAnswer = true;
    yesnoDone = true;
}

void on_yesno_no_button_clicked(GtkButton* button, gpointer user_data)
{
    yesnoAnswer = false;
    yesnoDone = true;
}

bool noReadAccess(char* fileName)
{
    return access(fileName, R_OK) != 0;
}

int test_exec_file(char* path)
{
    char resolved[4096];
    if (realpath(path, resolved) && access(resolved, X_OK) == 0) {
        strcpy(path, resolved);
        return 1;
    }
    return 0;
}

char* determine_homm3_path()
{
    char path[4096];
    char* pathEnv;
    char* dir;
    char* colon;
    char* result = NULL;

    if (homm3Path)
        return homm3Path;
    pathEnv = getenv("PATH");
    if (pathEnv) {
        for (dir = pathEnv; dir; dir = strchr(dir, ':')) {
            if (dir != pathEnv)
                dir++;
            strncpy(path, dir, sizeof(path) - 1);
            colon = strchr(path, ':');
            if (colon)
                *colon = '\0';
            if (path[strlen(path) - 1] != '/')
                strcat(path, "/heroes3");
            else
                strcat(path, "heroes3");
            if (test_exec_file(path)) {
                result = (char*)malloc(strlen(path) + 1);
                strcpy(result, path);
            }
        }
    }
    if (result) {
        dir = strrchr(result, '/');
        if (!dir) {
            strcpy(homm3ExecName, result);
            strcpy(result, "./");
        } else {
            strcpy(homm3ExecName, dir + 1);
            dir[1] = '\0';
        }
    }
    homm3Path = result;
    return result;
}

void on_h3path_ok_clicked(GtkButton* button, gpointer user_data)
{
    GtkEditable* entry = GTK_EDITABLE(_widget("h3path_entry"));
    const gchar* text = gtk_editable_get_chars(entry, 0, -1);
    int length = strlen(text);
    if (text[length - 1] != '/') {
        homm3Path = (char*)g_malloc(length + 2);
        strcpy(homm3Path, text);
        strcat(homm3Path, "/");
    } else {
        homm3Path = (char*)g_malloc(length + 1);
        strcpy(homm3Path, text);
    }
    g_free((gpointer)text);
    h3pathDialogOpen = false;
}

void on_h3path_cancel_clicked(GtkButton* button, gpointer user_data)
{
    _exit(0);
}

bool accessible_data_files()
{
    if (!homm3Path && !determine_homm3_path())
        return false;
    char fileName[strlen(homm3Path) + 50];
    sprintf(fileName, "%sdata/%s", homm3Path, "h3bitmap.lod");
    if (noReadAccess(fileName))
        return false;
    sprintf(fileName, "%sdata/%s", homm3Path, "h3sprite.lod");
    if (noReadAccess(fileName))
        return false;
    return true;
}

}
}
