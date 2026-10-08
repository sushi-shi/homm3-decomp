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
#include <sys/time.h>
#include <unistd.h>
#include <string>

#include "editor/cppbridge.h"
#include "editor/Hero.h"
#include "editor/MapDoc.h"
#include "editor/MapEditWnd.h"
#include "editor/MapFrameWnd.h"
#include "editor/MapView.h"
#include "editor/MiniMapWnd.h"
#include "editor/ObjectPaletteWnd.h"
#include "editor/TileHRuler.h"
#include "editor/TileVRuler.h"
#include "editor/Colors.h"
#include "editor/GameResource.h"
#include "editor/MapEditorText.h"
#include "editor/ObjectSprites.h"
#include "editor/Player.h"
#include "editor/ObjectSpecializations.h"
#include "editor/T16bppBitmap.h"
#include "editor/Town.h"
#include "editor/Tile.h"
#include "artifact.h"
#include "creaturetype.h"
#include "herodefs.h"
#include "objecttype.h"
#include "objnames.h"
#include "resourcemanager.h"
#include "spelldefs.h"

namespace {
#include <gdk/gdkkeysyms.h>
#include <glade/glade.h>
}

// main.c: the loaded glade tree.
extern GladeXML* xml_obj;

GdkColor _m_white;
GdkColor _m_black;

namespace {

static volatile bool saveDialogDone = false;
static volatile bool saveDialogResult = false;
static volatile bool msgboxDone = false;
static bool yesnoAnswer = false;
static volatile bool yesnoDone = false;
static volatile bool h3pathDialogOpen = false;
static char* currentFileName = NULL;
static TMapSpecsSheet* mapSpecsSheet = NULL;
static TMapView* mapView = NULL;
static TMapFrameWnd* mapFrameWnd = NULL;
static TMapEditWnd* mapEditWnd = NULL;
static TMiniMapWnd* miniMapWnd = NULL;
static TTileHRuler* hRuler = NULL;
static TTileVRuler* vRuler = NULL;
static TObjectPaletteWnd* objPaletteWnd = NULL;
static GtkNotebook* toolNotebook = NULL;
static GtkToggleButton* activeBrushToggle = NULL;
static GtkToggleButton* activeToolToggle = NULL;
static unsigned long lastAnimationTime = 0;
static gint animationTimer = -1;
static bool bObjectAnimation = false;
static bool bTerrainAnimation = false;
static GtkAdjustment* mapVAdjustment = NULL;
static GtkAdjustment* mapHAdjustment = NULL;

}

GdkColor _m_red = { 0, 0xffff, 0, 0 };
GdkColor _m_yellow = { 0, 0xffff, 0xffff, 0 };

