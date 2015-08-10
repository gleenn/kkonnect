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

#include "src/kk_freenect2_device.h"
#include "src/utils.h"

namespace kkonnect {

#define MAX_DEVICE_OPEN_ATTEMPTS   10

#define DEVICE_WIDTH     512
#define DEVICE_HEIGHT    424
#define DEVICE_FPS       15

Freenect2Device::Freenect2Device()
    : BaseFreenectDevice(kDeviceVersion1), device_(NULL),
      video_data_(NULL), depth_data_(NULL) {}

Freenect2Device::~Freenect2Device() {
  if (device_)
    freenect2_close_device(device_);

  delete[] video_data_;
  delete[] depth_data_;
}

void Freenect2Device::Connect(freenect2_context* context,
                              const DeviceOpenRequest& request) {
  CHECK(!device_);
  int device_index = request.device_index;
  fprintf(stderr, "Connecting to Kinect1 #%d\n", device_index);

  int openAttempt = 1;
  freenect2_device* device_raw = NULL;
  while (true) {
    int res = freenect2_open_device(context, &device_raw, device_index);
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

  if (request.video_format == kImageFormatVideoRgb) {
    // TODO(igorc): Find modes that produce lower FPS and bandwidth.
    CHECK_FREENECT(freenect2_set_video_mode(
	device_, freenect2_find_video_mode(
	FREENECT2_RESOLUTION_512x424, FREENECT2_VIDEO_RGB)));
    SetVideoParamsLocked(DEVICE_WIDTH, DEVICE_HEIGHT, DEVICE_FPS);
    video_data_ = new uint8_t[GetVideoBufferSizeLocked()];
  }

  if (request.depth_format == kImageFormatDepthMm) {
    CHECK_FREENECT(freenect2_set_depth_mode(
	device_, freenect2_find_depth_mode(
	FREENECT2_RESOLUTION_512x424, FREENECT2_DEPTH_MM)));
    SetDepthParamsLocked(DEVICE_WIDTH, DEVICE_HEIGHT, DEVICE_FPS);
    depth_data_ = new uint16_t[GetDepthBufferSizeLocked()];
  }
}

void Freenect2Device::Start() {
  Autolock l(mutex_);
  if (IsVideoEnabledLocked()) {
    CHECK_FREENECT(freenect2_start_video(device_));
    fprintf(stderr, "Connected to Kinect2 video stream\n");
  }
  if (IsDepthEnabledLocked()) {
    CHECK_FREENECT(freenect2_start_depth(device_));
    fprintf(stderr, "Connected to Kinect2 depth stream\n");
  }
}

void Freenect2Device::Stop() {
  Autolock l(mutex_);
  if (IsVideoEnabledLocked()) freenect2_stop_video(device_);
  if (IsDepthEnabledLocked()) freenect2_stop_depth(device_);
}

void Freenect2Device::HandleVideoData(void* video_data) {
  Autolock l(mutex_);
  memcpy(video_data_, video_data, GetVideoBufferSizeLocked());
  SetVideoDataLocked(depth_data_);
}

void Freenect2Device::HandleDepthData(void* depth_data) {
  Autolock l(mutex_);
  memcpy(depth_data_, depth_data, GetDepthBufferSizeLocked());
  SetDepthDataLocked(depth_data_);
}

}  // namespace kkonnect
