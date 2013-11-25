macro(ensure_clang_is_present dest_dir name url)
if(EXISTS "${dest_dir}/${name}/tools/clang")
  MESSAGE("clang is present.")
else(EXISTS "${dest_dir}/${name}/tools/clang")
  MESSAGE("Cloning clang from ${url}...")
  Find_Package(Git)
  Find_Program(GITL_EXECUTABLE git)
  execute_process( COMMAND ${GIT_EXECUTABLE} clone ${url} ${dest_dir}/${name}/tools/clang )
endif(EXISTS "${dest_dir}/${name}/tools/clang")

endmacro()