namespace {

static char* homm3Path = NULL;
static bool bShowToolbar = true;
static bool bShowModebar = true;
static bool bShowStatusbar = true;
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

bool doSaveConfirm()
{
    bool bSave = false;
    if (mapView->getPDocument()->IsModified())
        bSave = askYesNoQuestion("Save current map?");
    return bSave;
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

void on_objpalvscroll_value_changed(GtkAdjustment* adjustment, gpointer user_data)
{
    objPaletteWnd->OnVScroll(42);
}

void on_mapvscroll_value_changed(GtkAdjustment* adjustment, gpointer user_data)
{
    mapEditWnd->OnVScroll(42);
}

void on_maphscroll_value_changed(GtkAdjustment* adjustment, gpointer user_data)
{
    mapEditWnd->OnHScroll(42);
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


void init_objects(int argc, char** argv)
{
    allocColors();
    GtkWidget* h3pathDialog = _widget("h3path_dlg");
    while (!accessible_data_files()) {
        if (homm3Path) {
            g_free(homm3Path);
            homm3Path = NULL;
        }
        h3pathDialogOpen = true;
        gtk_widget_show(h3pathDialog);
        while (h3pathDialogOpen)
            gtk_main_iteration();
        gtk_widget_hide(h3pathDialog);
    }

    toolNotebook = GTK_NOTEBOOK(_widget("toolnotebook"));
    GtkWidget* scrollbar = _widget("mapvscroll");
    mapVAdjustment = gtk_range_get_adjustment(GTK_RANGE(scrollbar));
    gtk_signal_connect(GTK_OBJECT(mapVAdjustment), "value-changed",
                       GTK_SIGNAL_FUNC(on_mapvscroll_value_changed), NULL);
    scrollbar = _widget("maphscroll");
    mapHAdjustment = gtk_range_get_adjustment(GTK_RANGE(scrollbar));
    gtk_signal_connect(GTK_OBJECT(mapHAdjustment), "value-changed",
                       GTK_SIGNAL_FUNC(on_maphscroll_value_changed), NULL);
    scrollbar = _widget("objpalettevscroll");
    GtkAdjustment* objPaletteAdjustment = gtk_range_get_adjustment(GTK_RANGE(scrollbar));
    gtk_signal_connect(GTK_OBJECT(objPaletteAdjustment), "value-changed",
                       GTK_SIGNAL_FUNC(on_objpalvscroll_value_changed), NULL);
    handleQueuedEvents();

    char dataPath[strlen(homm3Path) + 15];
    strcpy(dataPath, homm3Path);
    strcat(dataPath, "data/");
    ResourceManager::SetPath(dataPath);
    ResourceManager::Open(true, true);
    ResourceManager::SetPixelFormat(T16bppBitmapBase<ubyte>::redMask(), T16bppBitmapBase<ubyte>::greenMask(),
                                    T16bppBitmapBase<ubyte>::blueMask());
    handleQueuedEvents();
    SText::initialize();
    handleQueuedEvents();
    loadTilesets();
    handleQueuedEvents();
    initColors();
    handleQueuedEvents();
    initializeObjectSprites();
    handleQueuedEvents();
    InitializeAdvObjectTypeTraitsTable();
    handleQueuedEvents();
    InitializeGameResourceTypeTraitsTable();
    handleQueuedEvents();
    InitializePlayerTraitsTable();
    handleQueuedEvents();
    InitializeCreatureTypeTraitsTable();
    handleQueuedEvents();
    InitializeArtifactTraitsTable();
    handleQueuedEvents();
    InitializeSpellTraitsTable();
    handleQueuedEvents();
    InitializeCreatureBankTypeTraitsTable();
    handleQueuedEvents();
    InitializeMonolithTypeTraitsTables();
    handleQueuedEvents();
    TMine::initializeTypeTraitsTable();
    handleQueuedEvents();
    TGenerator::initializeTypeTraitsTables();
    handleQueuedEvents();
    TGarrison::initializeTypeTraitsTable();
    handleQueuedEvents();
    InitializeSSkillTraitsTable();
    handleQueuedEvents();
    InitializeHeroClassTraitsTable();
    handleQueuedEvents();
    InitializeHeroTraitsTable();
    handleQueuedEvents();
    loadObjectTypeTable();
    handleQueuedEvents();
    THero::initialize();
    handleQueuedEvents();
    TTown::initialize();
    handleQueuedEvents();

    mapView = new TMapView(mapHAdjustment, mapVAdjustment);
    miniMapWnd = mapView->getMiniMapWnd();
    mapFrameWnd = mapView->getMapFrameWnd();
    mapEditWnd = mapFrameWnd->getMapEditWnd();
    hRuler = mapFrameWnd->getHRuler();
    vRuler = mapFrameWnd->getVRuler();
    objPaletteWnd = mapView->getObjPaletteWnd();
    activeBrushToggle = GTK_TOGGLE_BUTTON(_widget("2x2brushtoggle"));
    gtk_toggle_button_set_active(activeBrushToggle, TRUE);
    activeToolToggle = GTK_TOGGLE_BUTTON(_widget("dirttoggle"));
    gtk_toggle_button_set_active(activeToolToggle, TRUE);
    handleQueuedEvents();

    for (int i = 1; i < argc; i++) {
        if (argv[i]) {
            doOpen(argv[i]);
            handleQueuedEvents();
            return;
        }
    }
}

void shutdown_objects()
{
}

static unsigned long getMilliseconds()
{
    struct timeval now;
    gettimeofday(&now, NULL);
    return now.tv_sec * 1000 + now.tv_usec / 1000;
}

static gint animationTimerFunc(gpointer data)
{
    unsigned long now = getMilliseconds();
    unsigned long elapsed = now - lastAnimationTime;
    if (elapsed > 179) {
        unsigned int frames = elapsed / 180;
        lastAnimationTime += frames * 180;
        animateTilesets(frames);
        TMapView::animate(frames, true);
    }
}

bool doSave(char* filename)
{
    bool bResult = true;
#line 670
    assert(filename != NULL);
    FILE* file = fopen(filename, "wb");
    if (file) {
        fclose(file);
        mapView->getPDocument()->OnSaveDocument(filename);
    } else {
        int length = strlen(filename) + 35;
        char message[length];
        snprintf(message, length, "Couldn't open \"%s\"!", filename);
        doMessageBox(message);
        bResult = false;
    }
    return bResult;
}

bool save_request(char* filename)
{
    bool bResult = false;
    if (filename) {
        bResult = doSave(filename);
    } else {
        GtkFileSelection* fileSelection = GTK_FILE_SELECTION(_widget("save_map_dlg"));
        gtk_file_selection_set_filename(fileSelection, "");
        saveDialogDone = false;
        gtk_widget_show(GTK_WIDGET(fileSelection));
        while (!saveDialogDone)
            gtk_main_iteration();
        gtk_widget_hide(GTK_WIDGET(fileSelection));
        bResult = saveDialogResult;
    }
    return bResult;
}

void terminate_mapeditor()
{
    if (doSaveConfirm() == true && !save_request(currentFileName))
        return;
    shutdown_objects();
    _exit(0);
}

void doOpen(char* filename)
{
#line 733
    assert(filename != NULL);
    int length = strlen(filename);
    char pathName[length + 5];
    strcpy(pathName, filename);
    FILE* file = fopen(pathName, "rb");
    if (!file) {
        strcat(pathName, ".h3m");
        file = fopen(pathName, "rb");
        if (!file) {
            char message[length + length + 150];
            snprintf(message, length + length + 150, "Couldn't open \"%s\" or \"%s\"!", filename, pathName);
            doMessageBox(message);
            return;
        }
    }
    fclose(file);

    GtkToggleButton* undergroundButton = GTK_TOGGLE_BUTTON(_widget("underground_button"));
    if (gtk_toggle_button_get_active(undergroundButton))
        gtk_toggle_button_set_active(undergroundButton, FALSE);
    TMapDoc* pDoc = mapView->getPDocument();
    pDoc->OnOpenDocument(pathName);
    bool bTwoLayer = pDoc->getPMap()->isTwoLayer();
    gtk_widget_set_sensitive(GTK_WIDGET(undergroundButton), bTwoLayer ? TRUE : FALSE);
    currentFileName = (char*)realloc(currentFileName, strlen(pathName) + 1);
    strcpy(currentFileName, pathName);
    mapView->OnInitialUpdate(NULL, NULL);
    mapEditWnd->OnSize(0, mapEditWnd->_m_hWnd->allocation.width, mapEditWnd->_m_hWnd->allocation.width);
}

void on_open_map_ok_clicked(GtkButton* button, gpointer user_data)
{
    GtkFileSelection* fileSelection = GTK_FILE_SELECTION(_widget("open_map_dlg"));
    char* filename = gtk_file_selection_get_filename(fileSelection);
    gtk_widget_hide(GTK_WIDGET(fileSelection));
    doOpen(filename);
}

void on_save_map_ok_clicked(GtkButton* button, gpointer user_data)
{
    GtkFileSelection* fileSelection = GTK_FILE_SELECTION(_widget("save_map_dlg"));
    char* filename = gtk_file_selection_get_filename(fileSelection);
#line 790
    assert(filename != NULL);
    int length = strlen(filename);
    char pathName[length + 5];
    strcpy(pathName, filename);
    char* extension = strchr(pathName, '.');
    if (!extension || strcasecmp(extension, ".h3m") != 0)
        strcat(pathName, ".h3m");
    FILE* file = fopen(pathName, "rb");
    if (file) {
        fclose(file);
        if (!askYesNoQuestion("File Exists! Overwrite?"))
            return;
    }
    currentFileName = (char*)realloc(currentFileName, strlen(pathName) + 1);
    strcpy(currentFileName, pathName);
    saveDialogDone = true;
    saveDialogResult = doSave(currentFileName);
}

void on_open_map_cancel_clicked(GtkButton* button, gpointer user_data)
{
    GtkFileSelection* fileSelection = GTK_FILE_SELECTION(_widget("open_map_dlg"));
    gtk_widget_hide(GTK_WIDGET(fileSelection));
}

void on_save_map_cancel_clicked(GtkButton* button, gpointer user_data)
{
    saveDialogResult = false;
    saveDialogDone = true;
}

gboolean on_MainWindow_delete_event(GtkWidget* widget, GdkEvent* event, gpointer user_data)
{
    terminate_mapeditor();
    return TRUE;
}

void new_request()
{
    if (doSaveConfirm() == true && !save_request(currentFileName))
        return;
    GtkWidget* newMapDialog = _widget("new_map_dlg");
    GtkToggleButton* button = GTK_TOGGLE_BUTTON(_widget("new_map_72x72"));
    gtk_toggle_button_set_active(button, TRUE);
    button = GTK_TOGGLE_BUTTON(_widget("new_map_two_level"));
    gtk_toggle_button_set_active(button, TRUE);
    gtk_widget_show(newMapDialog);
}

void open_request()
{
    if (doSaveConfirm() == true && !save_request(currentFileName))
        return;
    GtkFileSelection* fileSelection = GTK_FILE_SELECTION(_widget("open_map_dlg"));
    gtk_file_selection_set_filename(fileSelection, "");
    gtk_widget_show(GTK_WIDGET(fileSelection));
}

void on_menuitem_file_new_activate(GtkMenuItem* menuitem, gpointer user_data)
{
    new_request();
}

void on_menuitem_file_open_activate(GtkMenuItem* menuitem, gpointer user_data)
{
    open_request();
}

void on_menuitem_file_save_activate(GtkMenuItem* menuitem, gpointer user_data)
{
    save_request(currentFileName);
}

void on_menuitem_file_saveas_activate(GtkMenuItem* menuitem, gpointer user_data)
{
    save_request(NULL);
}

void on_menuitem_file_importtext_activate(GtkMenuItem* menuitem, gpointer user_data)
{
    GtkFileSelection* fileSelection = GTK_FILE_SELECTION(_widget("import_text_dlg"));
    gtk_file_selection_set_filename(fileSelection, "");
    gtk_widget_show(GTK_WIDGET(fileSelection));
}

void on_menuitem_file_exporttext_activate(GtkMenuItem* menuitem, gpointer user_data)
{
    GtkFileSelection* fileSelection = GTK_FILE_SELECTION(_widget("export_text_dlg"));
    gtk_file_selection_set_filename(fileSelection, "");
    gtk_widget_show(GTK_WIDGET(fileSelection));
}

void on_menuitem_file_exit_activate(GtkMenuItem* menuitem, gpointer user_data)
{
    terminate_mapeditor();
}

void on_menuitem_edit_undo_activate(GtkMenuItem* menuitem, gpointer user_data)
{
    mapView->OnEditUndo();
}

void on_menuitem_edit_redo_activate(GtkMenuItem* menuitem, gpointer user_data)
{
    mapView->OnEditRedo();
}

void on_menuitem_edit_cut_activate(GtkMenuItem* menuitem, gpointer user_data)
{
    mapEditWnd->OnEditCut();
}

void on_menuitem_edit_copy_activate(GtkMenuItem* menuitem, gpointer user_data)
{
    mapEditWnd->OnEditCopy();
}

void on_menuitem_edit_paste_activate(GtkMenuItem* menuitem, gpointer user_data)
{
    mapEditWnd->OnEditPaste();
}

void on_menuitem_edit_delete_activate(GtkMenuItem* menuitem, gpointer user_data)
{
    mapEditWnd->OnEditDelete();
}

void on_menuitem_edit_find_activate(GtkMenuItem* menuitem, gpointer user_data)
{
    mapView->OnEditFind();
}

void on_menuitem_edit_findagain_activate(GtkMenuItem* menuitem, gpointer user_data)
{
    mapView->OnEditFindNext();
}

void on_menuitem_edit_properties_activate(GtkMenuItem* menuitem, gpointer user_data)
{
    mapEditWnd->OnEditProperties();
}

void on_menuitem_view_zoomin_activate(GtkMenuItem* menuitem, gpointer user_data)
{
    mapEditWnd->incZoom();
}

void on_menuitem_view_zoomout_activate(GtkMenuItem* menuitem, gpointer user_data)
{
    mapEditWnd->decZoom();
}

void on_menuitem_view_underground_activate(GtkMenuItem* menuitem, gpointer user_data)
{
    GtkToggleButton* button = GTK_TOGGLE_BUTTON(_widget("underground_button"));
    gtk_toggle_button_toggled(button);
}

void on_menuitem_view_grid_activate(GtkMenuItem* menuitem, gpointer user_data)
{
    GtkToggleButton* button = GTK_TOGGLE_BUTTON(_widget("grid_button"));
    gtk_toggle_button_toggled(button);
}

void on_menuitem_view_passability_activate(GtkMenuItem* menuitem, gpointer user_data)
{
    GtkToggleButton* button = GTK_TOGGLE_BUTTON(_widget("passability_button"));
    gtk_toggle_button_toggled(button);
}

void flipTimer()
{
    if (animationTimer == -1) {
        animationTimer = gtk_timeout_add(180, animationTimerFunc, NULL);
    } else if (!bObjectAnimation && !bTerrainAnimation) {
        gtk_timeout_remove(animationTimer);
        animationTimer = -1;
        TMapView::resetAnimation();
    }
}

void on_menuitem_view_objectanimation_activate(GtkMenuItem* menuitem, gpointer user_data)
{
    bObjectAnimation = bObjectAnimation ? false : true;
    flipTimer();
}

void on_menuitem_view_terrainanimation_activate(GtkMenuItem* menuitem, gpointer user_data)
{
    bTerrainAnimation = bTerrainAnimation ? false : true;
    flipTimer();
}

void on_menuitem_view_toolbar_activate(GtkMenuItem* menuitem, gpointer user_data)
{
    GtkWidget* bar = _widget("toolbar");
    if (bShowToolbar) {
        gtk_item_deselect(GTK_ITEM(menuitem));
        gtk_widget_hide(bar);
    } else {
        gtk_item_select(GTK_ITEM(menuitem));
        gtk_widget_show(bar);
    }
    bShowToolbar = !bShowToolbar;
}

void on_menuitem_view_modebar_activate(GtkMenuItem* menuitem, gpointer user_data)
{
    GtkWidget* bar = _widget("modebar");
    if (bShowModebar) {
        gtk_item_deselect(GTK_ITEM(menuitem));
        gtk_widget_hide(bar);
    } else {
        gtk_item_select(GTK_ITEM(menuitem));
        gtk_widget_show(bar);
    }
    bShowModebar = !bShowModebar;
}

void on_menuitem_view_statusbar_activate(GtkMenuItem* menuitem, gpointer user_data)
{
    GtkWidget* bar = _widget("statusbar");
    if (bShowStatusbar) {
        gtk_item_deselect(GTK_ITEM(menuitem));
        gtk_widget_hide(bar);
    } else {
        gtk_item_select(GTK_ITEM(menuitem));
        gtk_widget_show(bar);
    }
    bShowStatusbar = !bShowStatusbar;
}

void on_menuitem_tools_terrain_1x1_activate(GtkMenuItem* menuitem, gpointer user_data)
{
    GtkToggleButton* button = GTK_TOGGLE_BUTTON(_widget("1x1brushtoggle"));
    gtk_toggle_button_set_active(button, TRUE);
}

void on_menuitem_tools_terrain_2x2_activate(GtkMenuItem* menuitem, gpointer user_data)
{
    GtkToggleButton* button = GTK_TOGGLE_BUTTON(_widget("2x2brushtoggle"));
    gtk_toggle_button_set_active(button, TRUE);
}

void on_menuitem_tools_terrain_4x4_activate(GtkMenuItem* menuitem, gpointer user_data)
{
    GtkToggleButton* button = GTK_TOGGLE_BUTTON(_widget("4x4brushtoggle"));
    gtk_toggle_button_set_active(button, TRUE);
}

void on_menuitem_tools_terrain_fill_activate(GtkMenuItem* menuitem, gpointer user_data)
{
    GtkToggleButton* button = GTK_TOGGLE_BUTTON(_widget("fillbrushtoggle"));
    gtk_toggle_button_set_active(button, TRUE);
}

void on_menuitem_tools_terrain_dirt_acivate(GtkMenuItem* menuitem, gpointer user_data)
{
    GtkToggleButton* button = GTK_TOGGLE_BUTTON(_widget("dirttoggle"));
    gtk_toggle_button_set_active(button, TRUE);
}

void on_menuitem_tools_terrain_sand_activate(GtkMenuItem* menuitem, gpointer user_data)
{
    GtkToggleButton* button = GTK_TOGGLE_BUTTON(_widget("sandtoggle"));
    gtk_toggle_button_set_active(button, TRUE);
}

void on_menuitem_tools_terrain_grass_activate(GtkMenuItem* menuitem, gpointer user_data)
{
    GtkToggleButton* button = GTK_TOGGLE_BUTTON(_widget("grasstoggle"));
    gtk_toggle_button_set_active(button, TRUE);
}

void on_menuitem_tools_terrain_snow_activate(GtkMenuItem* menuitem, gpointer user_data)
{
    GtkToggleButton* button = GTK_TOGGLE_BUTTON(_widget("snowtoggle"));
    gtk_toggle_button_set_active(button, TRUE);
}

void on_menuitem_tools_terrain_swamp_activate(GtkMenuItem* menuitem, gpointer user_data)
{
    GtkToggleButton* button = GTK_TOGGLE_BUTTON(_widget("swamptoggle"));
    gtk_toggle_button_set_active(button, TRUE);
}

void on_menuitem_tools_terrain_rough_activate(GtkMenuItem* menuitem, gpointer user_data)
{
    GtkToggleButton* button = GTK_TOGGLE_BUTTON(_widget("roughtoggle"));
    gtk_toggle_button_set_active(button, TRUE);
}

void on_menuitem_tools_terrain_subterranean_activate(GtkMenuItem* menuitem, gpointer user_data)
{
    GtkToggleButton* button = GTK_TOGGLE_BUTTON(_widget("subterraneantoggle"));
    gtk_toggle_button_set_active(button, TRUE);
}

void on_menuitem_tools_terrain_lava_activate(GtkMenuItem* menuitem, gpointer user_data)
{
    GtkToggleButton* button = GTK_TOGGLE_BUTTON(_widget("lavatoggle"));
    gtk_toggle_button_set_active(button, TRUE);
}

void on_menuitem_tools_terrain_water_activate(GtkMenuItem* menuitem, gpointer user_data)
{
    GtkToggleButton* button = GTK_TOGGLE_BUTTON(_widget("watertoggle"));
    gtk_toggle_button_set_active(button, TRUE);
}

void on_menuitem_tools_terrain_rock_activate(GtkMenuItem* menuitem, gpointer user_data)
{
    GtkToggleButton* button = GTK_TOGGLE_BUTTON(_widget("rocktoggle"));
    gtk_toggle_button_set_active(button, TRUE);
}

void on_menuitem_tools_rivers_clear_activate(GtkMenuItem* menuitem, gpointer user_data)
{
    GtkToggleButton* button = GTK_TOGGLE_BUTTON(_widget("clearrivtoggle"));
    gtk_toggle_button_set_active(button, TRUE);
}

void on_menuitem_tools_rivers_icy_activate(GtkMenuItem* menuitem, gpointer user_data)
{
    GtkToggleButton* button = GTK_TOGGLE_BUTTON(_widget("icyrivtoggle"));
    gtk_toggle_button_set_active(button, TRUE);
}

void on_menuitem_tools_rivers_muddy_activate(GtkMenuItem* menuitem, gpointer user_data)
{
    GtkToggleButton* button = GTK_TOGGLE_BUTTON(_widget("muddyrivtoggle"));
    gtk_toggle_button_set_active(button, TRUE);
}

void on_menuitem_tools_rivers_lava_activate(GtkMenuItem* menuitem, gpointer user_data)
{
    GtkToggleButton* button = GTK_TOGGLE_BUTTON(_widget("lavarivtoggle"));
    gtk_toggle_button_set_active(button, TRUE);
}

void on_menuitem_tools_roads_dirt_activate(GtkMenuItem* menuitem, gpointer user_data)
{
    GtkToggleButton* button = GTK_TOGGLE_BUTTON(_widget("dirtroadtoggle"));
    gtk_toggle_button_set_active(button, TRUE);
}

void on_menuitem_tools_roads_gravel_activate(GtkMenuItem* menuitem, gpointer user_data)
{
    GtkToggleButton* button = GTK_TOGGLE_BUTTON(_widget("gravelroadtoggle"));
    gtk_toggle_button_set_active(button, TRUE);
}

void on_menuitem_tools_roads_cobblestone_activate(GtkMenuItem* menuitem, gpointer user_data)
{
    GtkToggleButton* button = GTK_TOGGLE_BUTTON(_widget("cobblestoneroadtoggle"));
    gtk_toggle_button_set_active(button, TRUE);
}

void on_menuitem_tools_erase_1x1_activate(GtkMenuItem* menuitem, gpointer user_data)
{
    GtkToggleButton* button = GTK_TOGGLE_BUTTON(_widget("1x1erasertoggle"));
    gtk_toggle_button_set_active(button, TRUE);
}

void on_menuitem_tools_erase_2x2_activate(GtkMenuItem* menuitem, gpointer user_data)
{
    GtkToggleButton* button = GTK_TOGGLE_BUTTON(_widget("2x2erasertoggle"));
    gtk_toggle_button_set_active(button, TRUE);
}

void on_menuitem_tools_erase_4x4_activate(GtkMenuItem* menuitem, gpointer user_data)
{
    GtkToggleButton* button = GTK_TOGGLE_BUTTON(_widget("4x4erasertoggle"));
    gtk_toggle_button_set_active(button, TRUE);
}

void on_menuitem_tools_erase_fill_activate(GtkMenuItem* menuitem, gpointer user_data)
{
    GtkToggleButton* button = GTK_TOGGLE_BUTTON(_widget("fillerasertoggle"));
    gtk_toggle_button_set_active(button, TRUE);
}

void on_menuitem_tools_objects_dirt_activate(GtkMenuItem* menuitem, gpointer user_data)
{
    gtk_notebook_set_page(toolNotebook, 4);
    mapView->OnToolsObjectsDirt();
}

void on_menuitem_tools_objects_sand_activate(GtkMenuItem* menuitem, gpointer user_data)
{
    gtk_notebook_set_page(toolNotebook, 4);
    mapView->OnToolsObjectsSand();
}

void on_menuitem_tools_objects_grass_activate(GtkMenuItem* menuitem, gpointer user_data)
{
    gtk_notebook_set_page(toolNotebook, 4);
    mapView->OnToolsObjectsGrass();
}

void on_menuitem_tools_objects_snow_activate(GtkMenuItem* menuitem, gpointer user_data)
{
    gtk_notebook_set_page(toolNotebook, 4);
    mapView->OnToolsObjectsSnow();
}

void on_menuitem_tools_objects_swamp_activate(GtkMenuItem* menuitem, gpointer user_data)
{
    gtk_notebook_set_page(toolNotebook, 4);
    mapView->OnToolsObjectsSwamp();
}

void on_menuitem_tools_objects_rough_activate(GtkMenuItem* menuitem, gpointer user_data)
{
    gtk_notebook_set_page(toolNotebook, 4);
    mapView->OnToolsObjectsRough();
}

void on_menuitem_tools_objects_subterranean_activate(GtkMenuItem* menuitem, gpointer user_data)
{
    gtk_notebook_set_page(toolNotebook, 4);
    mapView->OnToolsObjectsSubterranean();
}

void on_menuitem_tools_objects_lava_activate(GtkMenuItem* menuitem, gpointer user_data)
{
    gtk_notebook_set_page(toolNotebook, 4);
    mapView->OnToolsObjectsLava();
}

void on_menuitem_tools_objects_water_activate(GtkMenuItem* menuitem, gpointer user_data)
{
    gtk_notebook_set_page(toolNotebook, 4);
    mapView->OnToolsObjectsWater();
}

void on_menuitem_tools_objects_allterain_activate(GtkMenuItem* menuitem, gpointer user_data)
{
    gtk_notebook_set_page(toolNotebook, 4);
    mapView->OnToolsObjectsAllTerrain();
}

void on_menuitem_tools_objects_towns_activate(GtkMenuItem* menuitem, gpointer user_data)
{
    gtk_notebook_set_page(toolNotebook, 4);
    mapView->OnToolsObjectsTowns();
}

void on_menuitem_tools_objects_monsters_activate(GtkMenuItem* menuitem, gpointer user_data)
{
    gtk_notebook_set_page(toolNotebook, 4);
    mapView->OnToolsObjectsMonsters();
}

void on_menuitem_tools_objects_heroes_activate(GtkMenuItem* menuitem, gpointer user_data)
{
    gtk_notebook_set_page(toolNotebook, 4);
    mapView->OnToolsObjectsHeroes();
}

void on_menuitem_tools_objects_artifacts_activate(GtkMenuItem* menuitem, gpointer user_data)
{
    gtk_notebook_set_page(toolNotebook, 4);
    mapView->OnToolsObjectsArtifacts();
}

void on_menuitem_tools_objects_treasures_activate(GtkMenuItem* menuitem, gpointer user_data)
{
    gtk_notebook_set_page(toolNotebook, 4);
    mapView->OnToolsObjectsTreasures();
}


void on_menuitem_tools_validatemap_activate(GtkMenuItem* menuitem, gpointer user_data)
{
    mapView->OnToolsValidateMap();
}

void on_menuitem_tools_options_activate(GtkMenuItem* menuitem, gpointer user_data)
{
    unsigned int frequency = TMapDoc::getSpecialTileFrequency();
    GtkToggleButton* repaintButton = GTK_TOGGLE_BUTTON(_widget("repaint_map"));
    gtk_toggle_button_set_active(repaintButton, FALSE);
    GtkRange* scale = GTK_RANGE(_widget("tile_freq_scale"));
    GtkAdjustment* adjustment = gtk_range_get_adjustment(scale);
    adjustment->value = frequency;
    adjustment->lower = 0;
    adjustment->upper = 8.0f;
    gtk_adjustment_changed(adjustment);
    gtk_adjustment_value_changed(adjustment);
    gtk_widget_show(_widget("options_dlg"));
}

void on_menuitem_player_none_activate(GtkMenuItem* menuitem, gpointer user_data)
{
    mapView->OnPlayersNone();
}

void on_menuitem_player_1_activate(GtkMenuItem* menuitem, gpointer user_data)
{
    mapView->OnPlayersPlayer1();
}

void on_menuitem_player_2_activate(GtkMenuItem* menuitem, gpointer user_data)
{
    mapView->OnPlayersPlayer2();
}

void on_menuitem_player_3_activate(GtkMenuItem* menuitem, gpointer user_data)
{
    mapView->OnPlayersPlayer3();
}

void on_menuitem_player_4_activate(GtkMenuItem* menuitem, gpointer user_data)
{
    mapView->OnPlayersPlayer4();
}

void on_menuitem_player_5_activate(GtkMenuItem* menuitem, gpointer user_data)
{
    mapView->OnPlayersPlayer5();
}

void on_menuitem_player_6_activate(GtkMenuItem* menuitem, gpointer user_data)
{
    mapView->OnPlayersPlayer6();
}

void on_menuitem_player_7_activate(GtkMenuItem* menuitem, gpointer user_data)
{
    mapView->OnPlayersPlayer7();
}

void on_menuitem_player_8_activate(GtkMenuItem* menuitem, gpointer user_data)
{
    mapView->OnPlayersPlayer8();
}

void on_menuitem_help_about_activate(GtkMenuItem* menuitem, gpointer user_data)
{
    gtk_widget_show(_widget("about_dlg"));
}

void on_new_button_clicked(GtkButton* button, gpointer user_data)
{
    new_request();
}

void on_open_button_clicked(GtkButton* button, gpointer user_data)
{
    open_request();
}

void on_save_button_clicked(GtkButton* button, gpointer user_data)
{
    save_request(currentFileName);
}

void on_cut_button_clicked(GtkButton* button, gpointer user_data)
{
}

void on_copy_button_clicked(GtkButton* button, gpointer user_data)
{
}

void on_paste_button_clicked(GtkButton* button, gpointer user_data)
{
}

void on_undo_button_clicked(GtkButton* button, gpointer user_data)
{
    mapView->OnEditUndo();
}

void on_redo_button_clicked(GtkButton* button, gpointer user_data)
{
    mapView->OnEditRedo();
}

void on_clear_button_clicked(GtkButton* button, gpointer user_data)
{
    mapEditWnd->OnEditDelete();
}

void on_properties_button_clicked(GtkButton* button, gpointer user_data)
{
}

void on_zoomin_button_clicked(GtkButton* button, gpointer user_data)
{
    mapEditWnd->incZoom();
}

void on_zoomout_button_clicked(GtkButton* button, gpointer user_data)
{
    mapEditWnd->decZoom();
}

void on_underground_button_toggled(GtkToggleButton* togglebutton, gpointer user_data)
{
    mapView->OnViewUnderground();
    mapEditWnd->OnSize(1, mapEditWnd->_m_hWnd->allocation.width, mapEditWnd->_m_hWnd->allocation.width);
}

void on_grid_button_toggled(GtkToggleButton* togglebutton, gpointer user_data)
{
    TMapView::setViewGrid(!TMapView::getViewGrid());
}

void on_passability_button_toggled(GtkToggleButton* togglebutton, gpointer user_data)
{
    TMapView::setViewPassability(!TMapView::getViewPassability());
}

void on_placeterrain_button_clicked(GtkButton* button, gpointer user_data)
{
    gtk_notebook_set_page(toolNotebook, 0);
    mapView->OnToolsTerrain();
}

void on_placerivers_button_clicked(GtkButton* button, gpointer user_data)
{
    gtk_notebook_set_page(toolNotebook, 1);
    mapView->OnToolsRivers();
}

void on_placeroads_button_clicked(GtkButton* button, gpointer user_data)
{
    gtk_notebook_set_page(toolNotebook, 2);
    mapView->OnToolsRoads();
}

void on_eraseobjects_button_clicked(GtkButton* button, gpointer user_data)
{
    gtk_notebook_set_page(toolNotebook, 3);
    mapView->OnToolsErase();
}

void on_placedirt_button_clicked(GtkButton* button, gpointer user_data)
{
    gtk_notebook_set_page(toolNotebook, 4);
    mapView->OnToolsObjectsDirt();
}

void on_placesand_button_clicked(GtkButton* button, gpointer user_data)
{
    gtk_notebook_set_page(toolNotebook, 4);
    mapView->OnToolsObjectsSand();
}

void on_placegrass_button_clicked(GtkButton* button, gpointer user_data)
{
    gtk_notebook_set_page(toolNotebook, 4);
    mapView->OnToolsObjectsGrass();
}

void on_placesnow_button_clicked(GtkButton* button, gpointer user_data)
{
    gtk_notebook_set_page(toolNotebook, 4);
    mapView->OnToolsObjectsSnow();
}

void on_swamp_button_clicked(GtkButton* button, gpointer user_data)
{
    gtk_notebook_set_page(toolNotebook, 4);
    mapView->OnToolsObjectsSwamp();
}

void on_rough_button_clicked(GtkButton* button, gpointer user_data)
{
    gtk_notebook_set_page(toolNotebook, 4);
    mapView->OnToolsObjectsRough();
}

void on_subterranean_button_clicked(GtkButton* button, gpointer user_data)
{
    gtk_notebook_set_page(toolNotebook, 4);
    mapView->OnToolsObjectsSubterranean();
}

void on_lava_button_clicked(GtkButton* button, gpointer user_data)
{
    gtk_notebook_set_page(toolNotebook, 4);
    mapView->OnToolsObjectsLava();
}

void on_water_button_clicked(GtkButton* button, gpointer user_data)
{
    gtk_notebook_set_page(toolNotebook, 4);
    mapView->OnToolsObjectsWater();
}

void on_allterrain_button_clicked(GtkButton* button, gpointer user_data)
{
    gtk_notebook_set_page(toolNotebook, 4);
    mapView->OnToolsObjectsAllTerrain();
}

void on_towns_button_clicked(GtkButton* button, gpointer user_data)
{
    gtk_notebook_set_page(toolNotebook, 4);
    mapView->OnToolsObjectsTowns();
}

void on_monsters_button_clicked(GtkButton* button, gpointer user_data)
{
    gtk_notebook_set_page(toolNotebook, 4);
    mapView->OnToolsObjectsMonsters();
}

void on_heroes_button_clicked(GtkButton* button, gpointer user_data)
{
    gtk_notebook_set_page(toolNotebook, 4);
    mapView->OnToolsObjectsHeroes();
}

void on_artifacts_button_clicked(GtkButton* button, gpointer user_data)
{
    gtk_notebook_set_page(toolNotebook, 4);
    mapView->OnToolsObjectsArtifacts();
}

void on_treasures_button_clicked(GtkButton* button, gpointer user_data)
{
    gtk_notebook_set_page(toolNotebook, 4);
    mapView->OnToolsObjectsTreasures();
}

void on_about_ok_clicked(GtkButton* button, gpointer user_data)
{
    gtk_widget_hide(_widget("about_dlg"));
}

void on_mapeditwnd_draw(GtkWidget* widget, GdkRectangle* area, gpointer user_data)
{
    CRect rect(area->x, area->y, area->width + area->x, area->height + area->y);
    mapEditWnd->OnPaint(rect);
}

gboolean on_mapeditwnd_configure_event(GtkWidget* widget, GdkEventConfigure* event, gpointer user_data)
{
    mapEditWnd->OnSize(0, widget->allocation.width, widget->allocation.height);
    return FALSE;
}

gboolean on_mapeditwnd_expose_event(GtkWidget* widget, GdkEventExpose* event, gpointer user_data)
{
    GdkRectangle* area = &event->area;
    CRect rect(area->x, area->y, area->width + area->x, area->height + area->y);
    mapEditWnd->OnPaint(rect);
    return FALSE;
}

gboolean on_mapeditwnd_button_press_event(GtkWidget* widget, GdkEventButton* event, gpointer user_data)
{
    CPoint point((int)event->x, (int)event->y);
    if (event->button == 1)
        mapEditWnd->OnLButtonDown(event->state, point);
    else if (event->button == 2)
        mapEditWnd->OnMButtonDown(event->state, point);
    else if (event->button == 3)
        mapEditWnd->OnRButtonDown(event->state, point);
    return FALSE;
}

gboolean on_mapeditwnd_button_release_event(GtkWidget* widget, GdkEventButton* event, gpointer user_data)
{
    CPoint point((int)event->x, (int)event->y);
    if (event->button == 1)
        mapEditWnd->OnLButtonUp(event->state, point);
    if (event->button == 3)
        mapEditWnd->OnRButtonUp(event->state, point);
    return FALSE;
}

gboolean on_mapeditwnd_motion_notify_event(GtkWidget* widget, GdkEventMotion* event, gpointer user_data)
{
    CPoint point((int)event->x, (int)event->y);
    mapEditWnd->OnMouseMove(0, point);
    return FALSE;
}

gboolean on_minimapwnd_button_press_event(GtkWidget* widget, GdkEventButton* event, gpointer user_data)
{
    CPoint point((int)event->x, (int)event->y);
    if (event->button == 1)
        miniMapWnd->OnLButtonDown(event->state, point);
    return FALSE;
}

gboolean on_minimapwnd_button_release_event(GtkWidget* widget, GdkEventButton* event, gpointer user_data)
{
    CPoint point((int)event->x, (int)event->y);
    if (event->button == 1)
        miniMapWnd->OnLButtonUp(event->state, point);
    return FALSE;
}

gboolean on_minimapwnd_expose_event(GtkWidget* widget, GdkEventExpose* event, gpointer user_data)
{
    GdkRectangle* area = &event->area;
    CRect rect(area->x, area->y, area->width + area->x, area->height + area->y);
    miniMapWnd->OnPaint(rect);
    return TRUE;
}

gboolean on_minimapwnd_motion_notify_event(GtkWidget* widget, GdkEventMotion* event, gpointer user_data)
{
    CPoint point((int)event->x, (int)event->y);
    miniMapWnd->OnMouseMove(0, point);
    return FALSE;
}

gboolean on_minimapwnd_configure_event(GtkWidget* widget, GdkEventConfigure* event, gpointer user_data)
{
    miniMapWnd->OnSize(0, widget->allocation.width, widget->allocation.height);
    return FALSE;
}

void on_minimapwnd_draw(GtkWidget* widget, GdkRectangle* area, gpointer user_data)
{
    CRect rect(area->x, area->y, area->width + area->x, area->height + area->y);
    miniMapWnd->OnPaint(rect);
}

gboolean on_mapeditwnd_enter_notify_event(GtkWidget* widget, GdkEventCrossing* event, gpointer user_data)
{
    mapEditWnd->OnMouseEnter();
    return FALSE;
}

gboolean on_mapeditwnd_leave_notify_event(GtkWidget* widget, GdkEventCrossing* event, gpointer user_data)
{
    mapEditWnd->OnMouseLeave();
    return FALSE;
}

void on_hruler_draw(GtkWidget* widget, GdkRectangle* area, gpointer user_data)
{
    CRect rect(area->x, area->y, area->width + area->x, area->height + area->y);
    hRuler->OnPaint(rect);
}

gboolean on_hruler_expose_event(GtkWidget* widget, GdkEventExpose* event, gpointer user_data)
{
    on_hruler_draw(widget, &event->area, user_data);
    return TRUE;
}

gboolean on_hruler_configure_event(GtkWidget* widget, GdkEventConfigure* event, gpointer user_data)
{
    hRuler->OnSize(0, widget->allocation.width, widget->allocation.height);
    return FALSE;
}

gboolean on_vruler_configure_event(GtkWidget* widget, GdkEventConfigure* event, gpointer user_data)
{
    vRuler->OnSize(0, widget->allocation.width, widget->allocation.height);
    return FALSE;
}

void on_vruler_draw(GtkWidget* widget, GdkRectangle* area, gpointer user_data)
{
    CRect rect(area->x, area->y, area->width + area->x, area->height + area->y);
    vRuler->OnPaint(rect);
}

gboolean on_vruler_expose_event(GtkWidget* widget, GdkEventExpose* event, gpointer user_data)
{
    on_vruler_draw(widget, &event->area, user_data);
    return TRUE;
}

bool notebook_toggle(GtkToggleButton* button, int page, GtkToggleButton** pActiveButton)
{
    if (*pActiveButton == button) {
        if (!gtk_toggle_button_get_active(button))
            gtk_toggle_button_set_active(button, TRUE);
        return false;
    } else if (!gtk_toggle_button_get_active(button))
        return false;
    *pActiveButton = button;
    if (gtk_notebook_get_current_page(toolNotebook) != page)
        gtk_notebook_set_page(toolNotebook, page);
    return true;
}

void on_1x1brushtoggle_toggled(GtkToggleButton* togglebutton, gpointer user_data)
{
    if (!notebook_toggle(togglebutton, 0, &activeBrushToggle))
        return;
    GtkWidget* button = _widget("2x2brushtoggle");
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(button), FALSE);
    button = _widget("4x4brushtoggle");
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(button), FALSE);
    button = _widget("fillbrushtoggle");
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(button), FALSE);
    mapView->OnToolsTerrain1x1();
}

