#include "utils.h"
static void testPutFile(char* path, char* contents);
static int testSystem(const char* command);
#define putFile testPutFile
#define system testSystem
#define main nextui_app_main
#include "../workspace/all/nextui/nextui.c"
#undef main
#undef system
#undef putFile

#include <stdarg.h>
#include <errno.h>

// The fixture exercises list/navigation/config code without starting SDL or games.
void LOG_note(int level, const char* fmt, ...) { (void)level; (void)fmt; }

void PWR_powerOff(int reboot) { (void)reboot; assert(!"Unexpected power action"); }
void PWR_sleep(void) { assert(!"Unexpected sleep action"); }
bool PLAT_wifiEnabled(void) { assert(!"Unexpected Wi-Fi action"); return false; }
void PLAT_wifiEnable(bool on) { (void)on; assert(!"Unexpected Wi-Fi action"); }
bool PLAT_bluetoothEnabled(void) { assert(!"Unexpected Bluetooth action"); return false; }
void PLAT_bluetoothEnable(bool on) { (void)on; assert(!"Unexpected Bluetooth action"); }

static char queued_command[1024];
static void testPutFile(char* path, char* contents) {
	if (!strcmp(path, "/tmp/next")) snprintf(queued_command, sizeof(queued_command), "%s", contents);
	else putFile(path, contents);
}
static int testSystem(const char* command) { (void)command; return 0; }

static void makeDirectory(const char* path) {
	char copy[PATH_MAX];
	snprintf(copy, sizeof(copy), "%s", path);
	for (char* p = copy + 1; *p; p++) {
		if (*p != '/') continue;
		*p = '\0';
		assert(mkdir(copy, 0700) == 0 || errno == EEXIST);
		*p = '/';
	}
	assert(mkdir(copy, 0700) == 0 || errno == EEXIST);
}

static void writeFile(const char* path, const char* value) {
	char parent[PATH_MAX];
	snprintf(parent, sizeof(parent), "%s", path);
	*strrchr(parent, '/') = '\0';
	makeDirectory(parent);
	FILE* file = fopen(path, "w");
	assert(file);
	assert(fputs(value, file) >= 0);
	assert(fclose(file) == 0);
}

static Entry* findEntry(Array* entries, const char* path) {
	int index = EntryArray_indexOf(entries, (char*)path);
	assert(index >= 0);
	return entries->items[index];
}

static void expectAbsent(Array* entries, const char* path) {
	assert(EntryArray_indexOf(entries, (char*)path) == -1);
}

static void testConfig(void) {
	makeDirectory(SHARED_USERDATA_PATH);
	assert(setenv("SHARED_USERDATA_PATH", SHARED_USERDATA_PATH, 1) == 0);
	// A built-in font entry avoids the pre-existing CFG_setFontFile self-copy fallback.
	writeFile(SHARED_USERDATA_PATH "/minuisettings.txt", "font=1\n");
	CFG_init(NULL, NULL);
	assert(CFG_getLibraryView() == LIBRARY_VIEW_SYSTEMS);
	CFG_setLibraryView(LIBRARY_VIEW_ALL_GAMES);
	char value[256];
	CFG_get("libraryView", value);
	assert(!strcmp(value, "1"));
	FILE* json = tmpfile();
	assert(json);
	fflush(stdout);
	int original_stdout = dup(STDOUT_FILENO);
	assert(original_stdout >= 0 && dup2(fileno(json), STDOUT_FILENO) >= 0);
	CFG_print();
	fflush(stdout);
	assert(dup2(original_stdout, STDOUT_FILENO) >= 0);
	close(original_stdout);
	rewind(json);
	char json_text[8192];
	size_t length = fread(json_text, 1, sizeof(json_text) - 1, json);
	json_text[length] = '\0';
	assert(strstr(json_text, "\"libraryView\": 1,"));
	fclose(json);
	CFG_init(NULL, NULL);
	assert(CFG_getLibraryView() == LIBRARY_VIEW_ALL_GAMES);
	CFG_setLibraryView(-1);
	assert(CFG_getLibraryView() == LIBRARY_VIEW_SYSTEMS);
	CFG_setLibraryView(99);
	assert(CFG_getLibraryView() == LIBRARY_VIEW_SYSTEMS);
	// Old configuration files lack this key and retain the default.
	writeFile(SHARED_USERDATA_PATH "/minuisettings.txt", "font=1\nrecents=0\n");
	CFG_init(NULL, NULL);
	assert(CFG_getLibraryView() == LIBRARY_VIEW_SYSTEMS);
	writeFile(SHARED_USERDATA_PATH "/minuisettings.txt", "font=1\nlibraryView=99\n");
	CFG_init(NULL, NULL);
	assert(CFG_getLibraryView() == LIBRARY_VIEW_SYSTEMS);
	CFG_setLibraryView(CFG_DEFAULT_LIBRARY_VIEW);
	assert(CFG_getLibraryView() == LIBRARY_VIEW_SYSTEMS);
	CFG_setLibraryView(LIBRARY_VIEW_ALL_GAMES);
}

