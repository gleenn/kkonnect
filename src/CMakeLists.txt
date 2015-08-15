include_directories (${CMAKE_CURRENT_SOURCE_DIR})

list (APPEND SRC kk_connection.cc
                 kk_freenect_base.cc
                 kk_freenect_connection.cc
                 kk_freenect1_device.cc
                 #kk_freenect2_device.cc
                 utils.cc)

add_library (kkonnectstatic STATIC ${SRC})
set_target_properties (kkonnectstatic PROPERTIES OUTPUT_NAME kkonnect)
set_target_properties (kkonnectstatic PROPERTIES COMPILE_FLAGS "-fPIC")
#add_dependencies (kkonnectstatic libfreenect libfreenect2 libusb)
add_dependencies (kkonnectstatic libfreenect)

add_library (kkonnect SHARED ${SRC})
target_link_libraries (kkonnect PUBLIC libfreenect)
#target_link_libraries (kkonnect PUBLIC libfreenect2)
#target_link_libraries (kkonnect PRIVATE libusb)
#target_link_libraries (kkonnect PRIVATE turbojpeg)
#target_link_libraries (kkonnect OpenCL)
#target_link_libraries (kkonnect glfw)
set_target_properties (kkonnect PROPERTIES
  VERSION ${PROJECT_VER}
  SOVERSION ${PROJECT_APIVER})
