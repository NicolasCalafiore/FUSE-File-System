#define FUSE_USE_VERSION 26
#define _FILE_OFFSET_BITS 64

#include <fuse.h>           // libfuse‑dev (FUSE 2) – if you installed libfuse3‑dev, change to <fuse3/fuse.h>
#include <cstring>
#include <cerrno>
#include <unistd.h>
#include <limits.h>
#include <iostream>
#include <vector>
#include "Wad/Wad.h"

/* ----------------------------------------------------------------
 * Helpers
 * ----------------------------------------------------------------*/
static inline Wad* wad() {
    // Retrieve the Wad pointer we passed to fuse_main()
    return static_cast<Wad*>(fuse_get_context()->private_data);
}

static inline std::string toStr(const char* p) { return std::string(p); }

/* ----------------------------------------------------------------
 * Callback implementations
 * ----------------------------------------------------------------*/
static int fs_getattr(const char* path, struct stat* st) {
    memset(st, 0, sizeof(*st));
    std::string p = toStr(path);
    if (wad()->isDirectory(p)) {
        st->st_mode = S_IFDIR | 0777;
        st->st_nlink = 2;
        return 0;
    }
    if (wad()->isContent(p)) {
        st->st_mode = S_IFREG | 0777;
        st->st_nlink = 1;
        st->st_size  = wad()->getSize(p);
        return 0;
    }
    return -ENOENT;
}

static int fs_readdir(const char* path, void* buf, fuse_fill_dir_t filler,
                      off_t, struct fuse_file_info*) {
    std::string p = toStr(path);
    if (!wad()->isDirectory(p)) return -ENOTDIR;

    filler(buf, ".",  nullptr, 0);
    filler(buf, "..", nullptr, 0);

    std::vector<std::string> entries;
    if (wad()->getDirectory(p, &entries) < 0) return -ENOTDIR;
    for (const auto& name : entries)
        filler(buf, name.c_str(), nullptr, 0);
    return 0;
}

static int fs_open(const char* path, struct fuse_file_info*) {
    return wad()->isContent(toStr(path)) ? 0 : -EISDIR;
}

static int fs_read(const char* path, char* buf, size_t size, off_t off,
                   struct fuse_file_info*) {
    int n = wad()->getContents(toStr(path), buf, static_cast<int>(size), static_cast<int>(off));
    return (n >= 0) ? n : -EISDIR;
}

static int fs_write(const char* path, const char* buf, size_t size, off_t off,
                    struct fuse_file_info*) {
    int n = wad()->writeToFile(toStr(path), buf, static_cast<int>(size), static_cast<int>(off));
    return (n >= 0) ? n : -EACCES;
}

static int fs_mkdir(const char* path, mode_t) {
    wad()->createDirectory(toStr(path));
    return wad()->isDirectory(toStr(path)) ? 0 : -EACCES;
}

static int fs_mknod(const char* path, mode_t, dev_t) {
    wad()->createFile(toStr(path));
    return wad()->isContent(toStr(path)) ? 0 : -EACCES;
}

/* ----------------------------------------------------------------
 * Operations table (C++11 lambda builds & returns a fully‑initialised struct)
 * ----------------------------------------------------------------*/
static struct fuse_operations operations = []{
    struct fuse_operations op{};     // zero‑initialize all fields
    op.getattr = fs_getattr;
    op.readdir = fs_readdir;
    op.open    = fs_open;
    op.read    = fs_read;
    op.write   = fs_write;
    op.mkdir   = fs_mkdir;
    op.mknod   = fs_mknod;
    return op;
}();

/* ----------------------------------------------------------------
 * Main – kept exactly as provided by the user
 * ----------------------------------------------------------------*/
//https://www.youtube.com/watch?v=aMlX2x5N9Ak&ab_channel=HuiYuan
#include <iostream>

using namespace std;
int main(int argc, char* argv[]) {
    if (argc < 3) {
        std::cout << "Not enough arguments" << endl;
        exit(EXIT_SUCCESS);
    }

    std::string wadPath = argv[argc - 2];

    if (wadPath.at(0) != '/') {
        wadPath = string(get_current_dir_name()) + "/" + wadPath;
    }

    Wad *wad = Wad::loadWad(wadPath);

    argv[argc - 2] = argv[argc - 1];
    argc--;

    //((Wad*) fuse_get_context()->private_data)->getContents
    return fuse_main(argc, argv, &operations, wad);
}
