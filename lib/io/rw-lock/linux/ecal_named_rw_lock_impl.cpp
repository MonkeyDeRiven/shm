/* ========================= eCAL LICENSE =================================
 *
 * Copyright (C) 2016 - 2022 Continental Corporation
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * ========================= eCAL LICENSE =================================
*/

/**
 * @brief  eCAL named mutex
**/

#include <ecal/ecal_os.h>

#include "ecal_named_rw_lock_impl.h"

#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <pthread.h>
#include <unistd.h>
#include <cstdint>
#include <string>

#include <iostream>

struct alignas(8) named_rw_lock
{
  pthread_rwlock_t  rw_lock;
};
typedef struct named_rw_lock named_rw_lock_t;

namespace
{
  named_rw_lock_t* named_rw_lock_create(const char* rw_lock_name_)
  {
    // create shared memory file
    int previous_umask = umask(000);  // set umask to nothing, so we can create files with all possible permission bits
    int fd = ::shm_open(rw_lock_name_, O_RDWR | O_CREAT | O_EXCL, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH);
    umask(previous_umask);            // reset umask to previous permissions
    if (fd < 0) return nullptr;

    // set size to size of named mutex struct
    if(ftruncate(fd, sizeof(named_rw_lock_t)) == -1)
    {
      ::close(fd);
      return nullptr;
    }

    // create rw-lock
    pthread_rwlockattr_t shmrw;
    pthread_rwlockattr_init(&shmrw);
    pthread_rwlockattr_setpshared(&shmrw, PTHREAD_PROCESS_SHARED);

    // create condition variable
    //pthread_condattr_t shattr;
    //pthread_condattr_init(&shattr);
    //pthread_condattr_setpshared(&shattr, PTHREAD_PROCESS_SHARED);
#ifndef ECAL_OS_MACOS
    //pthread_condattr_setclock(&shattr, CLOCK_MONOTONIC);
#endif // ECAL_OS_MACOS


    // map them into shared memory
    auto* rw_lock = static_cast<named_rw_lock_t*>(mmap(nullptr, sizeof(named_rw_lock_t), PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0));
    ::close(fd);

    // initialize rw-lock and condition
    pthread_rwlock_init(&rw_lock->rw_lock, &shmrw);
    //pthread_cond_init(&rw_lock->cvar, &shattr);

    // start with unlocked mutex
    //rw_lock->locked = 0;

    // return new rw-lock
    return rw_lock;
  }

  bool named_rw_lock_lock_write(named_rw_lock_t* rwl_)
  {
    // wait for read access
    // block till all processes have released the lock
    pthread_rwlock_wrlock(&rwl_->rw_lock);
    // return true when lock was acquired
    return true;
  }

  bool named_rw_lock_trylock_write(named_rw_lock_t* rwl_)
  {
     // try to get write access without blocking
    if (pthread_rwlock_trywrlock(&rwl_->rw_lock) == 0)
      // return true if access is granted immediately
      return true;
    // return false if access was not granted immediately
    return false;
  }

  bool named_rw_lock_timedlock_write(named_rw_lock_t* rwl_, const timespec& timeout_)
  {
    // try to get write access
    // block if lock is held by another process
    // unblock and return false after timeout period
    int result = pthread_rwlock_timedwrlock(&rwl_->rw_lock, &timeout_);
    if (result == 0)
      // return true if access was granted within the timeout period
      return true;
    // return false if access was not granted within the timeout period
    return false;
  }

  bool named_rw_lock_lock_read(named_rw_lock_t* rwl_)
  {
    // wait for read access
    // block until lock is released by writer process
    pthread_rwlock_rdlock(&rwl_->rw_lock);
    // return true when lock was acquired
    return true;
  }

  bool named_rw_lock_trylock_read(named_rw_lock_t* rwl_)
  {
    // try to get read access without blocking
    if (pthread_rwlock_tryrdlock(&rwl_->rw_lock) == 0)
      // return true if access is granted immediately
      return true;
    // return false if access was not granted immediately
    return false;
  }

  bool named_rw_lock_timedlock_read(named_rw_lock_t* rwl_, const timespec& timeout_)
  {
    // try to get read access
    // block if lock is held by a writing process
    // unblock and return false after timeout period
    if (pthread_rwlock_timedrdlock(&rwl_->rw_lock, &timeout_) == 0)
      // return true if access was granted within the timeout period
      return true;
    // return false if access was not granted within the timeout period
    return false;
  }

  int named_rw_lock_unlock(named_rw_lock_t* rwl_)
  {
      return pthread_rwlock_unlock(&rwl_->rw_lock);
  }

  int named_rw_lock_destroy(const char* rw_lock_name_)
  {
    // destroy (unlink) shared memory file
    return(::shm_unlink(rw_lock_name_));
  }

  named_rw_lock_t* named_rw_lock_open(const char* rw_lock_name_)
  {
    // try to open existing shared memory file
    int fd = ::shm_open(rw_lock_name_, O_RDWR, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH);
    if (fd < 0) return nullptr;

    // map file content to mutex
    auto* rwl = static_cast<named_rw_lock_t*>(mmap(nullptr, sizeof(named_rw_lock_t), PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0));
    ::close(fd);

    // return opened mutex
    return rwl;
  }

  void named_rw_lock_close(named_rw_lock_t* rw_lock_)
  {
    // unmap condition mutex from shared memory file
    munmap(static_cast<void*>(rw_lock_), sizeof(named_rw_lock_t));
  }

