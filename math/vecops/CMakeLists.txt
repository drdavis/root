############################################################################
# CMakeLists.txt file for building ROOT math/vecops package
############################################################################

set(HEADERS
  ROOT/TAdoptAllocator.hxx
  ROOT/TVec.hxx
)

set(SOURCES
  TAdoptAllocator.cxx
  TVec.cxx
)

if(vdt)
  # needed for rootcling, to be removed
  include_directories(${VDT_INCLUDE_DIRS})
endif()

ROOT_STANDARD_LIBRARY_PACKAGE(ROOTVecOps
                              HEADERS ${HEADERS}
                              SOURCES ${SOURCES}
                              DICTIONARY_OPTIONS "-writeEmptyRootPCM"
                              DEPENDENCIES Core)

if(vdt)
  target_link_libraries(ROOTVecOps Vdt::Vdt)
endif()

target_compile_options(ROOTVecOps PRIVATE -O3)

ROOT_ADD_TEST_SUBDIRECTORY(test)
