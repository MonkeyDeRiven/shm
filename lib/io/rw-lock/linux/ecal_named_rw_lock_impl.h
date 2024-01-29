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

#pragma once

#include "io/rw-lock/ecal_named_rw_lock_base.h"

typedef struct named_rw_lock named_rw_lock_t;

namespace eCAL
{
  class CNamedRwLockImpl : public CNamedRwLockImplBase
  {
  public:
    CNamedRwLockImpl(const std::string &name_, bool recoverable_);
    ~CNamedRwLockImpl();
    
    CNamedRwLockImpl(const CNamedRwLockImpl&) = delete;
    CNamedRwLockImpl& operator=(const CNamedRwLockImpl&) = delete;
    CNamedRwLockImpl(CNamedRwLockImpl&&) = delete;
    CNamedRwLockImpl& operator=(CNamedRwLockImpl&&) = delete;

    bool IsCreated() const final;
    bool IsRecoverable() const final;
    bool WasRecovered() const final;
    bool HasOwnership() const final;
    int GetReaderCount();

    void DropOwnership() final;

    bool LockRead(int64_t timeout_);
    bool UnlockRead(int64_t timeout_);
    bool Lock(int64_t timeout_) final;
    void Unlock() final;
  private:
    named_rw_lock_t* m_rw_lock_handle;
    std::string m_named;
    bool m_has_ownership;
  };
}