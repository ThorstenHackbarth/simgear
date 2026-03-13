// SPDX-License-Identifier: LGPL-2.1-or-later
// SPDX-FileCopyrightText: 2026 James Turner <james@flightgear.org>

#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>

struct zlib_filefunc_def_s;
typedef struct zlib_filefunc_def_s zlib_filefunc_def;

namespace simgear {

/**
 * Opaque state used by the minizip IO callbacks for in-memory unzip access.
 * Populate `buffer` with the zip archive bytes before calling
 * fillUnzipBufferFuncs().  `pos` is managed internally by the callbacks
 * and should be left at its default value of 0.
 */
struct UnzipOpaqueData {
    std::vector<uint8_t> buffer;
    std::size_t pos = 0;
};

/**
 * Populate @a pzlib_filefunc_def with read-only minizip IO callbacks backed
 * by @a d.  The caller retains ownership of @a d and must ensure it remains
 * valid for the lifetime of the unzFile handle created with the returned
 * function table.
 */
void fillUnzipBufferFuncs(UnzipOpaqueData* d, zlib_filefunc_def* pzlib_filefunc_def);

} // namespace simgear
