include_directories(${CMAKE_CURRENT_SOURCE_DIR})

LIST(APPEND SRC kk_connection.cc
                kk_freenect_base.cc
                kk_freenect1_device.cc
                kk_freenect2_device.cc
                utils.cc)

add_library (kkonnect SHARED ${SRC})
set_target_properties (kkonnect PROPERTIES
  VERSION ${PROJECT_VER}
  SOVERSION ${PROJECT_APIVER})	

add_library (kkonnectstatic STATIC ${SRC})
set_target_properties (kkonnectstatic PROPERTIES OUTPUT_NAME kkonnect)
SET_TARGET_PROPERTIES (kkonnectstatic PROPERTIES COMPILE_FLAGS "-fPIC")
