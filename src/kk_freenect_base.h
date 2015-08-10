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

#ifndef KKONNECT_KK_FREENECT_BASE_H_
#define KKONNECT_KK_FREENECT_BASE_H_

#include <kk_device.h>
#include <pthread.h>

#include "src/utils.h"

namespace kkonnect {

#define CHECK_FREENECT(call)                                            \
    { int res__ = (call);                                               \
      if (res__) {                                                      \
        fprintf(stderr, "EXITING with check-fail at %s (%s:%d): %d"     \
                ". Condition = '" TOSTRING(call) "'\n",                 \
                __FILE__, __FUNCTION__, __LINE__, res__);               \
        exit(-1);                                                       \
      } }

// Implements Device for a libfreenect device.
class BaseFreenectDevice : public Device {
 public:
  BaseFreenectDevice(DeviceVersion version);
  virtual ~BaseFreenectDevice();

  virtual DeviceInfo GetDeviceInfo() const;
  virtual ImageInfo GetVideoImageInfo() const;
  virtual ImageInfo GetDepthImageInfo() const;

  virtual ErrorCode GetStatus() const;

  virtual bool GetAndClearVideoData(uint8_t* dst, int row_size);
  virtual bool GetAndClearDepthData(uint16_t* dst, int row_size);

  // Connects and starts the device stream.
  virtual void Connect(const DeviceOpenRequest& request) = 0;

  // Stops the device before calling the destructor.
  virtual void Stop() = 0;

 protected:
  void SetStatusLocked(ErrorCode status);

  void SetVideoParamsLocked(int width, int height, int fps);
  void SetDepthParamsLocked(int width, int height, int fps);
  int IsVideoEnabledLocked() const { return video_width_ != 0; }
  int IsDepthEnabledLocked() const { return depth_width_ != 0; }
  int GetVideoBufferSizeLocked() const;
  int GetDepthBufferSizeLocked() const;

  void SetDepthDataLocked(void* depth_data);
  void SetVideoDataLocked(void* video_data);

  mutable pthread_mutex_t mutex_;

 private:
  DeviceVersion version_;
  ErrorCode status_;
  uint8_t* last_video_data_;
  uint16_t* last_depth_data_;
  bool has_video_update_;
  bool has_depth_update_;
  int video_width_;
  int video_height_;
  int video_fps_;
  int depth_width_;
  int depth_height_;
  int depth_fps_;
};

}  // namespace kkonnect

#endif  // KKONNECT_KK_FREENECT_BASE_H_
