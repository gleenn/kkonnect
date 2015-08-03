libkkonnect
===========

libkkonnect is a library for connecting to Microsoft Kinect devices.

Background, why : Two major versions of Kinect devices require 2 different
client-side librarys.

The goal is to provide a single API that supports:
1. libfreenect, to access Kinect for XBox360
2. libfreenect2, to access Kinect for XBoxOne and K4W2
3. Remote kkonnect host, to access a remote Kinect via TCP/IP
4. Access to multiple Kinect devices connected to a single physical host


# Build Instructions

At this point the code is developed and tested on Linux only.
Please contribute code to support other platforms.



# Code Contributions

Feel free to send pull requests to fix bugs. Please use issue tracker
to start feature or any other discussions.

The code should generally follow Google C++ Style Guide.


# Maintainers

- Original Code: Igor Chernyshev


# License

The libfreenect project is covered under a dual Apache v2/GPL v2
license. The licensing criteria are listed below, as well as at the
top of each source file in the repo.

```
This file is part of the OpenKinect Project. http://www.openkinect.org

Copyright (c) 2010 individual OpenKinect contributors. See the CONTRIB
file for details.

This code is licensed to you under the terms of the Apache License,
version 2.0, or, at your option, the terms of the GNU General Public
License, version 2.0. See the APACHE20 and GPL2 files for the text of
the licenses, or the following URLs:
http://www.apache.org/licenses/LICENSE-2.0
http://www.gnu.org/licenses/gpl-2.0.txt

If you redistribute this file in source form, modified or unmodified,
you may:

- Leave this header intact and distribute it under the same terms,
  accompanying it with the APACHE20 and GPL2 files, or
- Delete the Apache 2.0 clause and accompany it with the GPL2 file, or
- Delete the GPL v2 clause and accompany it with the APACHE20 file

In all cases you must keep the copyright notice intact and include a
copy of the CONTRIB file.

Binary distributions must follow the binary distribution requirements
of either License.
```
