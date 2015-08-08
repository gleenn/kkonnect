/*
 * This file is part of the KKonnect Project.
 *
 * Copyright (c) 2015 individual KKonnect contributors. See the CONTRIB file
 * for details.
 *
 * This code is licensed to you under the terms of the Apache License, version
 * 2.0, or, at your option, the terms of the GNU General Public License,
 * version 2.0. See the APACHE20 and GPL2 files for the text of the licenses,
 * or the following URLs:
 * http://www.apache.org/licenses/LICENSE-2.0
 * http://www.gnu.org/licenses/gpl-2.0.txt
 *
 * If you redistribute this file in source form, modified or unmodified, you
 * may:
 *   1) Leave this header intact and distribute it under the same terms,
 *      accompanying it with the APACHE20 and GPL20 files, or
 *   2) Delete the Apache 2.0 clause and accompany it with the GPL2 file, or
 *   3) Delete the GPL v2 clause and accompany it with the APACHE20 file
 * In all cases you must keep the copyright notice intact and include a copy
 * of the CONTRIB file.
 *
 * Binary distributions must follow the binary distribution requirements of
 * either License.
 */

#include "utils.h"

#include <stdint.h>
#include <string.h>

namespace kkonnect {

void CopyImageData(void* dst, const void* src, int dst_row_size,
		   int src_row_size, int height) {
  CHECK(dst_row_size >= 0);
  CHECK(src_row_size > 0);
  CHECK(height >= 0);
  if (!dst_row_size || dst_row_size == src_row_size) {
    memcpy(dst, src, src_row_size * height);
    return;
  }

  CHECK(dst_row_size > src_row_size);
  for (int i = 0; i < height; ++i) {
    memcpy(dst, src, src_row_size);
    dst = reinterpret_cast<uint8_t*>(dst) + dst_row_size;
    src = reinterpret_cast<const uint8_t*>(src) + src_row_size;
  }
}

}  // namespace kkonnect
