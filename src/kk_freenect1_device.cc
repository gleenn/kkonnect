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

#include "src/kk_freenect1_device.h"
#include "src/utils.h"

namespace kkonnect {

#define MAX_DEVICE_OPEN_ATTEMPTS   10

#define DEVICE_WIDTH     640
#define DEVICE_HEIGHT    480
#define DEVICE_FPS       15

Freenect1Device::Freenect1Device()
    : BaseFreenectDevice(kDeviceVersion1), device_(NULL), video_data1_(NULL),
      video_data2_(NULL), depth_data1_(NULL), depth_data2_(NULL) {}

Freenect1Device::~Freenect1Device() {
  if (device_)
    freenect_close_device(device_);

  delete[] video_data1_;
  delete[] video_data2_;
  delete[] depth_data1_;
  delete[] depth_data2_;
}

void Freenect1Device::Connect(freenect_context* context,
                              const DeviceOpenRequest& request) {
  CHECK(!device_);
  int device_index = request.device_index;
  fprintf(stderr, "Connecting to Kinect1 #%d\n", device_index);

  int openAttempt = 1;
  freenect_device* device_raw = NULL;
  while (true) {
    int res = freenect_open_device(context, &device_raw, device_index);
    if (!res) break;
    if (openAttempt >= MAX_DEVICE_OPEN_ATTEMPTS) {
      // TODO(igorc): Log and return an error.
      Autolock l(mutex_);
      SetStatusLocked(kErrorUnableToConnect);
      CHECK_FREENECT(res);
      return;
    }
    Sleep(0.5);
    fprintf(
        stderr, "Retrying freenect_open_device on #%d after error %d\n",
        device_index, res);
    ++openAttempt;
  }

  Autolock l(mutex_);
  device_ = device_raw;
  CHECK_FREENECT(freenect_set_led(device_, LED_RED));
  freenect_update_tilt_state(device_);
  freenect_get_tilt_state(device_);

  if (request.video_format == kImageFormatVideoRgb) {
    // TODO(igorc): Try switching to compressed UYVY and depth streams.
    // Use YUV_RGB as it forces 15Hz refresh rate and takes 2 bytes per pixel.
    CHECK_FREENECT(freenect_set_video_mode(
	device_, freenect_find_video_mode(
	FREENECT_RESOLUTION_MEDIUM, FREENECT_VIDEO_YUV_RGB)));
    CHECK_FREENECT(freenect_set_video_buffer(device_, video_data1_));
    SetVideoParamsLocked(DEVICE_WIDTH, DEVICE_HEIGHT, DEVICE_FPS);
    video_data1_ = new uint8_t[GetVideoBufferSizeLocked()];
    video_data2_ = new uint8_t[GetVideoBufferSizeLocked()];
  }

  if (request.depth_format == kImageFormatDepthMm) {
    CHECK_FREENECT(freenect_set_depth_mode(
	device_, freenect_find_depth_mode(
	FREENECT_RESOLUTION_MEDIUM, FREENECT_DEPTH_MM)));
    CHECK_FREENECT(freenect_set_depth_buffer(device_, depth_data1_));
    SetDepthParamsLocked(DEVICE_WIDTH, DEVICE_HEIGHT, DEVICE_FPS);
    depth_data1_ = new uint16_t[GetDepthBufferSizeLocked()];
    depth_data2_ = new uint16_t[GetDepthBufferSizeLocked()];
  }

  SetStatusLocked(kErrorSuccess);
}

void Freenect1Device::Start() {
  Autolock l(mutex_);
  if (IsVideoEnabledLocked()) {
    CHECK_FREENECT(freenect_start_video(device_));
    fprintf(stderr, "Connected to Kinect1 video stream\n");
  }
  if (IsDepthEnabledLocked()) {
    CHECK_FREENECT(freenect_start_depth(device_));
    fprintf(stderr, "Connected to Kinect1 depth stream\n");
  }
}

void Freenect1Device::Stop() {
  Autolock l(mutex_);
  if (IsVideoEnabledLocked()) freenect_stop_video(device_);
  if (IsDepthEnabledLocked()) freenect_stop_depth(device_);
}

void Freenect1Device::HandleVideoData(void* video_data) {
  Autolock l(mutex_);
  CHECK_FREENECT(freenect_set_video_buffer(
      device_,
      video_data == video_data1_ ? video_data2_ : video_data2_));
  SetVideoDataLocked(video_data);
}

void Freenect1Device::HandleDepthData(void* depth_data) {
  Autolock l(mutex_);
  CHECK_FREENECT(freenect_set_depth_buffer(
      device_,
      depth_data == depth_data1_ ? depth_data2_ : depth_data2_));
  SetDepthDataLocked(depth_data);
}

}  // namespace kkonnect
