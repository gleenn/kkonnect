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

#ifndef KKONNECT_KK_DEVICE_H_
#define KKONNECT_KK_DEVICE_H_

#include "kk_errors.h"

namespace kkonnect {

class Connection;

// Contains information about a given device.
enum DeviceVersion {
  kDeviceVersion1,  // XBox360
  kDeviceVersion2,  // K4W2 or XBoxOne
};

struct DeviceInfo {
  DeviceVersion version;
};

// Contains information about the opened image stream.
enum ImageFormat {
  // 24-bit RGB values.
  kImageFormatCameraRgb = 0,
  // 16-bit depth values, in mm.
  kImageFormatDepthMm = 20,
};

struct ImageInfo {
  bool enabled;
  int width;
  int height;
  ImageFormat format;
  int refresh_fps;
};

// Provides access to a given device's data.
class Device {
 public:
  const DeviceInfo& device_info() const { return info_; }
  const ImageInfo& camera_info() const { return camera_info_; }
  const ImageInfo& depth_info() const { return depth_info_; }

 protected:
  Device(const DeviceInfo& info, const ImageInfo& camera_info,
	 const ImageInfo& depth_info);
  virtual ~Device();

 private:
  friend class Connection;

  DeviceInfo info_;
  ImageInfo camera_info_;
  ImageInfo depth_info_;
};

}  // namespace kkonnect

#endif  // KKONNECT_KK_DEVICE_H_
