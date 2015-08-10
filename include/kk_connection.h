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

#ifndef KKONNECT_KK_CONNECTION_H_
#define KKONNECT_KK_CONNECTION_H_

#include <pthread.h>

#include "kk_device.h"
#include "kk_errors.h"

namespace kkonnect {

// Provides access to all devices addressed by this connection.
class Connection {
 public:
  // Opens connection to all locally-attached devices.
  static Connection* OpenLocal();

  // Closes this connection. All opened Device objects become invalid.
  // This method will invoke Connection's destructor.
  void Close();

  // Refreshes the list of connected devices.
  virtual ErrorCode Refresh() = 0;

  // Obtains device info or establishes connection with a device.
  // Device indexes may only get updated on Refresh() call.
  virtual int GetDeviceCount() = 0;
  virtual ErrorCode GetDeviceInfo(int device_index, DeviceInfo* info) = 0;

  ErrorCode OpenDevice(const DeviceOpenRequest& request, Device** device);
  void CloseDevice(Device* device);

 protected:
  Connection();
  virtual ~Connection();

  // Invoked by Close() after closing all open devices.
  // Implementation of this method must destroy the connection object.
  virtual void CloseInternalLocked() = 0;

  virtual ErrorCode OpenDeviceInternalLocked(
      const DeviceOpenRequest& request, Device** device) = 0;
  virtual void CloseDeviceInternalLocked(Device* device) = 0;

  // Iterates over the list of all open devices.
  Device* GetFirstDeviceLocked() const { return devices_; }
  Device* GetNextDeviceLocked(Device* device) const { return device->next_; }

  mutable pthread_mutex_t mutex_;

 private:
  void CloseDeviceLocked(Device* device);

  Device* devices_;

  Connection(const Connection& src);
  Connection& operator=(const Connection& src);
};

}  // namespace kkonnect

#endif  // KKONNECT_KK_CONNECTION_H_