void on_2x2brushtoggle_toggled(GtkToggleButton* togglebutton, gpointer user_data)
{
    if (!notebook_toggle(togglebutton, 0, &activeBrushToggle))
        return;
    GtkWidget* button = _widget("1x1brushtoggle");
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(button), FALSE);
    button = _widget("4x4brushtoggle");
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(button), FALSE);
    button = _widget("fillbrushtoggle");
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(button), FALSE);
    mapView->OnToolsTerrain2x2();
}

void on_4x4brushtoggle_toggled(GtkToggleButton* togglebutton, gpointer user_data)
{
    if (!notebook_toggle(togglebutton, 0, &activeBrushToggle))
        return;
    GtkWidget* button = _widget("1x1brushtoggle");
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(button), FALSE);
    button = _widget("2x2brushtoggle");
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(button), FALSE);
    button = _widget("fillbrushtoggle");
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(button), FALSE);
    mapView->OnToolsTerrain4x4();
}

void on_fillbrushtoggle_toggled(GtkToggleButton* togglebutton, gpointer user_data)
{
    if (!notebook_toggle(togglebutton, 0, &activeBrushToggle))
        return;
    GtkWidget* button = _widget("1x1brushtoggle");
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(button), FALSE);
    button = _widget("2x2brushtoggle");
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(button), FALSE);
    button = _widget("4x4brushtoggle");
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(button), FALSE);
    mapView->OnToolsTerrainFill();
}

