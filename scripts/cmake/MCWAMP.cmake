include (CMakeForceCompiler)

# libc++
set(LIBCXX_SRC_DIR "${PROJECT_SOURCE_DIR}/libc++/libcxx")
set(LIBCXX_INC_DIR "${LIBCXX_SRC_DIR}/include")
set(LIBCXX_LIB_DIR "${PROJECT_BINARY_DIR}/libc++/libcxx/lib")

if (NOT APPLE)
# libcxxrt
set(LIBCXXRT_LIB_DIR "${PROJECT_BINARY_DIR}/libc++/libcxxrt/lib")
endif (NOT APPLE)
# gtest
set(GTEST_SRC_DIR "${PROJECT_SOURCE_DIR}/utils")
set(GTEST_INC_DIR "${PROJECT_SOURCE_DIR}/utils")

# MCWAMP
seT(MCWAMP_INC_DIR "${PROJECT_SOURCE_DIR}/include")

# CXXAMPFLAGS
set(CXXAMP_FLAGS "-I${GTEST_INC_DIR} -I${LIBCXX_INC_DIR} -I${MCWAMP_INC_DIR} -stdlib=libc++ -std=c++amp -DGTEST_HAS_TR1_TUPLE=0")

# STATIC ONLY FOR NOW.

####################
# C++AMP runtime interface (mcwamp) 
####################
macro(add_mcwamp_library name )
  CMAKE_FORCE_CXX_COMPILER("${PROJECT_BINARY_DIR}/compiler/bin/clang++" MCWAMPCC)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${CXXAMP_FLAGS} -fPIC -DCXXAMP_NV=1")
  if (HAS_HSA EQUAL 1)
    # add HSA headers
    include_directories(${HSA_HEADER})
    #add_definitions("-DCXXAMP_ENABLE_HSA=1")
  else (HAS_HSA EQUAL 1)
    # add OpenCL headers
    include_directories("${OPENCL_HEADER}/..")
  endif (HAS_HSA EQUAL 1)
  add_library( ${name} ${ARGN} )
endmacro(add_mcwamp_library name )

####################
# C++AMP runtime (OpenCL implementation) 
####################
macro(add_mcwamp_library_opencl name )
  CMAKE_FORCE_CXX_COMPILER("${PROJECT_BINARY_DIR}/compiler/bin/clang++" MCWAMPCC)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${CXXAMP_FLAGS} -fPIC -DCXXAMP_NV=1")
  # add OpenCL headers
  include_directories("${OPENCL_HEADER}/..")
  add_library( ${name} ${ARGN} )
endmacro(add_mcwamp_library_opencl name )

####################
# C++AMP runtime (HSA implementation) 
####################
macro(add_mcwamp_library_hsa name )
  CMAKE_FORCE_CXX_COMPILER("${PROJECT_BINARY_DIR}/compiler/bin/clang++" MCWAMPCC)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${CXXAMP_FLAGS} -fPIC")
  # add HSA headers
  include_directories(${HSA_HEADER} ${LIBHSAIL_HEADER} ${LIBHSAIL_HEADER}/generated)
  add_library( ${name} ${ARGN} )
endmacro(add_mcwamp_library_hsa name )

####################
# C++AMP config (clamp-config)
####################
macro(add_mcwamp_executable name )
  link_directories(${LIBCXX_LIB_DIR} ${LIBCXXRT_LIB_DIR})
  CMAKE_FORCE_CXX_COMPILER("${PROJECT_BINARY_DIR}/compiler/bin/clang++" MCWAMPCC)
  set(CMAKE_INSTALL_RPATH "${CMAKE_INSTALL_PREFIX}/lib")
  set(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -I${GTEST_INC_DIR} -I${LIBCXX_INC_DIR} -I${MCWAMP_INC_DIR} -stdlib=libc++ -std=c++amp" )
  add_executable( ${name} ${ARGN} )
  if (APPLE)
    target_link_libraries( ${name} mcwamp c++abi)
  else (APPLE)
    target_link_libraries( ${name} mcwamp cxxrt dl pthread)
  endif (APPLE)
endmacro(add_mcwamp_executable name )
