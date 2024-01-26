add_library(shm)

target_sources(shm
PUBLIC
  ecal_def.h
  io/shm/ecal_memfile_header.h
  io/shm/ecal_memfile.h
  io/shm/ecal_memfile_db.h
  io/shm/ecal_memfile_info.h
  $<$<BOOL:${WIN32}>:${CMAKE_CURRENT_SOURCE_DIR}/io/mtx/win32/ecal_named_mutex_impl.h>
  $<$<BOOL:${WIN32}>:${CMAKE_CURRENT_SOURCE_DIR}/io/rw-lock/win32/ecal_named_rw_lock_impl.h>
PRIVATE
  io/shm/ecal_memfile.cpp
  io/shm/ecal_memfile_db.cpp
  io/mtx/ecal_named_mutex.cpp
  io/rw-lock/ecal_named_rw_lock.cpp
  $<$<BOOL:${WIN32}>:${CMAKE_CURRENT_SOURCE_DIR}/io/mtx/win32/ecal_named_mutex_impl.cpp>
  $<$<BOOL:${UNIX}>:${CMAKE_CURRENT_SOURCE_DIR}/io/mtx/linux/ecal_named_mutex_impl.cpp>
  $<$<BOOL:${WIN32}>:${CMAKE_CURRENT_SOURCE_DIR}/io/shm/win32/ecal_memfile_os.cpp>
  $<$<BOOL:${UNIX}>:${CMAKE_CURRENT_SOURCE_DIR}/io/shm/linux/ecal_memfile_os.cpp>
  $<$<BOOL:${WIN32}>:${CMAKE_CURRENT_SOURCE_DIR}/io/rw-lock/win32/ecal_named_rw_lock_impl.cpp>
)

target_include_directories(shm PUBLIC . io/mtx io/rw-lock io/shm)