void on_sandtoggle_toggled(GtkToggleButton* togglebutton, gpointer user_data)
{
    if (!notebook_toggle(togglebutton, 0, &activeToolToggle))
        return;
    GtkWidget* button = _widget("dirttoggle");
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(button), FALSE);
    button = _widget("grasstoggle");
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(button), FALSE);
    button = _widget("snowtoggle");
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(button), FALSE);
    button = _widget("swamptoggle");
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(button), FALSE);
    button = _widget("roughtoggle");
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(button), FALSE);
    button = _widget("subterraneantoggle");
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(button), FALSE);
    button = _widget("lavatoggle");
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(button), FALSE);
    button = _widget("watertoggle");
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(button), FALSE);
    button = _widget("rocktoggle");
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(button), FALSE);
    mapView->OnToolsTerrainSand();
}

void on_grasstoggle_toggled(GtkToggleButton* togglebutton, gpointer user_data)
{
    if (!notebook_toggle(togglebutton, 0, &activeToolToggle))
        return;
    GtkWidget* button = _widget("dirttoggle");
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(button), FALSE);
    button = _widget("sandtoggle");
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(button), FALSE);
    button = _widget("snowtoggle");
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(button), FALSE);
    button = _widget("swamptoggle");
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(button), FALSE);
    button = _widget("roughtoggle");
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(button), FALSE);
    button = _widget("subterraneantoggle");
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(button), FALSE);
    button = _widget("lavatoggle");
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(button), FALSE);
    button = _widget("watertoggle");
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(button), FALSE);
    button = _widget("rocktoggle");
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(button), FALSE);
    mapView->OnToolsTerrainGrass();
}

