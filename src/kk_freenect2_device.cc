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

class FrameListenerImpl : public libfreenect2::FrameListener {
 public:
  FrameListenerImpl(Freenect2Device* device) : device_(device) {}
  virtual ~FrameListenerImpl() {}

  virtual bool onNewFrame(
      libfreenect2::Frame::Type type, libfreenect2::Frame* frame);

 private:
  Freenect2Device* device_;
};

Freenect2Device::Freenect2Device(
    libfreenect2::Freenect2* context, const DeviceOpenRequest& request)
    : BaseFreenectDevice(kDeviceVersion2), context_(context),
      open_request_(request), device_(NULL), video_data_(NULL),
      depth_data_(NULL) {
  callback_ = new FrameListenerImpl(this);
}

Freenect2Device::~Freenect2Device() {
  if (device_) {
    device_->close();
    delete device_;
  }

  delete[] video_data_;
  delete[] depth_data_;
  delete callback_;
}

void Freenect2Device::Connect() {
  CHECK(!device_);
  int device_index = open_request_.device_index;
  fprintf(stderr, "Connecting to Kinect1 #%d\n", device_index);

  int openAttempt = 1;
  libfreenect2::Freenect2Device* device_raw = NULL;
  while (true) {
    device_raw = context_->openDevice(device_index);
    if (device_raw) break;
    fprintf(
        stderr, "Failed freenect2_open_device on #%d, attempt=%d\n",
        device_index, openAttempt);
    if (openAttempt >= MAX_DEVICE_OPEN_ATTEMPTS) {
      Autolock l(mutex_);
      SetStatusLocked(kErrorUnableToConnect);
      return;
    }
    Sleep(0.5);
    ++openAttempt;
  }

  Autolock l(mutex_);
  device_ = device_raw;

  if (open_request_.video_format == kImageFormatVideoRgb) {
    // TODO(igorc): Find modes that produce lower FPS and bandwidth.
    // CHECK_FREENECT(freenect2_set_video_mode(
    //    device_, freenect2_find_video_mode(
    //    FREENECT2_RESOLUTION_512x424, FREENECT2_VIDEO_RGB)));
    SetVideoParamsLocked(DEVICE_WIDTH, DEVICE_HEIGHT, DEVICE_FPS);
    video_data_ = new uint8_t[GetVideoBufferSizeLocked()];
    device_->setColorFrameListener(callback_);
  }

  if (open_request_.depth_format == kImageFormatDepthMm) {
    // CHECK_FREENECT(freenect2_set_depth_mode(
    //    device_, freenect2_find_depth_mode(
    //    FREENECT2_RESOLUTION_512x424, FREENECT2_DEPTH_MM)));
    SetDepthParamsLocked(DEVICE_WIDTH, DEVICE_HEIGHT, DEVICE_FPS);
    depth_data_ = new uint16_t[GetDepthBufferSizeLocked()];
    device_->setIrAndDepthFrameListener(callback_);
  }

  device_->start();
  fprintf(stderr, "Connected to Kinect2 streams\n");

  SetStatusLocked(kErrorSuccess);
}

void Freenect2Device::Stop() {
  Autolock l(mutex_);
  if (device_) device_->stop();
}

void Freenect2Device::HandleVideoData(void* video_data) {
  Autolock l(mutex_);
  memcpy(video_data_, video_data, GetVideoBufferSizeLocked());
  SetVideoDataLocked(depth_data_);
}

void Freenect2Device::HandleDepthData(void* depth_data) {
  Autolock l(mutex_);
  uint32_t* depth_data2 = reinterpret_cast<uint32_t*>(depth_data);
  for (int i = 0; i < DEVICE_WIDTH * DEVICE_HEIGHT; ++i) {
    depth_data_[i] = depth_data2[i];
  }
  SetDepthDataLocked(depth_data_);
}

bool FrameListenerImpl::onNewFrame(
    libfreenect2::Frame::Type type, libfreenect2::Frame* frame) {
  if (type == libfreenect2::Frame::Color) {
    CHECK(frame->width == DEVICE_WIDTH);
    CHECK(frame->height == DEVICE_HEIGHT);
    CHECK(frame->bytes_per_pixel == 3);
    device_->HandleVideoData(frame->data);
    return true;
  }

  if (type == libfreenect2::Frame::Depth) {
    // fprintf(stderr, "Params = %d %d %d\n",
    //     (int)frame->width, (int)frame->height,
    //     (int)frame->bytes_per_pixel);
    CHECK(frame->width == DEVICE_WIDTH);
    CHECK(frame->height == DEVICE_HEIGHT);
    CHECK(frame->bytes_per_pixel == 4);
    device_->HandleDepthData(frame->data);
    return true;
  }

  return true;
}

}  // namespace kkonnect
