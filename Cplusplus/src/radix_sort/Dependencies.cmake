FindVcpkgPackage(benchmark LINKAGE DYNAMIC REQUIRED CONFIG COMPONENTS benchmark benchmark_main)
FindVcpkgPackage(protobuf LINKAGE DYNAMIC REQUIRED CONFIG)
FindVcpkgPackage(yyjson LINKAGE STATIC REQUIRED CONFIG)

target_link_libraries(${PROJECT_NAME} PRIVATE
    benchmark::benchmark
    benchmark::benchmark_main
    protobuf::libprotobuf
    yyjson::yyjson
)

set(DYNAMIC_LINK_LIBRARY_LIST 
  "benchmark*"
  "libproto*"
)

CopyTargetDependentLibs(${PROJECT_NAME} "${DYNAMIC_LINK_LIBRARY_LIST}" ${VCPKG_DYNAMIC_BIN_PATH} ${VCPKG_DYNAMIC_DEBUG_BIN_PATH})