void on_snowtoggle_toggled(GtkToggleButton* togglebutton, gpointer user_data)
{
    if (!notebook_toggle(togglebutton, 0, &activeToolToggle))
        return;
    GtkWidget* button = _widget("dirttoggle");
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(button), FALSE);
    button = _widget("sandtoggle");
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(button), FALSE);
    button = _widget("grasstoggle");
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(button), FALSE);
    button = _widget("swamptoggle");
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(button), FALSE);
    button = _widget("roughtoggle");
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(button), FALSE);
    button = _widget("subterraneantoggle");
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(button), FALSE);
    button = _widget("lavatoggle");
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(button), FALSE);
    button = _widget("watertoggle");
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(button), FALSE);
    button = _widget("rocktoggle");
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(button), FALSE);
    mapView->OnToolsTerrainSnow();
}

void on_swamptoggle_toggled(GtkToggleButton* togglebutton, gpointer user_data)
{
    if (!notebook_toggle(togglebutton, 0, &activeToolToggle))
        return;
    GtkWidget* button = _widget("dirttoggle");
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(button), FALSE);
    button = _widget("sandtoggle");
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(button), FALSE);
    button = _widget("grasstoggle");
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(button), FALSE);
    button = _widget("snowtoggle");
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(button), FALSE);
    button = _widget("roughtoggle");
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(button), FALSE);
    button = _widget("subterraneantoggle");
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(button), FALSE);
    button = _widget("lavatoggle");
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(button), FALSE);
    button = _widget("watertoggle");
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(button), FALSE);
    button = _widget("rocktoggle");
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(button), FALSE);
    mapView->OnToolsTerrainSwamp();
}

