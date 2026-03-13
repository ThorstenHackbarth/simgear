// SPDX-License-Identifier: LGPL-2.1-or-later
// SPDX-FileCopyrightText: 2026 James Turner <james@flightgear.org>

#include "unzip_buffer.hxx"

// zlib.h has to precede ioapi.h
// clang-format off
#include "zlib.h"
#include "ioapi.h"
// clang-format on

#include <cassert>
#include <cstddef>
#include <cstring>

namespace simgear {

static voidpf ZCALLBACK fopen_simgear_buffer_func(voidpf opaque, const char* /*filename*/, int /*mode*/)
{
    // The buffer is already set up in opaque; return it as the stream handle.
    return opaque;
}

static uLong ZCALLBACK fread_simgear_buffer_func(voidpf opaque, voidpf /*stream*/, void* buf, uLong size)
{
    UnzipOpaqueData* d = static_cast<UnzipOpaqueData*>(opaque);

    assert(d->pos <= d->buffer.size());
    const std::size_t remaining = d->buffer.size() - d->pos;
    if (size > remaining) {
        size = static_cast<uLong>(remaining);
    }

    memcpy(buf, d->buffer.data() + d->pos, size);
    d->pos += size;
    return size;
}

static uLong ZCALLBACK fwrite_simgear_buffer_func(voidpf /*opaque*/, voidpf /*stream*/, const void* /*buf*/, uLong /*size*/)
{
    // Writing is not supported for this read-only buffer implementation.
    return 0;
}

static long ZCALLBACK ftell_simgear_buffer_func(voidpf opaque, voidpf /*stream*/)
{
    const UnzipOpaqueData* d = static_cast<const UnzipOpaqueData*>(opaque);
    return static_cast<long>(d->pos);
}

static long ZCALLBACK fseek_simgear_buffer_func(voidpf opaque, voidpf /*stream*/, uLong offset, int origin)
{
    UnzipOpaqueData* d = static_cast<UnzipOpaqueData*>(opaque);
    assert(d->pos <= d->buffer.size());

    switch (origin) {
    case ZLIB_FILEFUNC_SEEK_CUR:
        if (offset <= d->buffer.size() - d->pos) {
            d->pos += offset;
        } else {
            return -1l; // seek beyond end of buffer
        }
        break;
    case ZLIB_FILEFUNC_SEEK_END:
        assert(offset == uLong(0)); // this is the only valid value
        d->pos = d->buffer.size();
        break;
    case ZLIB_FILEFUNC_SEEK_SET:
        d->pos = static_cast<std::size_t>(offset);
        break;
    default:
        return -1l;
    }

    return 0l;
}

static int ZCALLBACK fclose_simgear_buffer_func(voidpf /*opaque*/, voidpf /*stream*/)
{
    // Buffer is owned by the caller; nothing to free here.
    return 0;
}

static int ZCALLBACK ferror_simgear_buffer_func(voidpf /*opaque*/, voidpf /*stream*/)
{
    return 0; // no errors
}

void fillUnzipBufferFuncs(UnzipOpaqueData* d, zlib_filefunc_def* pzlib_filefunc_def)
{
    pzlib_filefunc_def->zopen_file = fopen_simgear_buffer_func;
    pzlib_filefunc_def->zread_file = fread_simgear_buffer_func;
    pzlib_filefunc_def->zwrite_file = fwrite_simgear_buffer_func;
    pzlib_filefunc_def->ztell_file = ftell_simgear_buffer_func;
    pzlib_filefunc_def->zseek_file = fseek_simgear_buffer_func;
    pzlib_filefunc_def->zclose_file = fclose_simgear_buffer_func;
    pzlib_filefunc_def->zerror_file = ferror_simgear_buffer_func;
    pzlib_filefunc_def->opaque = static_cast<voidpf>(d);
}

} // namespace simgear
