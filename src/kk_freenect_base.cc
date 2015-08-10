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

#include "src/kk_freenect_base.h"

namespace kkonnect {

BaseFreenectDevice::BaseFreenectDevice(DeviceVersion version)
  : version_(version), status_(kErrorInProgress),
    last_video_data_(NULL), last_depth_data_(NULL),
    has_video_update_(false), has_depth_update_(false),
    video_width_(0), video_height_(0), video_fps_(0),
    depth_width_(0), depth_height_(0), depth_fps_(0) {
  pthread_mutex_init(&mutex_, NULL);
}

BaseFreenectDevice::~BaseFreenectDevice() {}

DeviceInfo BaseFreenectDevice::GetDeviceInfo() const {
  return DeviceInfo(version_);
}

ImageInfo BaseFreenectDevice::GetVideoImageInfo() const {
  Autolock l(mutex_);
  if (!video_width_) {
    return ImageInfo();
  }
  return ImageInfo(video_width_, video_height_, kImageFormatVideoRgb,
		   video_fps_);
}

ImageInfo BaseFreenectDevice::GetDepthImageInfo() const {
  Autolock l(mutex_);
  if (!depth_width_) {
    return ImageInfo();
  }
  return ImageInfo(depth_width_, depth_height_, kImageFormatDepthMm,
		   depth_fps_);
}

ErrorCode BaseFreenectDevice::GetStatus() const {
  Autolock l(mutex_);
  return status_;
}

void BaseFreenectDevice::SetStatusLocked(ErrorCode status) {
  status_ = status;
}

void BaseFreenectDevice::SetVideoParamsLocked(int width, int height, int fps) {
  video_width_ = width;
  video_height_ = height;
  video_fps_ = fps;
}

void BaseFreenectDevice::SetDepthParamsLocked(int width, int height, int fps) {
  depth_width_ = width;
  depth_height_ = height;
  depth_fps_ = fps;
}

int BaseFreenectDevice::GetVideoBufferSizeLocked() const {
  return video_width_ * video_height_ * 3;
}

int BaseFreenectDevice::GetDepthBufferSizeLocked() const {
  return depth_width_ * depth_height_ * 2;
}

void BaseFreenectDevice::SetVideoDataLocked(void* video_data) {
  last_video_data_ = reinterpret_cast<uint8_t*>(video_data);
  has_video_update_ = true;
}

void BaseFreenectDevice::SetDepthDataLocked(void* depth_data) {
  last_depth_data_ = reinterpret_cast<uint16_t*>(depth_data);
  has_depth_update_ = true;
}

bool BaseFreenectDevice::GetAndClearVideoData(uint8_t* dst, int row_size) {
  Autolock l(mutex_);
  if (!has_video_update_ || !video_width_) return false;
  CopyImageData(dst, last_video_data_, row_size, video_width_ * 3,
		video_height_);
  has_video_update_ = false;
  return true;
}

bool BaseFreenectDevice::GetAndClearDepthData(uint16_t* dst, int row_size) {
  Autolock l(mutex_);
  if (!has_depth_update_ || !depth_width_) return false;
  CopyImageData(dst, last_depth_data_, row_size, depth_width_ * 2,
		depth_height_);
  has_depth_update_ = false;
  return true;
}

}  // namespace kkonnect
