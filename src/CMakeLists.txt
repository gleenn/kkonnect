include_directories(${CMAKE_CURRENT_SOURCE_DIR})

LIST(APPEND SRC kk_connection.cc
                kk_device_freenect.cc
                kk_device_freenect2.cc
                kk_freenect_base.cc
                utils.cc)

add_library (kkonnect SHARED ${SRC})
set_target_properties (kkonnect PROPERTIES
  VERSION ${PROJECT_VER}
  SOVERSION ${PROJECT_APIVER})	

add_library (kkonnectstatic STATIC ${SRC})
set_target_properties (kkonnectstatic PROPERTIES OUTPUT_NAME kkonnect)
SET_TARGET_PROPERTIES (kkonnectstatic PROPERTIES COMPILE_FLAGS "-fPIC")