void on_roughtoggle_toggled(GtkToggleButton* togglebutton, gpointer user_data)
{
    if (!notebook_toggle(togglebutton, 0, &activeToolToggle))
        return;
    GtkWidget* button = _widget("dirttoggle");
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(button), FALSE);
    button = _widget("sandtoggle");
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(button), FALSE);
    button = _widget("grasstoggle");
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(button), FALSE);
    button = _widget("snowtoggle");
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(button), FALSE);
    button = _widget("swamptoggle");
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(button), FALSE);
    button = _widget("subterraneantoggle");
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(button), FALSE);
    button = _widget("lavatoggle");
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(button), FALSE);
    button = _widget("watertoggle");
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(button), FALSE);
    button = _widget("rocktoggle");
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(button), FALSE);
    mapView->OnToolsTerrainRough();
}

void on_subterraneantoggle_toggled(GtkToggleButton* togglebutton, gpointer user_data)
{
    if (!notebook_toggle(togglebutton, 0, &activeToolToggle))
        return;
    GtkWidget* button = _widget("dirttoggle");
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(button), FALSE);
    button = _widget("sandtoggle");
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(button), FALSE);
    button = _widget("grasstoggle");
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(button), FALSE);
    button = _widget("snowtoggle");
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(button), FALSE);
    button = _widget("swamptoggle");
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(button), FALSE);
    button = _widget("roughtoggle");
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(button), FALSE);
    button = _widget("lavatoggle");
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(button), FALSE);
    button = _widget("watertoggle");
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(button), FALSE);
    button = _widget("rocktoggle");
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(button), FALSE);
    mapView->OnToolsTerrainSubterranean();
}

