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

#include <kk_device.h>

#include <string.h>

#include "external/libfreenect2/include/libfreenect2.h"
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

#define DEVICE_WIDTH     512
#define DEVICE_HEIGHT    424
#define DEVICE_FPS       15

// Represents connection to a single device.
// All methods are invoked under devices_mutex_.
class DeviceFreenect2 : public Device {
 public:
  DeviceFreenect2(freenect2_device* device);
  virtual ~DeviceFreenect2();

  virtual DeviceInfo GetDeviceInfo() const;
  virtual ImageInfo GetVideoImageInfo() const;
  virtual ImageInfo GetDepthImageInfo() const;

  virtual bool GetAndClearVideoData(uint8_t* dst, int row_size);
  virtual bool GetAndClearDepthData(uint16_t* dst, int row_size);


  void Setup(bool video_enabled, bool depth_enabled);
  void Teardown();

  void HandleDepthData(void* depth_data);
  void HandleVideoData(void* rgb_data);

  const freenect2_device* device() { return device_; }

 private:
  mutable pthread_mutex_t mutex_ = PTHREAD_MUTEX_INITIALIZER;
  freenect2_device* device_;
  uint8_t* video_data_;
  uint16_t* depth_data_;
  int video_data_size_;
  int depth_data_size_;
  bool has_video_update_;
  bool has_depth_update_;
};

DeviceFreenect2::DeviceFreenect2(freenect2_device* device)
    : device_(device), has_video_update_(false), has_depth_update_(false) {
  CHECK(device_);
  video_data_size_ = DEVICE_WIDTH * DEVICE_HEIGHT * 3;
  depth_data_size_ = DEVICE_WIDTH * DEVICE_HEIGHT * 2;
  video_data_ = new uint8_t[video_data_size_];
  depth_data_ = new uint16_t[depth_data_size_];
}

DeviceFreenect2::~DeviceFreenect2() {
  if (device_)
    freenect2_close_device(device_);

  delete[] video_data_;
  delete[] depth_data_;
}

DeviceInfo DeviceFreenect2::GetDeviceInfo() const {
  return DeviceInfo(kDeviceVersion2);
}

ImageInfo DeviceFreenect2::GetVideoImageInfo() const {
  if (false) {
    return ImageInfo();
  }
  return ImageInfo(DEVICE_WIDTH, DEVICE_HEIGHT, kImageFormatVideoRgb,
		   DEVICE_FPS);
}

ImageInfo DeviceFreenect2::GetDepthImageInfo() const {
  if (false) {
    return ImageInfo();
  }
  return ImageInfo(DEVICE_WIDTH, DEVICE_HEIGHT, kImageFormatDepthMm,
		   DEVICE_FPS);
}

void DeviceFreenect2::Setup(bool video_enabled, bool depth_enabled) {
  Autolock l(mutex_);
  // TODO(igorc): Find modes that produce lower FPS and bandwidth.
  CHECK_FREENECT(freenect2_set_depth_mode(
      device_, freenect2_find_depth_mode(
	  FREENECT2_RESOLUTION_512x424, FREENECT2_DEPTH_MM)));
  CHECK_FREENECT(freenect2_set_video_mode(
      device_, freenect2_find_video_mode(
	  FREENECT2_RESOLUTION_512x424, FREENECT2_VIDEO_RGB)));

  if (video_enabled) {
    CHECK_FREENECT(freenect2_start_video(device_));
    fprintf(stderr, "Connected to Kinect2 video stream\n");
  }
  if (depth_enabled) {
    CHECK_FREENECT(freenect2_start_depth(device_));
    fprintf(stderr, "Connected to Kinect2 depth stream\n");
  }
}

void DeviceFreenect2::Teardown() {
  Autolock l(mutex_);
  freenect2_stop_depth(device_);
  freenect2_stop_video(device_);
}

void DeviceFreenect2::HandleVideoData(void* rgb_data) {
  Autolock l(mutex_);
  memcpy(video_data_, rgb_data, video_data_size_);
  has_video_update_ = true;
}

void DeviceFreenect2::HandleDepthData(void* depth_data) {
  Autolock l(mutex_);
  memcpy(depth_data_, depth_data, depth_data_size_);
  has_depth_update_ = true;
}

bool DeviceFreenect2::GetAndClearVideoData(uint8_t* dst, int row_size) {
  Autolock l(mutex_);
  if (!has_video_update_) return false;
  CopyImageData(dst, video_data_, row_size, DEVICE_WIDTH * 3,
		DEVICE_HEIGHT);
  has_video_update_ = false;
  return true;
}

bool DeviceFreenect2::GetAndClearDepthData(uint16_t* dst, int row_size) {
  Autolock l(mutex_);
  if (!has_depth_update_) return false;
  CopyImageData(dst, depth_data_, row_size, DEVICE_WIDTH * 2,
		DEVICE_HEIGHT);
  has_depth_update_ = false;
  return true;
}

}  // namespace kkonnect
