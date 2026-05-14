FindVcpkgPackage(OpenCV LINKAGE DYNAMIC REQUIRED CONFIG COMPONENTS core imgcodecs highgui imgproc)

target_link_libraries(${PROJECT_NAME} PRIVATE
    opencv_core
    opencv_imgproc
    opencv_imgcodecs
    opencv_highgui
)

set(DYNAMIC_LINK_LIBRARY_LIST 
  "opencv_core*"
  "opencv_imgcodecs*"
  "opencv_highgui*"
  "opencv_imgproc*"
  "opencv_videoio*"
  "jpeg62*"
  "libpng*"
  "libwebp*"
  "tiff*"
  "avformat*"
  "avcodec*"
  "avutil*"
  "swscale*"
  "swresample*"
  "libsharpyuv*"
  "liblzma*"
  "zlib*"
)

CopyTargetDependentLibs(${PROJECT_NAME} "${DYNAMIC_LINK_LIBRARY_LIST}" ${VCPKG_DYNAMIC_BIN_PATH} ${VCPKG_DYNAMIC_DEBUG_BIN_PATH})