static void makeLibrary(void) {
	writeFile(PAKS_PATH "/Emus/GB.pak/launch.sh", "");
	writeFile(PAKS_PATH "/Emus/GBC.pak/launch.sh", "");
	writeFile(PAKS_PATH "/Emus/PS.pak/launch.sh", "");
	writeFile(ROMS_PATH "/Game Boy (GB)/Same.gb", "");
	writeFile(ROMS_PATH "/Game Boy (GB)/rename.gb", "");
	writeFile(ROMS_PATH "/Game Boy (GB)/hidden.gb", "");
	writeFile(ROMS_PATH "/Game Boy (GB)/.dot.gb", "");
	writeFile(ROMS_PATH "/Game Boy (GB)/off.gb.disabled", "");
	writeFile(ROMS_PATH "/Game Boy (GB)/map.txt", "rename.gb\tAlias\r\nhidden.gb\t.hidden\nHidden folder\t.disabled\n");
	writeFile(ROMS_PATH "/Game Boy (GB)/Hidden folder/invisible.gb", "");
	writeFile(ROMS_PATH "/Game Boy (GB)/Nested/deep.gb", "");
	writeFile(ROMS_PATH "/Game Boy (GB)/Nested/map.txt", "deep.gb\tDeep Alias\n");
	writeFile(ROMS_PATH "/Game Boy (GB)/Other/Same.gb", "");
	writeFile(ROMS_PATH "/Game Boy (GB)/App.pak/launch.sh", "");
	writeFile(ROMS_PATH "/Game Boy (GBC)/Same.gb", "");
	writeFile(ROMS_PATH "/Game Boy (GBC)/color.gbc", "");
	writeFile(ROMS_PATH "/Game Boy (GBC)/map.txt", "color.gbc\tColor Alias\n");
	writeFile(ROMS_PATH "/Hidden (GB)/hidden.gb", "");
	writeFile(ROMS_PATH "/Unsupported (NONE)/unavailable.rom", "");
	writeFile(ROMS_PATH "/.Hidden (GB)/secret.gb", "");
	makeDirectory(ROMS_PATH "/Empty (GB)/Empty child");
	writeFile(ROMS_PATH "/PlayStation (PS)/Cue Game/Cue Game.cue", "");
	writeFile(ROMS_PATH "/PlayStation (PS)/Cue Game/track.bin", "");
	writeFile(ROMS_PATH "/PlayStation (PS)/Multi Game/Multi Game.m3u", "disc1.chd\ndisc2.chd\n");
	writeFile(ROMS_PATH "/PlayStation (PS)/Multi Game/disc1.chd", "");
	writeFile(ROMS_PATH "/PlayStation (PS)/Multi Game/disc2.chd", "");
	writeFile(ROMS_PATH "/PlayStation (PS)/single.m3u", "single.chd\n");
	writeFile(ROMS_PATH "/PlayStation (PS)/single.chd", "");
	writeFile(ROMS_PATH "/Game Boy (GB)/compressed.zip", "");
	writeFile(ROMS_PATH "/map.txt", "Hidden (GB)\t.hidden\nSame.gb\tWRONG ROOT ALIAS\n");
	writeFile(SDCARD_PATH "/map.txt", "Same.gb\tWRONG CARD ALIAS\n");
	assert(symlink(ROMS_PATH "/Game Boy (GB)", ROMS_PATH "/Game Boy (GB)/loop") == 0);
	assert(symlink(ROMS_PATH "/Game Boy (GB)/Same.gb", ROMS_PATH "/Game Boy (GB)/link.gb") == 0);
	char long_alias[241];
	memset(long_alias, 'L', sizeof(long_alias) - 1);
	long_alias[sizeof(long_alias) - 1] = '\0';
	char long_map[256];
	snprintf(long_map, sizeof(long_map), "long.gb\t%s\n", long_alias);
	writeFile(ROMS_PATH "/Game Boy (GB)/Long/long.gb", "");
	writeFile(ROMS_PATH "/Game Boy (GB)/Long/map.txt", long_map);
	writeFile(ROMS_PATH "/Game Boy (GB)/Long2/long.gb", "");
	writeFile(ROMS_PATH "/Game Boy (GB)/Long2/map.txt", long_map);
	// Bounds: these exceed the existing launch helpers' 256-byte path capacity.
	char long_dir[256];
	memset(long_dir, 'x', 220);
	long_dir[220] = '\0';
	char long_path[PATH_MAX];
	snprintf(long_path, sizeof(long_path), "%s/Game Boy (GB)/%s/game.gb", ROMS_PATH, long_dir);
	writeFile(long_path, "");
}

