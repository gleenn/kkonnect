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

#include "external/libfreenect/include/libfreenect.h"
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

#define DEVICE_WIDTH     640
#define DEVICE_HEIGHT    480
#define DEVICE_FPS       15

// Represents connection to a single device.
// All methods are invoked under devices_mutex_.
class DeviceImpl : public Device {
 public:
  DeviceImpl(freenect_device* device);
  virtual ~DeviceImpl();

  virtual DeviceInfo GetDeviceInfo() const;
  virtual ImageInfo GetVideoImageInfo() const;
  virtual ImageInfo GetDepthImageInfo() const;

  virtual bool GetAndClearVideoData(uint8_t* dst, int row_size);
  virtual bool GetAndClearDepthData(uint16_t* dst, int row_size);


  void Setup(bool video_enabled, bool depth_enabled);
  void Teardown();

  void HandleDepthData(void* depth_data);
  void HandleVideoData(void* rgb_data);

  const freenect_device* device() { return device_; }

 private:
  mutable pthread_mutex_t mutex_ = PTHREAD_MUTEX_INITIALIZER;
  freenect_device* device_;
  uint8_t* video_data1_;
  uint8_t* video_data2_;
  uint16_t* depth_data1_;
  uint16_t* depth_data2_;
  uint8_t* last_video_data_;
  uint16_t* last_depth_data_;
  bool has_video_update_;
  bool has_depth_update_;
};

DeviceImpl::DeviceImpl(freenect_device* device)
    : device_(device), last_video_data_(NULL), last_depth_data_(NULL),
      has_video_update_(false), has_depth_update_(false) {
  CHECK(device_);
  int video_data_size = DEVICE_WIDTH * DEVICE_HEIGHT * 3;
  int depth_data_size = DEVICE_WIDTH * DEVICE_HEIGHT * 2;
  video_data1_ = new uint8_t[video_data_size];
  video_data2_ = new uint8_t[video_data_size];
  depth_data1_ = new uint16_t[depth_data_size];
  depth_data2_ = new uint16_t[depth_data_size];
}

DeviceImpl::~DeviceImpl() {
  if (device_)
    freenect_close_device(device_);

  delete[] video_data1_;
  delete[] video_data2_;
  delete[] depth_data1_;
  delete[] depth_data2_;
}

DeviceInfo DeviceImpl::GetDeviceInfo() const {
  return DeviceInfo(kDeviceVersion1);
}

ImageInfo DeviceImpl::GetVideoImageInfo() const {
  if (false) {
    return ImageInfo();
  }
  return ImageInfo(DEVICE_WIDTH, DEVICE_HEIGHT, kImageFormatVideoRgb,
		   DEVICE_FPS);
}

ImageInfo DeviceImpl::GetDepthImageInfo() const {
  if (false) {
    return ImageInfo();
  }
  return ImageInfo(DEVICE_WIDTH, DEVICE_HEIGHT, kImageFormatDepthMm,
		   DEVICE_FPS);
}

void DeviceImpl::Setup(bool video_enabled, bool depth_enabled) {
  Autolock l(mutex_);
  CHECK_FREENECT(freenect_set_led(device_, LED_RED));
  freenect_update_tilt_state(device_);
  freenect_get_tilt_state(device_);
  // TODO(igorc): Try switching to compressed UYVY and depth streams.
  CHECK_FREENECT(freenect_set_depth_mode(
      device_, freenect_find_depth_mode(
	  FREENECT_RESOLUTION_MEDIUM, FREENECT_DEPTH_MM)));
  // Use YUV_RGB as it forces 15Hz refresh rate and takes 2 bytes per pixel.
  CHECK_FREENECT(freenect_set_video_mode(
      device_, freenect_find_video_mode(
	  FREENECT_RESOLUTION_MEDIUM, FREENECT_VIDEO_YUV_RGB)));
  CHECK_FREENECT(freenect_set_depth_buffer(device_, depth_data1_));
  CHECK_FREENECT(freenect_set_video_buffer(device_, video_data1_));

  if (video_enabled) {
    CHECK_FREENECT(freenect_start_video(device_));
    fprintf(stderr, "Connected to Kinect video stream\n");
  }
  if (depth_enabled) {
    CHECK_FREENECT(freenect_start_depth(device_));
    fprintf(stderr, "Connected to Kinect depth stream\n");
  }
}

void DeviceImpl::Teardown() {
  Autolock l(mutex_);
  freenect_stop_depth(device_);
  freenect_stop_video(device_);
}

void DeviceImpl::HandleVideoData(void* rgb_data) {
  Autolock l(mutex_);
  last_video_data_ = reinterpret_cast<uint8_t*>(rgb_data);
  CHECK_FREENECT(freenect_set_video_buffer(
      device_,
      last_video_data_ == video_data1_ ? video_data2_ : video_data2_));
  has_video_update_ = true;
}

void DeviceImpl::HandleDepthData(void* depth_data) {
  Autolock l(mutex_);
  last_depth_data_ = reinterpret_cast<uint16_t*>(depth_data);
  CHECK_FREENECT(freenect_set_depth_buffer(
      device_,
      last_depth_data_ == depth_data1_ ? depth_data2_ : depth_data2_));
  has_depth_update_ = true;
}

bool DeviceImpl::GetAndClearVideoData(uint8_t* dst, int row_size) {
  Autolock l(mutex_);
  if (!has_video_update_) return false;
  CopyImageData(dst, last_video_data_, row_size, DEVICE_WIDTH * 3,
		DEVICE_HEIGHT);
  has_video_update_ = false;
  return true;
}

bool DeviceImpl::GetAndClearDepthData(uint16_t* dst, int row_size) {
  Autolock l(mutex_);
  if (!has_depth_update_) return false;
  CopyImageData(dst, last_depth_data_, row_size, DEVICE_WIDTH * 2,
		DEVICE_HEIGHT);
  has_depth_update_ = false;
  return true;
}

}  // namespace kkonnect
