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
 * @brief  eCAL named rw-lock
**/

#pragma once

#include "io/rw-lock/ecal_named_rw_lock_base.h"
#include <atomic>

class CreateMutexException : public std::exception {
public:
  CreateMutexException() : errorMessage("Could not create NamedMutex!") {};
  CreateMutexException(const char* msg) : errorMessage(msg) {};

  const char* what() const override {
    return errorMessage.c_str();
  }

private:
  std::string errorMessage;
};

class CreateEventException : public std::exception {
public:
  CreateEventException() : errorMessage("Could not create NamedEvent!") {};
  CreateEventException(const char* msg) : errorMessage(msg) {};

  const char* what() const override {
    return errorMessage.c_str();
  }

private:
  std::string errorMessage;
};

class SHMException : public std::exception {
public:
  SHMException() : errorMessage("Could not create shared memory!") {};
  SHMException(const char* msg) : errorMessage(msg) {};

  const char* what() const override {
    return errorMessage.c_str();
  }

private:
  std::string errorMessage;
};

struct named_rw_lock_state
{
  bool writer_active = false;
  int reader_count = 0;
};

typedef struct named_rw_lock_state state;

namespace eCAL
{
  class CNamedRwLockRecoverableImpl : public CNamedRwLockImplBase
  {
  public:
    CNamedRwLockRecoverableImpl(const std::string& name_, bool recoverable_);
    ~CNamedRwLockRecoverableImpl();

    CNamedRwLockRecoverableImpl(const CNamedRwLockRecoverableImpl&) = delete;
    CNamedRwLockRecoverableImpl& operator=(const CNamedRwLockRecoverableImpl&) = delete;
    CNamedRwLockRecoverableImpl(CNamedRwLockRecoverableImpl&&) = delete;
    CNamedRwLockRecoverableImpl& operator=(CNamedRwLockRecoverableImpl&&) = delete;

    bool IsCreated() const final;
    bool IsRecoverable() const final;
    bool WasRecovered() const final;
    bool HasOwnership() const final;
    int GetReaderCount() final;

    void DropOwnership() final;

    bool LockRead(int64_t timeout_) final;
    bool UnlockRead(int64_t timeout_) final;

    bool Lock(int64_t timeout_) final;
    bool Unlock() final;

  private:
    void* m_mutex_handle;
    void* m_event_handle;
    void* m_shm_handle;
    state* m_lock_state;

    // per lock attributes
    bool m_holds_write_lock;
    bool m_holds_read_lock;
  };
}