static void testLibrary(void) {
	Array* entries = getAllGames();
	assert(entries->count == 14);
	Entry* long_title = findEntry(entries, ROMS_PATH "/Game Boy (GB)/Long/long.gb");
	assert(strlen(long_title->unique) > 255);
	Entry* same_gb = findEntry(entries, ROMS_PATH "/Game Boy (GB)/Same.gb");
	Entry* same_gbc = findEntry(entries, ROMS_PATH "/Game Boy (GBC)/Same.gb");
	Entry* same_nested = findEntry(entries, ROMS_PATH "/Game Boy (GB)/Other/Same.gb");
	assert(!strcmp(same_gb->name, "Same"));
	assert(strcmp(same_gb->unique, same_gbc->unique));
	assert(strcmp(same_gb->unique, same_nested->unique));
	assert(strstr(same_gbc->unique, "GBC"));
	assert(!strcmp(findEntry(entries, ROMS_PATH "/Game Boy (GB)/rename.gb")->name, "Alias"));
	assert(!strcmp(findEntry(entries, ROMS_PATH "/Game Boy (GB)/Nested/deep.gb")->name, "Deep Alias"));
	assert(!strcmp(findEntry(entries, ROMS_PATH "/Game Boy (GBC)/color.gbc")->name, "Color Alias"));
	assert(findEntry(entries, ROMS_PATH "/Game Boy (GB)/App.pak")->type == ENTRY_PAK);
	assert(findEntry(entries, ROMS_PATH "/PlayStation (PS)/Cue Game")->type == ENTRY_DIR);
	assert(findEntry(entries, ROMS_PATH "/PlayStation (PS)/Multi Game")->type == ENTRY_DIR);
	assert(findEntry(entries, ROMS_PATH "/Game Boy (GB)/compressed.zip")->type == ENTRY_ROM);
	expectAbsent(entries, ROMS_PATH "/PlayStation (PS)/Multi Game/disc1.chd");
	expectAbsent(entries, ROMS_PATH "/PlayStation (PS)/Cue Game/track.bin");
	expectAbsent(entries, ROMS_PATH "/Game Boy (GB)/App.pak/launch.sh");
	expectAbsent(entries, ROMS_PATH "/Game Boy (GB)/hidden.gb");
	expectAbsent(entries, ROMS_PATH "/Game Boy (GB)/Hidden folder/invisible.gb");
	expectAbsent(entries, ROMS_PATH "/Hidden (GB)/hidden.gb");
	expectAbsent(entries, ROMS_PATH "/Unsupported (NONE)/unavailable.rom");
	expectAbsent(entries, ROMS_PATH "/Game Boy (GB)/link.gb");
	for (int i = 1; i < entries->count; i++)
		assert(compareLibraryEntries(&entries->items[i-1], &entries->items[i]) < 0);
	EntryArray_free(entries);

	Directory* dir = Directory_new(ROMS_PATH, 0);
	assert(dir->entries->count == 14);
	assert(!strcmp(findEntry(dir->entries, ROMS_PATH "/Game Boy (GB)/Same.gb")->name, "Same"));
	Directory_free(dir);

	// Systems mode keeps its existing display-name collation and folder navigation.
	CFG_setLibraryView(LIBRARY_VIEW_SYSTEMS);
	dir = Directory_new(ROMS_PATH, 0);
	assert(dir->entries->count == 3); // Empty, Game Boy (collated), PlayStation.
	Directory_free(dir);
	CFG_setLibraryView(LIBRARY_VIEW_ALL_GAMES);
}

static void freshRoot(void) {
	if (stack) DirectoryArray_free(stack);
	stack = Array_new();
	top = NULL;
	if (recents) RecentArray_free(recents);
	recents = Array_new();
	openDirectory(SDCARD_PATH, 0);
}

static void restoreEntry(const char* path, const char* expected) {
	freshRoot();
	writeFile(LAST_PATH, path);
	loadLast();
	assert(stack->count == 1);
	assert(!strcmp(((Entry*)top->entries->items[top->selected])->path, expected));
	assert(top->selected >= top->start && top->selected < top->end);
}

