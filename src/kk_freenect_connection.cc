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

#include <kk_connection.h>

#include "src/kk_freenect_connection.h"
#include "src/utils.h"

namespace kkonnect {

////////////////////////////////////////////////////////////////////////////////
// COMMON METHODS
////////////////////////////////////////////////////////////////////////////////

FreenectConnection* FreenectConnection::instance_ = NULL;

// static
FreenectConnection* FreenectConnection::GetInstanceImpl() {
  if (!instance_) instance_ = new FreenectConnection();
  return instance_;
}

FreenectConnection::FreenectConnection()
    : ref_count_(1), should_exit_(false),
      freenect1_context_(NULL), freenect1_device_count_(0) {
}

FreenectConnection::~FreenectConnection() {
  instance_ = NULL;
}

void FreenectConnection::DecRefLocked() {
  --ref_count_;
  if (!ref_count_) delete this;
}

void FreenectConnection::CloseInternalLocked() {
  should_exit_ = true;
  if (freenect1_context_) {
    // TODO(igorc): Signal threads.
    freenect_shutdown(freenect1_context_);
  }
  // TODO(igorc): De-init, but do not destroy. Also wait for threads to exit.
  DecRefLocked();
}

int FreenectConnection::GetDeviceCount() {
  Autolock l(mutex_);
  return freenect1_device_count_;
}

bool FreenectConnection::GetVersionLocked(
    int device_index, DeviceVersion* version) const {
  if (device_index < 0 || device_index >= freenect1_device_count_) {
    return false;
  }
  *version = kDeviceVersion1;
  return true;
}

ErrorCode FreenectConnection::GetDeviceInfo(
    int device_index, DeviceInfo* info) {
  Autolock l(mutex_);
  DeviceVersion version;
  if (!GetVersionLocked(device_index, &version)) {
    return kErrorUnknownDevice;
  }
  *info = DeviceInfo(version);
  return kErrorSuccess;
}

ErrorCode FreenectConnection::Refresh() {
  Autolock l(mutex_);

  if (!freenect1_context_) {
    CHECK_FREENECT(freenect_init(&freenect1_context_, NULL));
    freenect_set_log_level(freenect1_context_, FREENECT_LOG_DEBUG);
    freenect_select_subdevices(
	freenect1_context_, (freenect_device_flags) FREENECT_DEVICE_CAMERA);
    CHECK(!pthread_create(&freenect1_thread_, NULL, RunFreenect1Loop, this));
    CHECK(!pthread_create(&connection_thread_, NULL, RunConnectLoop, this));
    ref_count_ += 2;
  }

  freenect1_device_count_ = freenect_num_devices(freenect1_context_);
  fprintf(stderr, "Found %d Kinect1 devices\n", freenect1_device_count_);

  return kErrorSuccess;
}

ErrorCode FreenectConnection::OpenDeviceInternalLocked(
    const DeviceOpenRequest& request, Device** device) {
  DeviceVersion version;
  if (!GetVersionLocked(request.device_index, &version)) {
    return kErrorUnknownDevice;
  }

  BaseFreenectDevice* base_device;
  if (version == kDeviceVersion1) {
    base_device = new Freenect1Device(
        freenect1_context_, OnFreenect1VideoCallback, OnFreenect1DepthCallback);
  } else {
    // TODO(igorc): Implement.
    return kErrorInvalidArgument;
  }
  *device = base_device;
  return kErrorSuccess;
}

void FreenectConnection::CloseDeviceInternalLocked(Device* device) {
  BaseFreenectDevice* base_device =
      reinterpret_cast<BaseFreenectDevice*>(device);
  // TODO(igorc): Stop and destroy after clearing connect queue.
  base_device->Stop();
  delete base_device;
}

// static
void* FreenectConnection::RunConnectLoop(void* arg) {
  reinterpret_cast<FreenectConnection*>(arg)->RunConnectLoop();
  return NULL;
}

void FreenectConnection::RunConnectLoop() {
  while (!should_exit_) {
    // TODO(igorc): Implement.
  }
}

////////////////////////////////////////////////////////////////////////////////
// FREENECT1 METHODS
////////////////////////////////////////////////////////////////////////////////

// static
void* FreenectConnection::RunFreenect1Loop(void* arg) {
  reinterpret_cast<FreenectConnection*>(arg)->RunFreenect1Loop();
  return NULL;
}

void FreenectConnection::RunFreenect1Loop() {
  while (!should_exit_) {
    CHECK_FREENECT(freenect_process_events(freenect1_context_));
  }
}

// static
void FreenectConnection::OnFreenect1DepthCallback(
    freenect_device* dev, void* depth_data, uint32_t timestamp) {
  (void) timestamp;
  GetInstanceImpl()->HandleFreenect1DepthData(dev, depth_data);
}

// static
void FreenectConnection::OnFreenect1VideoCallback(
    freenect_device* dev, void* video_data, uint32_t timestamp) {
  (void) timestamp;
  GetInstanceImpl()->HandleFreenect1VideoData(dev, video_data);
}

Freenect1Device* FreenectConnection::FindFreenect1Locked(
    freenect_device* dev) const {
  Device* device = GetFirstDeviceLocked();
  while (device) {
    if (device->GetDeviceInfo().version != kDeviceVersion1) continue;
    Freenect1Device* device2 = reinterpret_cast<Freenect1Device*>(device);
    if (device2->device() == dev) {
      return device2;
    }
    device = GetNextDeviceLocked(device);
  }
  return NULL;
}

void FreenectConnection::HandleFreenect1DepthData(
    freenect_device* dev, void* depth_data) {
  Autolock l(mutex_);
  Freenect1Device* device = FindFreenect1Locked(dev);
  if (!device) return;  // Closed.
  device->HandleDepthData(depth_data);
}

void FreenectConnection::HandleFreenect1VideoData(
    freenect_device* dev, void* video_data) {
  Autolock l(mutex_);
  Freenect1Device* device = FindFreenect1Locked(dev);
  if (!device) return;  // Closed.
  device->HandleVideoData(video_data);
}

}  // namespace kkonnect