void on_lavatoggle_toggled(GtkToggleButton* togglebutton, gpointer user_data)
{
    if (!notebook_toggle(togglebutton, 0, &activeToolToggle))
        return;
    GtkWidget* button = _widget("dirttoggle");
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(button), FALSE);
    button = _widget("sandtoggle");
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(button), FALSE);
    button = _widget("grasstoggle");
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(button), FALSE);
    button = _widget("snowtoggle");
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(button), FALSE);
    button = _widget("swamptoggle");
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(button), FALSE);
    button = _widget("roughtoggle");
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(button), FALSE);
    button = _widget("subterraneantoggle");
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(button), FALSE);
    button = _widget("watertoggle");
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(button), FALSE);
    button = _widget("rocktoggle");
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(button), FALSE);
    mapView->OnToolsTerrainLava();
}

void on_watertoggle_toggled(GtkToggleButton* togglebutton, gpointer user_data)
{
    if (!notebook_toggle(togglebutton, 0, &activeToolToggle))
        return;
    GtkWidget* button = _widget("dirttoggle");
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(button), FALSE);
    button = _widget("sandtoggle");
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(button), FALSE);
    button = _widget("grasstoggle");
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(button), FALSE);
    button = _widget("snowtoggle");
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(button), FALSE);
    button = _widget("swamptoggle");
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(button), FALSE);
    button = _widget("roughtoggle");
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(button), FALSE);
    button = _widget("subterraneantoggle");
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(button), FALSE);
    button = _widget("lavatoggle");
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(button), FALSE);
    button = _widget("rocktoggle");
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(button), FALSE);
    mapView->OnToolsTerrainWater();
}

void on_rocktoggle_toggled(GtkToggleButton* togglebutton, gpointer user_data)
{
    if (!notebook_toggle(togglebutton, 0, &activeToolToggle))
        return;
    GtkWidget* button = _widget("dirttoggle");
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(button), FALSE);
    button = _widget("sandtoggle");
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(button), FALSE);
    button = _widget("grasstoggle");
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(button), FALSE);
    button = _widget("snowtoggle");
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(button), FALSE);
    button = _widget("swamptoggle");
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(button), FALSE);
    button = _widget("roughtoggle");
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(button), FALSE);
    button = _widget("subterraneantoggle");
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(button), FALSE);
    button = _widget("lavatoggle");
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(button), FALSE);
    button = _widget("watertoggle");
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(button), FALSE);
    mapView->OnToolsTerrainRock();
}

void on_dirttoggle_toggled(GtkToggleButton* togglebutton, gpointer user_data)
{
    if (!notebook_toggle(togglebutton, 0, &activeToolToggle))
        return;
    GtkWidget* button = _widget("sandtoggle");
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(button), FALSE);
    button = _widget("grasstoggle");
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(button), FALSE);
    button = _widget("snowtoggle");
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(button), FALSE);
    button = _widget("swamptoggle");
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(button), FALSE);
    button = _widget("roughtoggle");
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(button), FALSE);
    button = _widget("subterraneantoggle");
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(button), FALSE);
    button = _widget("lavatoggle");
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(button), FALSE);
    button = _widget("watertoggle");
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(button), FALSE);
    button = _widget("rocktoggle");
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(button), FALSE);
    mapView->OnToolsTerrainDirt();
}

void on_clearrivtoggle_toggled(GtkToggleButton* togglebutton, gpointer user_data)
{
    if (!notebook_toggle(togglebutton, 1, &activeToolToggle))
        return;
    GtkWidget* button = _widget("icyrivtoggle");
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(button), FALSE);
    button = _widget("muddyrivtoggle");
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(button), FALSE);
    button = _widget("lavarivtoggle");
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(button), FALSE);
    mapView->OnToolsRiversClear();
}

void on_icyrivtoggle_toggled(GtkToggleButton* togglebutton, gpointer user_data)
{
    if (!notebook_toggle(togglebutton, 1, &activeToolToggle))
        return;
    GtkWidget* button = _widget("clearrivtoggle");
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(button), FALSE);
    button = _widget("muddyrivtoggle");
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(button), FALSE);
    button = _widget("lavarivtoggle");
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(button), FALSE);
    mapView->OnToolsRiversIcy();
}

void on_muddyrivtoggle_toggled(GtkToggleButton* togglebutton, gpointer user_data)
{
    if (!notebook_toggle(togglebutton, 1, &activeToolToggle))
        return;
    GtkWidget* button = _widget("clearrivtoggle");
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(button), FALSE);
    button = _widget("icyrivtoggle");
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(button), FALSE);
    button = _widget("lavarivtoggle");
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(button), FALSE);
    mapView->OnToolsRiversMuddy();
}

void on_lavarivtoggle_toggled(GtkToggleButton* togglebutton, gpointer user_data)
{
    if (!notebook_toggle(togglebutton, 1, &activeToolToggle))
        return;
    GtkWidget* button = _widget("clearrivtoggle");
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(button), FALSE);
    button = _widget("icyrivtoggle");
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(button), FALSE);
    button = _widget("muddyrivtoggle");
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(button), FALSE);
    mapView->OnToolsRiversLava();
}

void on_dirtroadtoggle_toggled(GtkToggleButton* togglebutton, gpointer user_data)
{
    if (!notebook_toggle(togglebutton, 2, &activeToolToggle))
        return;
    GtkWidget* button = _widget("gravelroadtoggle");
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(button), FALSE);
    button = _widget("cobblestoneroadtoggle");
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(button), FALSE);
    mapView->OnToolsRoadsDirt();
}

void on_gravelroadtoggle_toggled(GtkToggleButton* togglebutton, gpointer user_data)
{
    if (!notebook_toggle(togglebutton, 2, &activeToolToggle))
        return;
    GtkWidget* button = _widget("dirtroadtoggle");
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(button), FALSE);
    button = _widget("cobblestoneroadtoggle");
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(button), FALSE);
    mapView->OnToolsRoadsGravel();
}

void on_cobblestoneroadtoggle_toggled(GtkToggleButton* togglebutton, gpointer user_data)
{
    if (!notebook_toggle(togglebutton, 2, &activeToolToggle))
        return;
    GtkWidget* button = _widget("dirtroadtoggle");
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(button), FALSE);
    button = _widget("gravelroadtoggle");
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(button), FALSE);
    mapView->OnToolsRoadsCobblestone();
}

void on_1x1erasertoggle_toggled(GtkToggleButton* togglebutton, gpointer user_data)
{
    if (!notebook_toggle(togglebutton, 3, &activeBrushToggle))
        return;
    GtkWidget* button = _widget("2x2erasertoggle");
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(button), FALSE);
    button = _widget("4x4erasertoggle");
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(button), FALSE);
    button = _widget("fillerasertoggle");
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(button), FALSE);
    mapView->OnToolsErase1x1();
}