static void testRestore(void) {
	CFG_setShowRecents(true);
	CFG_setShowTools(false);
	CFG_setShowCollections(true);
	restoreEntry(ROMS_PATH "/Game Boy (GB)/Nested/deep.gb", ROMS_PATH "/Game Boy (GB)/Nested/deep.gb");
	restoreEntry(ROMS_PATH "/PlayStation (PS)/Multi Game", ROMS_PATH "/PlayStation (PS)/Multi Game");
	restoreEntry(ROMS_PATH "/PlayStation (PS)/Multi Game/disc1.chd", ROMS_PATH "/PlayStation (PS)/Multi Game");
	restoreEntry(ROMS_PATH "/PlayStation (PS)/single.m3u", ROMS_PATH "/PlayStation (PS)/single.m3u");
	freshRoot();
	openDirectory(ROMS_PATH, 0); // Quick Menu > Games uses the same flattened list.
	assert(top->entries->count == 14);
	closeDirectory();
	assert(stack->count == 1);
	writeFile(RECENT_PATH, "/Roms/Game Boy (GB)/Same.gb\tSame\n");
	freshRoot();
	writeFile(LAST_PATH, FAUX_RECENT_PATH);
	loadLast();
	assert(!strcmp(top->path, FAUX_RECENT_PATH));
	assert(top->entries->count == 1);
	writeFile(COLLECTIONS_PATH "/Favorites.txt", "/Roms/Game Boy (GB)/Same.gb\n");
	freshRoot();
	writeFile(LAST_PATH, COLLECTIONS_PATH "/Favorites.txt/Same.gb");
	loadLast();
	assert(!strcmp(top->path, COLLECTIONS_PATH "/Favorites.txt"));
	assert(top->entries->count == 1);
}

static void testLaunchContext(void) {
	freshRoot();
	Entry* playlist = findEntry(top->entries, ROMS_PATH "/PlayStation (PS)/single.m3u");
	Entry_open(playlist);
	char last[256];
	getFile(LAST_PATH, last, sizeof(last));
	assert(!strcmp(last, playlist->path));
	assert(strstr(queued_command, "/PlayStation (PS)/single.chd"));
	Entry* nested = findEntry(top->entries, ROMS_PATH "/Game Boy (GB)/Nested/deep.gb");
	Entry_open(nested);
	getFile(LAST_PATH, last, sizeof(last));
	assert(!strcmp(last, nested->path));
	char recent_text[4096];
	getFile(RECENT_PATH, recent_text, sizeof(recent_text));
	assert(strstr(recent_text, "deep.gb\tDeep Alias"));
	assert(!strstr(recent_text, "Deep Alias (GB)"));
	Entry* multidisc = findEntry(top->entries, ROMS_PATH "/PlayStation (PS)/Multi Game");
	Entry_open(multidisc);
	getFile(LAST_PATH, last, sizeof(last));
	assert(!strcmp(last, multidisc->path));
	assert(strstr(queued_command, "/Multi Game/disc1.chd"));
	quit = 0;
	startgame = 0;
}

static void testAlphaCapacity(void) {
	Directory dir = {.path = SDCARD_PATH, .entries = Array_new(), .alphas = IntArray_new()};
	Array_push(dir.entries, Entry_newNamed(FAUX_RECENT_PATH, ENTRY_DIR, "Recents"));
	Array_push(dir.entries, Entry_newNamed(COLLECTIONS_PATH, ENTRY_DIR, "Collections"));
	for (char letter = 'A'; letter <= 'Z'; letter++) {
		char name[] = {letter, '\0'};
		Array_push(dir.entries, Entry_newNamed(ROMS_PATH "/Game Boy (GB)/x.gb", ENTRY_ROM, name));
	}
	Array_push(dir.entries, Entry_newNamed(TOOLS_PATH, ENTRY_DIR, "Tools"));
	Directory_index(&dir);
	assert(dir.alphas->count == 29);
	assert(dir.alphas->items[28] == 28);
	for (int i = 0; i < dir.entries->count; i++)
		assert(((Entry*)dir.entries->items[i])->alpha == i);
	EntryArray_free(dir.entries);
	IntArray_free(dir.alphas);
}

static void testEmpty(void) {
	// Hide every system using its source map: plain empty subfolders yield no games.
	writeFile(ROMS_PATH "/map.txt", "Game Boy (GB)\t.hidden\nGame Boy (GBC)\t.hidden\nPlayStation (PS)\t.hidden\nHidden (GB)\t.hidden\n");
	CFG_setShowCollections(false);
	CFG_setShowRecents(false);
	freshRoot();
	assert(top->entries->count == 0);
	assert(top->alphas->count == 0);
	readyResume(NULL);
	assert(!can_resume && !has_preview);
	openDirectory(ROMS_PATH, 0);
	assert(top->entries->count == 0);
	closeDirectory();
	assert(top->entries->count == 0);
}

int main(void) {
	testConfig();
	makeLibrary();
	testLibrary();
	testRestore();
	testLaunchContext();
	testAlphaCapacity();
	testEmpty();
	DirectoryArray_free(stack);
	RecentArray_free(recents);
	puts("Library fixtures passed");
	return 0;
}
