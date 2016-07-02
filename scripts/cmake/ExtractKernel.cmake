macro(add_kernel_library name )
  set(CMAKE_CXX_COMPILER "${PROJECT_BINARY_DIR}/compiler/bin/clang++")

  #HOST side library
  add_library( ${name} SHARED ${ARGN} )
  include_directories(${name} ${HSAKMT_HEADER})
  target_link_libraries( ${name} ${HSAKMT_LIBRARY} )
  amp_target(${name})
  target_compile_options(${name} PUBLIC -hc)
  # LLVM and Clang shall be compiled beforehand

  add_library( ${name}-static STATIC ${ARGN} )
  amp_target(${name}-static)
  target_compile_options(${name}-static PUBLIC -hc)
  include_directories(${name}-static ${HSAKMT_HEADER})
  add_custom_command(
    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/${name}.bc
    COMMAND ar -x  $<TARGET_FILE:${name}-static>
    COMMAND objcopy -j .kernel -O binary `ar -t $<TARGET_FILE:${name}-static>`  ${CMAKE_CURRENT_BINARY_DIR}/${name}.bc
    DEPENDS ${name}-static
  )
  add_custom_target( ${name}.bc DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/${name}.bc)
endmacro(add_kernel_library name )
