add_library(shm)

target_sources(shm
PUBLIC
  ecal_def.h
  io/shm/ecal_memfile_header.h
  io/shm/ecal_memfile.h
  io/shm/ecal_memfile_db.h
  io/shm/ecal_memfile_info.h
  $<$<BOOL:${WIN32}>:${CMAKE_CURRENT_SOURCE_DIR}/io/mtx/win32/ecal_named_mutex_impl.h>
PRIVATE
  io/shm/ecal_memfile.cpp
  io/shm/ecal_memfile_db.cpp
  io/mtx/ecal_named_mutex.cpp
  $<$<BOOL:${WIN32}>:${CMAKE_CURRENT_SOURCE_DIR}/io/mtx/win32/ecal_named_mutex_impl.cpp>
  $<$<BOOL:${UNIX}>:${CMAKE_CURRENT_SOURCE_DIR}/io/mtx/linux/ecal_named_mutex_impl.cpp>
  $<$<BOOL:${WIN32}>:${CMAKE_CURRENT_SOURCE_DIR}/io/shm/win32/ecal_memfile_os.cpp>
  $<$<BOOL:${UNIX}>:${CMAKE_CURRENT_SOURCE_DIR}/io/shm/linux/ecal_memfile_os.cpp>
)

target_include_directories(shm PUBLIC . io/mtx io/shm)
