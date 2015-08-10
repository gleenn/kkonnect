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

#ifndef KKONNECT_KK_FREENECT_CONNECTION_H_
#define KKONNECT_KK_FREENECT_CONNECTION_H_

#include <kk_device.h>
#include <pthread.h>

#include "src/kk_freenect_base.h"
#include "src/kk_freenect1_device.h"
#include "src/kk_freenect2_device.h"
#include "src/utils.h"

namespace kkonnect {

// Implements connection using local libfreenect and libfreenect2.
class FreenectConnection : public Connection {
 public:
  static FreenectConnection* GetInstanceImpl();

  virtual ErrorCode Refresh();
  virtual int GetDeviceCount();
  virtual ErrorCode GetDeviceInfo(int device_index, DeviceInfo* info);

 protected:
  FreenectConnection();
  virtual ~FreenectConnection();

  virtual void CloseInternalLocked();

  virtual ErrorCode OpenDeviceInternalLocked(
      const DeviceOpenRequest& request, Device** device);
  virtual void CloseDeviceInternalLocked(Device* device);

 private:
  void DecRefLocked();

  bool GetVersionLocked(int device_index, DeviceVersion* version) const;

  static void* RunConnectLoop(void* arg);
  void RunConnectLoop();
  BaseFreenectDevice* StartConnectingNextDeviceLocked();

  static void* RunFreenect1Loop(void* arg);
  static void OnFreenect1DepthCallback(
      freenect_device* dev, void* depth_data, uint32_t timestamp);
  static void OnFreenect1VideoCallback(
      freenect_device* dev, void* rgb_data, uint32_t timestamp);
  void RunFreenect1Loop();
  void HandleFreenect1DepthData(freenect_device* dev, void* depth_data);
  void HandleFreenect1VideoData(freenect_device* dev, void* rgb_data);
  Freenect1Device* FindFreenect1Locked(freenect_device* dev) const;

  static void OnFreenect2DepthCallback(
      freenect2_device* dev, uint32_t timestamp, void* depth_data, void* user);
  static void OnFreenect2VideoCallback(
      freenect2_device* dev, uint32_t timestamp, void* video_data, void* user);
  void HandleFreenect2DepthData(freenect2_device* dev, void* depth_data);
  void HandleFreenect2VideoData(freenect2_device* dev, void* video_data);
  Freenect2Device* FindFreenect2Locked(freenect2_device* dev) const;

  static pthread_mutex_t global_mutex_;
  static FreenectConnection* instance_;

  int ref_count_;
  volatile bool should_exit_;
  pthread_t connection_thread_;
  pthread_cond_t connection_cond_;
  freenect_context* freenect1_context_;
  pthread_t freenect1_thread_;
  int freenect1_device_count_;
  freenect2_context* freenect2_context_;
  int freenect2_device_count_;
};

}  // namespace kkonnect

#endif  // KKONNECT_KK_FREENECT_CONNECTION_H_