  std::string named_rw_lock_buildname(const std::string& rw_lock_name_)
  {
    // build shm file name
    std::string rw_lock_name;
    if(rw_lock_name_[0] != '/') rw_lock_name = "/";
    rw_lock_name += rw_lock_name_;
    // naming prefix was changed from mtx to rwl // by Max
    rw_lock_name += "_rwl";

    return(rw_lock_name);
  }
}

namespace eCAL
{

  CNamedRwLockImpl::CNamedRwLockImpl(const std::string &name_, bool /*recoverable_*/) : m_rw_lock_handle(nullptr), m_named(name_), m_has_ownership(false), m_holds_write_lock(false), m_holds_read_lock(false)
  {
    if(name_.empty())
      return;

    // build shm file name
    const std::string rwl_name = named_rw_lock_buildname(m_named);

    // we try to open an existing rw-lock first
    m_rw_lock_handle = named_rw_lock_open(rwl_name.c_str());

    // if we could not open it we create a new one
    if(m_rw_lock_handle == nullptr)
    {
      m_rw_lock_handle = named_rw_lock_create(rwl_name.c_str());
      if(m_rw_lock_handle)
        m_has_ownership = true;
    }
  }

  CNamedRwLockImpl::~CNamedRwLockImpl()
  {
    // check mutex handle
    if(m_rw_lock_handle == nullptr) return;

    // unlock mutex
    named_rw_lock_unlock(m_rw_lock_handle);

    // close mutex
    named_rw_lock_close(m_rw_lock_handle);

    // clean-up if mutex instance has ownership
    if(m_has_ownership)
      named_rw_lock_destroy(named_rw_lock_buildname(m_named).c_str());
  }

  bool CNamedRwLockImpl::IsCreated() const
  {
    return m_rw_lock_handle != nullptr;
  }

  bool CNamedRwLockImpl::IsRecoverable() const
  {
    return false;
  }
  bool CNamedRwLockImpl::WasRecovered() const
  {
    return false;
  }

  bool CNamedRwLockImpl::HasOwnership() const
  {
    return m_has_ownership;
  }

  void CNamedRwLockImpl::DropOwnership()
  {
    m_has_ownership = false;
  }

  int CNamedRwLockImpl::GetReaderCount(){
    return 0;
  }

  bool CNamedRwLockImpl::LockRead(int64_t timeout_) {
      // check mutex handle
      if (m_rw_lock_handle == nullptr)
          return false;
      if (m_holds_read_lock)
          return false;

      bool is_locked(false);
      // timeout_ < 0 -> wait infinite
      if (timeout_ < 0) {
          is_locked = (named_rw_lock_lock_read(m_rw_lock_handle));
      }
          // timeout_ == 0 -> check lock state only
      else if (timeout_ == 0) {
          is_locked = (named_rw_lock_trylock_read(m_rw_lock_handle));
      }
          // timeout_ > 0 -> wait timeout_ ms
      else {
          struct timespec abstime;
          clock_gettime(CLOCK_MONOTONIC, &abstime);

          abstime.tv_sec = abstime.tv_sec + timeout_ / 1000;
          abstime.tv_nsec = abstime.tv_nsec + (timeout_ % 1000) * 1000000;
          while (abstime.tv_nsec >= 1000000000) {
              abstime.tv_nsec -= 1000000000;
              abstime.tv_sec++;
          }
          is_locked = (named_rw_lock_timedlock_read(m_rw_lock_handle, abstime));
      }
      if (is_locked)
          m_holds_read_lock = true;
      return is_locked;
  }

  bool CNamedRwLockImpl::UnlockRead(int64_t timeout_)
  {
      if (m_holds_read_lock) {
          int is_unlocked(named_rw_lock_unlock(m_rw_lock_handle));
          if (is_unlocked == 0)
              m_holds_read_lock = false;
          else
              std::cout << is_unlocked;
          return is_unlocked;
      }
      return false;
  }

  bool CNamedRwLockImpl::Lock(int64_t timeout_)
  {
    // check mutex handle
    if (m_rw_lock_handle == nullptr)
      return false;

    if (m_holds_write_lock)
        return false;

    bool is_locked(false);
    // timeout_ < 0 -> wait infinite
    if (timeout_ < 0)
    {
      is_locked = (named_rw_lock_lock_write(m_rw_lock_handle));
    }
      // timeout_ == 0 -> check lock state only
    else if (timeout_ == 0)
    {
      is_locked = (named_rw_lock_trylock_write(m_rw_lock_handle));
    }
      // timeout_ > 0 -> wait timeout_ ms
    else
    {
      struct timespec abstime;
      clock_gettime(CLOCK_MONOTONIC, &abstime);

      abstime.tv_sec = abstime.tv_sec + timeout_ / 1000;
      abstime.tv_nsec = abstime.tv_nsec + (timeout_ % 1000) * 1000000;
      while (abstime.tv_nsec >= 1000000000)
      {
        abstime.tv_nsec -= 1000000000;
        abstime.tv_sec++;
      }
      is_locked = (named_rw_lock_timedlock_write(m_rw_lock_handle, abstime));
    }
    if (is_locked)
        m_holds_write_lock = true;
    return is_locked;
  }

  bool CNamedRwLockImpl::Unlock()
  {
    // check mutex handle
    if(m_rw_lock_handle == nullptr)
      return false;

    if (!m_holds_write_lock)
        return false;

    // unlock the mutex
    int result = named_rw_lock_unlock(m_rw_lock_handle);
    if (result == 0) {
        m_holds_write_lock = false;
    }
    else {
        std::cout << result;
        return false;
    }
    return true;
  }
}
