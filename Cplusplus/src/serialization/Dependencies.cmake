FindVcpkgPackage(benchmark LINKAGE DYNAMIC REQUIRED CONFIG COMPONENTS benchmark benchmark_main)
FindVcpkgPackage(TBB LINKAGE STATIC REQUIRED CONFIG COMPONENTS tbb tbbmalloc tbbmalloc_proxy)

target_link_libraries(${PROJECT_NAME} PRIVATE
    benchmark::benchmark
    benchmark::benchmark_main
    TBB::tbb
    TBB::tbbmalloc
)

set(DYNAMIC_LINK_LIBRARY_LIST 
  "benchmark*"
)

CopyTargetDependentLibs(${PROJECT_NAME} "${DYNAMIC_LINK_LIBRARY_LIST}" ${VCPKG_DYNAMIC_BIN_PATH} ${VCPKG_DYNAMIC_DEBUG_BIN_PATH})