void on_2x2erasertoggle_toggled(GtkToggleButton* togglebutton, gpointer user_data)
{
    if (!notebook_toggle(togglebutton, 3, &activeBrushToggle))
        return;
    GtkWidget* button = _widget("1x1erasertoggle");
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(button), FALSE);
    button = _widget("4x4erasertoggle");
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(button), FALSE);
    button = _widget("fillerasertoggle");
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(button), FALSE);
    mapView->OnToolsErase2x2();
}

void on_4x4erasertoggle_toggled(GtkToggleButton* togglebutton, gpointer user_data)
{
    if (!notebook_toggle(togglebutton, 3, &activeBrushToggle))
        return;
    GtkWidget* button = _widget("1x1erasertoggle");
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(button), FALSE);
    button = _widget("2x2erasertoggle");
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(button), FALSE);
    button = _widget("fillerasertoggle");
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(button), FALSE);
    mapView->OnToolsErase4x4();
}

void on_fillerasertoggle_toggled(GtkToggleButton* togglebutton, gpointer user_data)
{
    if (!notebook_toggle(togglebutton, 3, &activeBrushToggle))
        return;
    GtkWidget* button = _widget("1x1erasertoggle");
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(button), FALSE);
    button = _widget("2x2erasertoggle");
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(button), FALSE);
    button = _widget("4x4erasertoggle");
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(button), FALSE);
    mapView->OnToolsEraseFill();
}


gboolean on_objpalettewnd_expose_event(GtkWidget* widget, GdkEventExpose* event, gpointer user_data)
{
    GdkRectangle* area = &event->area;
    CRect rect(area->x, area->y, area->width + area->x, area->height + area->y);
    objPaletteWnd->OnPaint(rect);
    return FALSE;
}

gboolean on_objpalettewnd_leave_notify_event(GtkWidget* widget, GdkEventCrossing* event, gpointer user_data)
{
    objPaletteWnd->OnMouseLeave();
    return FALSE;
}

void on_objpalettewnd_draw(GtkWidget* widget, GdkRectangle* area, gpointer user_data)
{
    CRect rect(area->x, area->y, area->width + area->x, area->height + area->y);
    objPaletteWnd->OnPaint(rect);
}

gboolean on_objpalettewnd_configure_event(GtkWidget* widget, GdkEventConfigure* event, gpointer user_data)
{
    objPaletteWnd->OnSize(0, widget->allocation.width, widget->allocation.height);
    return FALSE;
}

gboolean on_objpalettewnd_motion_notify_event(GtkWidget* widget, GdkEventMotion* event, gpointer user_data)
{
    CPoint point((int)event->x, (int)event->y);
    objPaletteWnd->OnMouseMove(0, point);
    return FALSE;
}

gboolean on_objpalettewnd_button_press_event(GtkWidget* widget, GdkEventButton* event, gpointer user_data)
{
    CPoint point((int)event->x, (int)event->y);
    if (event->button == 1)
        objPaletteWnd->OnLButtonDown(event->state, point);
    return FALSE;
}

gboolean on_objpalettewnd_button_release_event(GtkWidget* widget, GdkEventButton* event, gpointer user_data)
{
    return FALSE;
}

void updateEraserBrush()
{
    GtkToggleButton* button1x1 = GTK_TOGGLE_BUTTON(_widget("1x1erasertoggle"));
    GtkToggleButton* button2x2 = GTK_TOGGLE_BUTTON(_widget("2x2erasertoggle"));
    GtkToggleButton* button4x4 = GTK_TOGGLE_BUTTON(_widget("4x4erasertoggle"));
    GtkToggleButton* buttonFill = GTK_TOGGLE_BUTTON(_widget("fillerasertoggle"));
    if (gtk_toggle_button_get_active(button1x1) == TRUE)
        mapView->OnToolsErase1x1();
    else if (gtk_toggle_button_get_active(button2x2) == TRUE)
        mapView->OnToolsErase2x2();
    else if (gtk_toggle_button_get_active(button4x4) == TRUE)
        mapView->OnToolsErase4x4();
    else if (gtk_toggle_button_get_active(buttonFill) == TRUE)
        mapView->OnToolsEraseFill();
    else
        gtk_toggle_button_set_active(button2x2, TRUE);
}

void updateTerrainBrush()
{
    GtkToggleButton* button1x1 = GTK_TOGGLE_BUTTON(_widget("1x1brushtoggle"));
    GtkToggleButton* button2x2 = GTK_TOGGLE_BUTTON(_widget("2x2brushtoggle"));
    GtkToggleButton* button4x4 = GTK_TOGGLE_BUTTON(_widget("4x4brushtoggle"));
    GtkToggleButton* buttonFill = GTK_TOGGLE_BUTTON(_widget("fillbrushtoggle"));
    if (gtk_toggle_button_get_active(button1x1) == TRUE)
        mapView->OnToolsTerrain1x1();
    else if (gtk_toggle_button_get_active(button2x2) == TRUE)
        mapView->OnToolsTerrain2x2();
    else if (gtk_toggle_button_get_active(button4x4) == TRUE)
        mapView->OnToolsTerrain4x4();
    else if (gtk_toggle_button_get_active(buttonFill) == TRUE)
        mapView->OnToolsTerrainFill();
    else
        gtk_toggle_button_set_active(button2x2, TRUE);
}

void on_toolnotebook_switch_page(GtkNotebook* notebook, GtkNotebookPage* page, gint page_num,
                                 gpointer user_data)
{
    switch (page_num) {
    case 4:
        mapView->OnToolsObjects();
        break;
    case 3:
        mapView->OnToolsErase();
        updateEraserBrush();
        break;
    case 2:
        mapView->OnToolsRoads();
        break;
    case 1:
        mapView->OnToolsRivers();
        break;
    case 0:
        mapView->OnToolsTerrain();
        updateTerrainBrush();
        break;
    default:
#line 2957
        assert(0);
        break;
    }
}

gboolean on_MainWindow_key_press_event(GtkWidget* widget, GdkEventKey* event, gpointer user_data)
{
    switch (event->keyval) {
    case GDK_Up:
    case GDK_KP_Up:
        mapEditWnd->OnVScroll(0x34);
        break;
    case GDK_Down:
    case GDK_KP_Down:
        mapEditWnd->OnVScroll(0x35);
        break;
    case GDK_Page_Up:
    case GDK_KP_Page_Up:
        mapEditWnd->OnVScroll(0x36);
        break;
    case GDK_Page_Down:
    case GDK_KP_Page_Down:
        mapEditWnd->OnVScroll(0x37);
        break;
    case GDK_Left:
    case GDK_KP_Left:
        mapEditWnd->OnHScroll(0x31);
        break;
    case GDK_Right:
    case GDK_KP_Right:
        mapEditWnd->OnHScroll(0x30);
        break;
    }
    return FALSE;
}

gboolean on_MainWindow_key_release_event(GtkWidget* widget, GdkEventKey* event, gpointer user_data)
{
    GtkObject* hScroll = GTK_OBJECT(_widget("maphscroll"));
    GtkObject* vScroll = GTK_OBJECT(_widget("mapvscroll"));
    gtk_signal_emit_by_name(hScroll, "key-release-event", widget, event, user_data);
    gtk_signal_emit_by_name(vScroll, "key-release-event", widget, event, user_data);
    return FALSE;
}

void on_abort_button_clicked(GtkButton* button, gpointer user_data)
{
    gtk_main_quit();
}

void on_options_apply_clicked(GtkButton* button, gpointer user_data)
{
    TMapDoc* pDoc = mapView->getPDocument();
    GtkRange* scale = GTK_RANGE(_widget("tile_freq_scale"));
    GtkAdjustment* adjustment = gtk_range_get_adjustment(scale);
    unsigned int frequency = (unsigned int)adjustment->value;
    TMapDoc::setSpecialTileFrequency(frequency);
    GtkToggleButton* repaintButton = GTK_TOGGLE_BUTTON(_widget("repaint_map"));
    if (gtk_toggle_button_get_active(repaintButton)) {
        pDoc->OnToolsRepaintMap();
        mapEditWnd->OnPaint();
        miniMapWnd->OnPaint();
    }
}

void on_options_ok_clicked(GtkButton* button, gpointer user_data)
{
    on_options_apply_clicked(button, user_data);
    gtk_widget_hide(_widget("options_dlg"));
}

void on_options_cancel_clicked(GtkButton* button, gpointer user_data)
{
    gtk_widget_hide(_widget("options_dlg"));
}

}
}
