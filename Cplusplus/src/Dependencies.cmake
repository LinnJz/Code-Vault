FindVcpkgPackage(absl LINKAGE STATIC REQUIRED CONFIG COMPONENTS)
FindVcpkgPackage(benchmark LINKAGE DYNAMIC REQUIRED CONFIG COMPONENTS benchmark benchmark_main)
FindVcpkgPackage(cryptopp LINKAGE STATIC REQUIRED CONFIG)
FindVcpkgPackage(frozen LINKAGE STATIC REQUIRED CONFIG)
FindVcpkgPackage(magic_enum LINKAGE STATIC REQUIRED CONFIG)
FindVcpkgPackage(protobuf LINKAGE DYNAMIC REQUIRED CONFIG)
FindVcpkgPackage(pugixml LINKAGE STATIC REQUIRED CONFIG)
FindVcpkgPackage(TBB LINKAGE STATIC REQUIRED CONFIG COMPONENTS tbb tbbmalloc tbbmalloc_proxy)
FindVcpkgPackage(yyjson LINKAGE STATIC REQUIRED CONFIG)
FindVcpkgPackage(glaze LINKAGE DYNAMIC REQUIRED CONFIG)
FindVcpkgPackage(folly LINKAGE STATIC REQUIRED CONFIG)

target_link_libraries(${PROJECT_NAME} PRIVATE
    #absl::base
    Folly::folly
    absl::inlined_vector
    absl::flat_hash_set
    benchmark::benchmark
    benchmark::benchmark_main
    #cryptopp::cryptopp
    #pugixml::pugixml
    #protobuf::libprotobuf
    TBB::tbb
    TBB::tbbmalloc
    glaze::glaze
    #yyjson::yyjson
)
target_compile_definitions(${PROJECT_NAME} PRIVATE CRYPTOPP_ENABLE_NAMESPACE_WEAK=1)

set(DYNAMIC_LINK_LIBRARY_LIST 
  "benchmark*"
  #"libproto*"
)

CopyTargetDependentLibs(${PROJECT_NAME} "${DYNAMIC_LINK_LIBRARY_LIST}" ${VCPKG_DYNAMIC_BIN_PATH} ${VCPKG_DYNAMIC_DEBUG_BIN_PATH})
