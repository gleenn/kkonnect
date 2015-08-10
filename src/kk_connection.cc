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

// static
Connection* Connection::OpenLocal() {
  return FreenectConnection::GetInstanceImpl();
}

Connection::Connection() : devices_(NULL) {
  pthread_mutex_init(&mutex_, NULL);
}

Connection::~Connection() {
  CHECK(!devices_);
}

void Connection::Close() {
  Autolock l(mutex_);
  while (devices_) {
    CloseDeviceLocked(devices_);
  }
  CloseInternalLocked();  // Will self-destroy.
}

ErrorCode Connection::OpenDevice(const DeviceOpenRequest& request,
				 Device** device) {
  Autolock l(mutex_);
  Device* found_device = devices_;
  while (found_device) {
    if (found_device->index_ == request.device_index) {
      return kErrorAlreadyOpened;
    }
    found_device = found_device->next_;
  }

  ErrorCode result = OpenDeviceInternalLocked(request, device);
  if (result != kErrorSuccess) return result;
  (*device)->next_ = devices_;
  (*device)->index_ = request.device_index;
  devices_ = *device;
  return kErrorSuccess;
}

void Connection::CloseDevice(Device* device) {
  Autolock l(mutex_);
  CloseDeviceLocked(device);
}

void Connection::CloseDeviceLocked(Device* device) {
  Device* prev_device = NULL;
  Device* found_device = devices_;
  while (found_device) {
    if (found_device == device) break;
    prev_device = found_device;
    found_device = found_device->next_;
  }
  if (!found_device) {
    fprintf(stderr, "Attempting to close an unknown Kinect device\n");
    return;
  }
  CHECK(found_device == device);
  if (prev_device) {
    prev_device->next_ = found_device->next_;
  } else {
    CHECK(found_device == devices_);
    devices_ = devices_->next_;
  }
  CloseDeviceInternalLocked(found_device);
}

}  // namespace kkonnect
