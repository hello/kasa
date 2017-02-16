#----------------------------------------------------------------
# Generated CMake target import file for configuration "Release".
#----------------------------------------------------------------

# Commands may need to know the format version.
SET(CMAKE_IMPORT_FILE_VERSION 1)

# Compute the installation prefix relative to this file.
GET_FILENAME_COMPONENT(_IMPORT_PREFIX "${CMAKE_CURRENT_LIST_FILE}" PATH)
GET_FILENAME_COMPONENT(_IMPORT_PREFIX "${_IMPORT_PREFIX}" PATH)
GET_FILENAME_COMPONENT(_IMPORT_PREFIX "${_IMPORT_PREFIX}" PATH)

# Import target "opencv_core" for configuration "Release"
SET_PROPERTY(TARGET opencv_core APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
SET_TARGET_PROPERTIES(opencv_core PROPERTIES
  IMPORTED_LOCATION_RELEASE "${_IMPORT_PREFIX}/lib/libopencv_core.so.2.4.8"
  IMPORTED_SONAME_RELEASE "libopencv_core.so.2.4"
  )

LIST(APPEND _IMPORT_CHECK_TARGETS opencv_core )
LIST(APPEND _IMPORT_CHECK_FILES_FOR_opencv_core "${_IMPORT_PREFIX}/lib/libopencv_core.so.2.4.8" )

# Import target "opencv_flann" for configuration "Release"
SET_PROPERTY(TARGET opencv_flann APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
SET_TARGET_PROPERTIES(opencv_flann PROPERTIES
  IMPORTED_LINK_INTERFACE_LIBRARIES_RELEASE "opencv_core"
  IMPORTED_LOCATION_RELEASE "${_IMPORT_PREFIX}/lib/libopencv_flann.so.2.4.8"
  IMPORTED_SONAME_RELEASE "libopencv_flann.so.2.4"
  )

LIST(APPEND _IMPORT_CHECK_TARGETS opencv_flann )
LIST(APPEND _IMPORT_CHECK_FILES_FOR_opencv_flann "${_IMPORT_PREFIX}/lib/libopencv_flann.so.2.4.8" )

# Import target "opencv_imgproc" for configuration "Release"
SET_PROPERTY(TARGET opencv_imgproc APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
SET_TARGET_PROPERTIES(opencv_imgproc PROPERTIES
  IMPORTED_LINK_INTERFACE_LIBRARIES_RELEASE "opencv_core"
  IMPORTED_LOCATION_RELEASE "${_IMPORT_PREFIX}/lib/libopencv_imgproc.so.2.4.8"
  IMPORTED_SONAME_RELEASE "libopencv_imgproc.so.2.4"
  )

LIST(APPEND _IMPORT_CHECK_TARGETS opencv_imgproc )
LIST(APPEND _IMPORT_CHECK_FILES_FOR_opencv_imgproc "${_IMPORT_PREFIX}/lib/libopencv_imgproc.so.2.4.8" )

# Import target "opencv_highgui" for configuration "Release"
SET_PROPERTY(TARGET opencv_highgui APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
SET_TARGET_PROPERTIES(opencv_highgui PROPERTIES
  IMPORTED_LINK_DEPENDENT_LIBRARIES_RELEASE "opencv_core;opencv_imgproc"
  IMPORTED_LOCATION_RELEASE "${_IMPORT_PREFIX}/lib/libopencv_highgui.so.2.4.8"
  IMPORTED_SONAME_RELEASE "libopencv_highgui.so.2.4"
  )

LIST(APPEND _IMPORT_CHECK_TARGETS opencv_highgui )
LIST(APPEND _IMPORT_CHECK_FILES_FOR_opencv_highgui "${_IMPORT_PREFIX}/lib/libopencv_highgui.so.2.4.8" )

# Import target "opencv_features2d" for configuration "Release"
SET_PROPERTY(TARGET opencv_features2d APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
SET_TARGET_PROPERTIES(opencv_features2d PROPERTIES
  IMPORTED_LINK_INTERFACE_LIBRARIES_RELEASE "opencv_core;opencv_flann;opencv_imgproc;opencv_highgui"
  IMPORTED_LOCATION_RELEASE "${_IMPORT_PREFIX}/lib/libopencv_features2d.so.2.4.8"
  IMPORTED_SONAME_RELEASE "libopencv_features2d.so.2.4"
  )

LIST(APPEND _IMPORT_CHECK_TARGETS opencv_features2d )
LIST(APPEND _IMPORT_CHECK_FILES_FOR_opencv_features2d "${_IMPORT_PREFIX}/lib/libopencv_features2d.so.2.4.8" )

# Import target "opencv_calib3d" for configuration "Release"
SET_PROPERTY(TARGET opencv_calib3d APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
SET_TARGET_PROPERTIES(opencv_calib3d PROPERTIES
  IMPORTED_LINK_INTERFACE_LIBRARIES_RELEASE "opencv_core;opencv_flann;opencv_imgproc;opencv_highgui;opencv_features2d"
  IMPORTED_LOCATION_RELEASE "${_IMPORT_PREFIX}/lib/libopencv_calib3d.so.2.4.8"
  IMPORTED_SONAME_RELEASE "libopencv_calib3d.so.2.4"
  )

LIST(APPEND _IMPORT_CHECK_TARGETS opencv_calib3d )
LIST(APPEND _IMPORT_CHECK_FILES_FOR_opencv_calib3d "${_IMPORT_PREFIX}/lib/libopencv_calib3d.so.2.4.8" )

# Import target "opencv_ml" for configuration "Release"
SET_PROPERTY(TARGET opencv_ml APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
SET_TARGET_PROPERTIES(opencv_ml PROPERTIES
  IMPORTED_LINK_INTERFACE_LIBRARIES_RELEASE "opencv_core"
  IMPORTED_LOCATION_RELEASE "${_IMPORT_PREFIX}/lib/libopencv_ml.so.2.4.8"
  IMPORTED_SONAME_RELEASE "libopencv_ml.so.2.4"
  )

LIST(APPEND _IMPORT_CHECK_TARGETS opencv_ml )
LIST(APPEND _IMPORT_CHECK_FILES_FOR_opencv_ml "${_IMPORT_PREFIX}/lib/libopencv_ml.so.2.4.8" )

# Import target "opencv_nonfree" for configuration "Release"
SET_PROPERTY(TARGET opencv_nonfree APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
SET_TARGET_PROPERTIES(opencv_nonfree PROPERTIES
  IMPORTED_LINK_INTERFACE_LIBRARIES_RELEASE "opencv_core;opencv_flann;opencv_imgproc;opencv_highgui;opencv_features2d;opencv_calib3d"
  IMPORTED_LOCATION_RELEASE "${_IMPORT_PREFIX}/lib/libopencv_nonfree.so.2.4.8"
  IMPORTED_SONAME_RELEASE "libopencv_nonfree.so.2.4"
  )

LIST(APPEND _IMPORT_CHECK_TARGETS opencv_nonfree )
LIST(APPEND _IMPORT_CHECK_FILES_FOR_opencv_nonfree "${_IMPORT_PREFIX}/lib/libopencv_nonfree.so.2.4.8" )

# Import target "opencv_objdetect" for configuration "Release"
SET_PROPERTY(TARGET opencv_objdetect APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
SET_TARGET_PROPERTIES(opencv_objdetect PROPERTIES
  IMPORTED_LINK_INTERFACE_LIBRARIES_RELEASE "opencv_core;opencv_imgproc;opencv_highgui"
  IMPORTED_LOCATION_RELEASE "${_IMPORT_PREFIX}/lib/libopencv_objdetect.so.2.4.8"
  IMPORTED_SONAME_RELEASE "libopencv_objdetect.so.2.4"
  )

LIST(APPEND _IMPORT_CHECK_TARGETS opencv_objdetect )
LIST(APPEND _IMPORT_CHECK_FILES_FOR_opencv_objdetect "${_IMPORT_PREFIX}/lib/libopencv_objdetect.so.2.4.8" )

# Import target "opencv_video" for configuration "Release"
SET_PROPERTY(TARGET opencv_video APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
SET_TARGET_PROPERTIES(opencv_video PROPERTIES
  IMPORTED_LINK_INTERFACE_LIBRARIES_RELEASE "opencv_core;opencv_imgproc"
  IMPORTED_LOCATION_RELEASE "${_IMPORT_PREFIX}/lib/libopencv_video.so.2.4.8"
  IMPORTED_SONAME_RELEASE "libopencv_video.so.2.4"
  )

LIST(APPEND _IMPORT_CHECK_TARGETS opencv_video )
LIST(APPEND _IMPORT_CHECK_FILES_FOR_opencv_video "${_IMPORT_PREFIX}/lib/libopencv_video.so.2.4.8" )

# Import target "opencv_contrib" for configuration "Release"
SET_PROPERTY(TARGET opencv_contrib APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
SET_TARGET_PROPERTIES(opencv_contrib PROPERTIES
  IMPORTED_LINK_INTERFACE_LIBRARIES_RELEASE "opencv_core;opencv_flann;opencv_imgproc;opencv_highgui;opencv_features2d;opencv_calib3d;opencv_ml;opencv_nonfree;opencv_objdetect;opencv_video"
  IMPORTED_LOCATION_RELEASE "${_IMPORT_PREFIX}/lib/libopencv_contrib.so.2.4.8"
  IMPORTED_SONAME_RELEASE "libopencv_contrib.so.2.4"
  )

LIST(APPEND _IMPORT_CHECK_TARGETS opencv_contrib )
LIST(APPEND _IMPORT_CHECK_FILES_FOR_opencv_contrib "${_IMPORT_PREFIX}/lib/libopencv_contrib.so.2.4.8" )

# Import target "opencv_legacy" for configuration "Release"
SET_PROPERTY(TARGET opencv_legacy APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
SET_TARGET_PROPERTIES(opencv_legacy PROPERTIES
  IMPORTED_LINK_INTERFACE_LIBRARIES_RELEASE "opencv_core;opencv_flann;opencv_imgproc;opencv_highgui;opencv_features2d;opencv_calib3d;opencv_ml;opencv_video"
  IMPORTED_LOCATION_RELEASE "${_IMPORT_PREFIX}/lib/libopencv_legacy.so.2.4.8"
  IMPORTED_SONAME_RELEASE "libopencv_legacy.so.2.4"
  )

LIST(APPEND _IMPORT_CHECK_TARGETS opencv_legacy )
LIST(APPEND _IMPORT_CHECK_FILES_FOR_opencv_legacy "${_IMPORT_PREFIX}/lib/libopencv_legacy.so.2.4.8" )

# Import target "opencv_photo" for configuration "Release"
SET_PROPERTY(TARGET opencv_photo APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
SET_TARGET_PROPERTIES(opencv_photo PROPERTIES
  IMPORTED_LINK_INTERFACE_LIBRARIES_RELEASE "opencv_core;opencv_imgproc"
  IMPORTED_LOCATION_RELEASE "${_IMPORT_PREFIX}/lib/libopencv_photo.so.2.4.8"
  IMPORTED_SONAME_RELEASE "libopencv_photo.so.2.4"
  )

LIST(APPEND _IMPORT_CHECK_TARGETS opencv_photo )
LIST(APPEND _IMPORT_CHECK_FILES_FOR_opencv_photo "${_IMPORT_PREFIX}/lib/libopencv_photo.so.2.4.8" )

# Import target "opencv_videostab" for configuration "Release"
SET_PROPERTY(TARGET opencv_videostab APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
SET_TARGET_PROPERTIES(opencv_videostab PROPERTIES
  IMPORTED_LINK_INTERFACE_LIBRARIES_RELEASE "opencv_core;opencv_flann;opencv_imgproc;opencv_highgui;opencv_features2d;opencv_calib3d;opencv_photo;opencv_video"
  IMPORTED_LOCATION_RELEASE "${_IMPORT_PREFIX}/lib/libopencv_videostab.so.2.4.8"
  IMPORTED_SONAME_RELEASE "libopencv_videostab.so.2.4"
  )

LIST(APPEND _IMPORT_CHECK_TARGETS opencv_videostab )
LIST(APPEND _IMPORT_CHECK_FILES_FOR_opencv_videostab "${_IMPORT_PREFIX}/lib/libopencv_videostab.so.2.4.8" )

# Loop over all imported files and verify that they actually exist
FOREACH(target ${_IMPORT_CHECK_TARGETS} )
  FOREACH(file ${_IMPORT_CHECK_FILES_FOR_${target}} )
    IF(NOT EXISTS "${file}" )
      MESSAGE(FATAL_ERROR "The imported target \"${target}\" references the file
   \"${file}\"
but this file does not exist.  Possible reasons include:
* The file was deleted, renamed, or moved to another location.
* An install or uninstall procedure did not complete successfully.
* The installation package was faulty and contained
   \"${CMAKE_CURRENT_LIST_FILE}\"
but not all the files it references.
")
    ENDIF()
  ENDFOREACH()
  UNSET(_IMPORT_CHECK_FILES_FOR_${target})
ENDFOREACH()
UNSET(_IMPORT_CHECK_TARGETS)

# Cleanup temporary variables.
SET(_IMPORT_PREFIX)

# Commands beyond this point should not need to know the version.
SET(CMAKE_IMPORT_FILE_VERSION)
