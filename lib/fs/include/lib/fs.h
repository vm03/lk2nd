/*
 * Copyright (c) 2009-2015 Travis Geiselbrecht
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files
 * (the "Software"), to deal in the Software without restriction,
 * including without limitation the rights to use, copy, modify, merge,
 * publish, distribute, sublicense, and/or sell copies of the Software,
 * and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */
#pragma once

void fs_init(void);

struct file_stat {
    bool is_dir;
    off_t size;
};

typedef struct _filehandle filehandle;
//typedef void *filecookie;
//typedef void *fscookie;

status_t fs_mount(const char *path, const char *fs, const char *device) __NONNULL();
status_t fs_unmount(const char *path) __NONNULL();

/* file api */
status_t fs_create_file(const char *path, filehandle **handle, uint64_t len) __NONNULL();
status_t fs_open_file(const char *path, filehandle **handle) __NONNULL();
ssize_t fs_read_file(filehandle *handle, void *buf, off_t offset, size_t len) __NONNULL();
ssize_t fs_write_file(filehandle *handle, const void *buf, off_t offset, size_t len) __NONNULL();
status_t fs_close_file(filehandle *handle) __NONNULL();
status_t fs_stat_file(filehandle *handle, struct file_stat *) __NONNULL();
status_t fs_make_dir(const char *path) __NONNULL();

/* convenience routines */
ssize_t fs_load_file(const char *path, void *ptr, size_t maxlen) __NONNULL();

/* walk through a path string, removing duplicate path seperators, flattening . and .. references */
void fs_normalize_path(char *path) __NONNULL();

/* file system api */
typedef struct _fscookie fscookie;
typedef struct _filecookie filecookie;
struct bdev;
struct fs_api {
    status_t (*mount)(struct bdev *, fscookie **);
    status_t (*unmount)(fscookie *);
    status_t (*open)(fscookie *, const char *, filecookie **);
    status_t (*create)(fscookie *, const char *, filecookie **, uint64_t);
    status_t (*mkdir)(fscookie *, const char *);
    status_t (*stat)(filecookie *, struct file_stat *);
    ssize_t (*read)(filecookie *, void *, off_t, size_t);
    ssize_t (*write)(filecookie *, const void *, off_t, size_t);
    status_t (*close)(filecookie *);
};

status_t fs_register_type(const char *name, const struct fs_api *api);

