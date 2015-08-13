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

#define LIBUSB_ERROR_INTERRUPTED  -10

pthread_mutex_t FreenectConnection::global_mutex_ = PTHREAD_MUTEX_INITIALIZER;
FreenectConnection* FreenectConnection::instance_ = NULL;

// static
FreenectConnection* FreenectConnection::GetInstanceImpl() {
  Autolock l(global_mutex_);
  if (!instance_) {
    instance_ = new FreenectConnection();
    instance_->Refresh();
  }
  return instance_;
}

FreenectConnection::FreenectConnection()
    : ref_count_(1), should_exit_(false),
      freenect1_context_(NULL), freenect1_device_count_(0),
      freenect2_context_(NULL), freenect2_device_count_(0) {
  pthread_cond_init(&connection_cond_, NULL);
}

FreenectConnection::~FreenectConnection() {
  pthread_cond_destroy(&connection_cond_);

  {
    Autolock l(global_mutex_);
    instance_ = NULL;
  }
}

void FreenectConnection::DecRefLocked() {
  --ref_count_;
  if (!ref_count_) delete this;
}

void FreenectConnection::CloseInternalLocked() {
  should_exit_ = true;
  delete freenect2_context_;
  if (freenect1_context_) freenect_shutdown(freenect1_context_);
  pthread_cond_broadcast(&connection_cond_);
  // TODO(igorc): De-init, but do not destroy. Also wait for threads to exit.
  DecRefLocked();
}

int FreenectConnection::GetDeviceCount() {
  Autolock l(mutex_);
  return freenect1_device_count_ + freenect2_device_count_;
}

bool FreenectConnection::GetVersionLocked(
    int device_index, DeviceVersion* version) const {
  if (device_index < 0) return false;
  if (device_index < freenect1_device_count_) {
    *version = kDeviceVersion1;
    return true;
  }
  device_index -= freenect1_device_count_;
  if (device_index < freenect2_device_count_) {
    *version = kDeviceVersion2;
    return true;
  }
  return false;
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

  if (!freenect2_context_) {
    freenect2_context_ = new libfreenect2::Freenect2();
    // freenect2_set_log_level(freenect2_context_, FREENECT2_LOG_DEBUG);
  }

  freenect1_device_count_ = freenect_num_devices(freenect1_context_);
  freenect2_device_count_ = freenect2_context_->enumerateDevices();

  fprintf(stderr, "Found %d Kinect1 and %d Kinect2 devices\n",
	  freenect1_device_count_, freenect2_device_count_);

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
        freenect1_context_, OnFreenect1VideoCallback, OnFreenect1DepthCallback,
	request);
  } else {
    DeviceOpenRequest request2 = request;
    request2.device_index -= freenect1_device_count_;
    base_device = new Freenect2Device(freenect2_context_, request2);
  }

  pthread_cond_broadcast(&connection_cond_);

  *device = base_device;
  return kErrorSuccess;
}

void FreenectConnection::CloseDeviceInternalLocked(Device* device) {
  BaseFreenectDevice* base_device =
      reinterpret_cast<BaseFreenectDevice*>(device);
  if (base_device->MarkConnectStarted()) {
    // The device did not start connecting yet.
    delete base_device;
    return;
  }
  // TODO(igorc): Stop and destroy after aborting Connect().
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
    BaseFreenectDevice* device;
    {
      Autolock l(mutex_);
      device = StartConnectingNextDeviceLocked();
      if (!device) {
        pthread_cond_wait(&connection_cond_, &mutex_);
        continue;
      }
    }

    device->Connect();
  }
}

BaseFreenectDevice* FreenectConnection::StartConnectingNextDeviceLocked() {
  Device* device = GetFirstDeviceLocked();
  while (device) {
    BaseFreenectDevice* base_device =
        reinterpret_cast<BaseFreenectDevice*>(device);
    if (base_device->MarkConnectStarted()) return base_device;
    device = GetNextDeviceLocked(device);
  }
  return NULL;
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
    int res = freenect_process_events(freenect1_context_);
    if (res == LIBUSB_ERROR_INTERRUPTED) {
      fprintf(stderr, "freenect1: LIBUSB_ERROR_INTERRUPTED\n");
      continue;
    }
    CHECK_FREENECT(res);
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
    if (device2->device() == dev) return device2;
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
