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

#ifndef KKONNECT_KK_FREENECT2_DEVICE_H_
#define KKONNECT_KK_FREENECT2_DEVICE_H_

#include <string>  // Because libfreenect2.hpp forgot to include it.

#include <libfreenect2/libfreenect2.hpp>
#include <pthread.h>

#include "src/kk_freenect_base.h"

namespace kkonnect {

// Implements Device for a libfreenect2 device.
class Freenect2Device : public BaseFreenectDevice {
 public:
  Freenect2Device(
      libfreenect2::Freenect2* context, const DeviceOpenRequest& request);
  virtual ~Freenect2Device();

  virtual void Connect();
  virtual void Stop();

  void HandleDepthData(void* depth_data);
  void HandleVideoData(void* video_data);

  const libfreenect2::Freenect2Device* device() { return device_; }

 private:
  libfreenect2::Freenect2* context_;
  libfreenect2::FrameListener* callback_;
  DeviceOpenRequest open_request_;
  libfreenect2::Freenect2Device* device_;
  uint8_t* video_data_;
  uint16_t* depth_data_;
};

}  // namespace kkonnect

#endif  // KKONNECT_KK_FREENECT2_DEVICE_H_
