#define FUSE_USE_VERSION 28
#define _FILE_OFFSET_BITS 64

#include <fuse.h>
#include <cstring>
#include <cerrno>
#include <unistd.h>
#include <limits.h>
#include <iostream>
#include <vector>
#include "../libWad/Wad.h"

using namespace std;
//https://www.youtube.com/watch?v=aMlX2x5N9Ak&ab_channel=HuiYuan
//https://www.youtube.com/watch?v=LZCILvr5tUk&ab_channel=StefanK
//https://libfuse.github.io/doxygen/structfuse__operations.html
//https://engineering.facile.it/blog/eng/write-filesystem-fuse/
//https://www.cs.hmc.edu/~geoff/classes/hmc.cs137.201601/homework/fuse/fuse_doc.html

static inline Wad *get_wad() {
    return static_cast<Wad *>(fuse_get_context()->private_data);
}

//Open may not be needed (?)

static int getattr_callback(const char *path, struct stat *stbuf) {
    memset(stbuf, 0, sizeof(struct stat));
    cout << "--> GETARR ACTIVATED" << endl;
    Wad *wad = get_wad();

    cout << "wad->isDirectory(path): " << wad->isDirectory(path) << endl;
    cout << "wad->isContent(path): " << wad->isContent(path) << endl;
    if (wad->isDirectory(path)) {
        stbuf->st_mode = S_IFDIR | 0777;
        stbuf->st_nlink = 2;
        stbuf->st_uid = fuse_get_context()->uid;
        stbuf->st_gid = fuse_get_context()->gid;

        return 0;
    }
    if (wad->isContent(path)) {
        stbuf->st_mode = S_IFREG | 0777;
        stbuf->st_nlink = 1;
        stbuf->st_size = wad->getSize(path);
        stbuf->st_uid = fuse_get_context()->uid;
        stbuf->st_gid = fuse_get_context()->gid;
        return 0;
    }

    cout << "Returing -1 for getattr_callback" << endl;
    return -ENOENT;
}

static int readdir_callback(const char *path_char, void *buf, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info *fi) {
    cout << "--> READ DIR ACTIVATED" << endl;

    Wad *wad = get_wad();
    string path(path_char);
    cout << "path: " << path << endl;
    //vector<string> parts = Wad::splitPath(path);
    //cout << "parts.size(): " << parts.size() << endl;
    string parentDir = Wad::getParentDirectory(path);
    cout << "parentDir: " << parentDir << endl;

    cout << "--> READ DIR ACTIVATED 2" << endl;
    if (!wad->isDirectory(path))
        return -1;

    cout << "--> READ DIR ACTIVATED 3" << endl;
    vector<string> files;
    if (wad->getDirectory(path, &files) < 0)
        return -1;

    filler(buf, ".",  NULL, 0);
    filler(buf, "..", NULL, 0);

    cout << "--> READ DIR ACTIVATED 4" << endl;

    for (const auto &name : files)
        filler(buf, name.c_str(), NULL, 0);

    return 0;
}

static int write_callback(const char *path, const char *buf, size_t size, off_t offset, struct fuse_file_info *fi) {
  cout << "WRITE ACTIAVTED" << endl;
  Wad* wad = get_wad();
    if (!wad->isContent(path)){
      cout << "WRITE not content" << endl;
        return -1;
        }

    int ret = wad->writeToFile(path, buf, size, offset);
    return (ret < 0) ? -1 : ret;

}

static int mknod_callback(const char *path_char, mode_t mode, dev_t dev) {
    Wad *wad = get_wad();
    string path(path_char);
    vector<string> parts = Wad::splitPath(path);
    string parentDir = Wad::getParentDirectory(path);
    int x = 0;

    if (wad->isDirectory(path) || wad->isContent(path)){
      cout << "mknod " << x << endl;
      x++;
        return -1;
        }

    if (parentDir.empty())
        parentDir = "/";

    if (wad->isME(parts.back())){
        cout << "mknod " << x << endl;
        x++;
        return -1;
        }

    wad->createFile(path);
    if (!wad->isContent(path)){
        cout << "mknod " << x << endl;
        x++;
        return -1;
        }

    return 0;
}
static int mkdir_callback(const char *raw, mode_t /*mode*/) {
    Wad *wad = get_wad();

    // normalize the path (strip trailing slash)
    std::string full(raw);
    if (!full.empty() && full.back() == '/')
        full.pop_back();

    auto parts = Wad::splitPath(full);
    if (parts.empty())
        return -EINVAL;

    std::string newName = parts.back();
    parts.pop_back();

    std::string parent = "/";
    if (!parts.empty()) {
        parent.clear();
        for (auto &seg : parts)
            parent += "/" + seg;
    }

    if (!wad->isDirectory(parent))
        return -ENOTDIR;

    if (!parts.empty() && wad->isME(parts.back()))
        return -EPERM;

    // enforce the two‐letter namespace rule
    if (newName.size() > 2)
        return -EPERM;

    // use the normalized path here!
    wad->createDirectory(full);

    if (!wad->isDirectory(full))
        return -EIO;

    return 0;
}



static int read_callback(const char *path, char *buf, size_t size, off_t offset,struct fuse_file_info *fi) {

    Wad* wad = get_wad();
    if (!wad->isContent(path))
        return -1;

    int file_size = wad->getSize(path);
    if (file_size < 0)
        return -1;
    if (offset >= file_size)
        return 0;

    int to_read = size;
    if (offset + (int)size > file_size)
        to_read = file_size - offset;

    int ret = wad->getContents(path, buf, to_read, offset);
    return (ret < 0) ? -1 : ret;
}

static int open_callback(const char *path, struct fuse_file_info *fi) {
    Wad* wad = get_wad();
    if (!wad->isContent(path)) return -ENOENT;
    return 0;
}

static int truncate_callback(const char *path, off_t size) {
    Wad* wad = get_wad();
    if (!wad->isContent(path))
        return -ENOENT;
    // we only support writing from offset=0, so if size==0 it's fine
    return 0;
}

static struct fuse_operations operations;

static void init_ops() {
    std::memset(&operations, 0, sizeof(operations));

    operations.getattr  = getattr_callback;
    operations.open     = open_callback;
    operations.read     = read_callback;
    operations.write    = write_callback;
    operations.mknod    = mknod_callback;
    operations.mkdir    = mkdir_callback;
    operations.readdir  = readdir_callback;
    operations.truncate = truncate_callback;

}

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

    init_ops();
    argv[argc - 2] = argv[argc - 1];
    argc--;

    //((Wad*)fuse_get_context()->private_Data)->getContents();
    return fuse_main(argc, argv, &operations, wad);
}
