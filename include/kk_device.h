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

#include <stdint.h>

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

  explicit DeviceInfo(DeviceVersion version) : version(version) {}
};

// Contains information about the opened image stream.
enum ImageFormat {
  // 24-bit RGB values.
  kImageFormatVideoRgb = 0,
  // 16-bit depth values, in mm.
  kImageFormatDepthMm = 20,
};

struct ImageInfo {
  // Whether the stream is enabled.
  // If disabled, all other fields have undefined values.
  bool enabled;
  int width;
  int height;
  ImageFormat format;
  int refresh_fps;

  ImageInfo()
      : enabled(false), width(0), height(0), format(kImageFormatVideoRgb),
	refresh_fps(0) {}

  ImageInfo(int width, int height, ImageFormat format, int refresh_fps)
      : enabled(true), width(width), height(height), format(format),
	refresh_fps(refresh_fps) {}
};

// Provides access to a given device's data.
class Device {
 public:
  virtual DeviceInfo GetDeviceInfo() const = 0;
  virtual ImageInfo GetVideoImageInfo() const = 0;
  virtual ImageInfo GetDepthImageInfo() const = 0;

  // Copies video data into |dst|. When non-zero, |row_size| defines
  // the length of row in the target array, in bytes. Returns true
  // if there was new data to copy. If there is no new data, returns false
  // and does not modify any memory referenced by |dst|.
  virtual bool GetAndClearVideoData(uint8_t* dst, int row_size) = 0;
  virtual bool GetAndClearDepthData(uint16_t* dst, int row_size) = 0;

 protected:
  Device() {}
  virtual ~Device() {}

  friend class Connection;

 private:
  Device(const Device& src);
  Device& operator=(const Device& src);
};

}  // namespace kkonnect

#endif  // KKONNECT_KK_DEVICE_H_
