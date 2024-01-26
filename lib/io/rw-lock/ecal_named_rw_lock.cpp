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

#include <ecal/ecal_os.h>

#include "ecal_named_rw_lock.h"

#ifdef ECAL_OS_LINUX
#include "linux/ecal_named_rw_lock_impl.h"
#if defined(ECAL_HAS_ROBUST_MUTEX) || defined(ECAL_HAS_CLOCKLOCK_MUTEX)
#include "linux/ecal_named_rw_lock_robust_clocklock_impl.h"
#endif
#endif

#ifdef ECAL_OS_WINDOWS
#include "win32/ecal_named_rw_lock_impl.h"
#endif

#include <iostream>

namespace eCAL
{
    CNamedRwLock::CNamedRwLock(const std::string& name_) : CNamedRwLock()
    {
        Create(name_);
    }

    CNamedRwLock::CNamedRwLock()
    {
        m_impl = std::make_unique<CNamedRwLockStubImpl>();
    }

    CNamedRwLock::~CNamedRwLock()
    {
    }

    CNamedRwLock::CNamedRwLock(CNamedRwLock&& named_rw_lock)
    {
        m_impl.swap(named_rw_lock.m_impl);
    }

    CNamedRwLock& CNamedRwLock::operator=(CNamedRwLock&& named_rw_lock)
    {
        m_impl.swap(named_rw_lock.m_impl);
        named_rw_lock.m_impl.reset();
        return *this;
    }

    bool CNamedRwLock::Create(const std::string& name_)
    {
#ifdef ECAL_OS_LINUX
#if !defined(ECAL_USE_CLOCKLOCK_MUTEX) && defined(ECAL_HAS_ROBUST_MUTEX)
        if (recoverable_)
            m_impl = std::make_unique<CNamedRwLockRobustClockLockImpl>(name_, true);
        else
            m_impl = std::make_unique<CNamedRwLockImpl>(name_, false);
#elif defined(ECAL_USE_CLOCKLOCK_MUTEX) && defined(ECAL_HAS_CLOCKLOCK_MUTEX)
        m_impl = std::make_unique<CNamedRwLockRobustClockLockImpl>(name_, recoverable_);
#else
        m_impl = std::make_unique<CNamedRwLockImpl>(name_, recoverable_);
#endif
#endif

#ifdef ECAL_OS_WINDOWS
        m_impl = std::make_unique<CNamedRwLockImpl>(name_);
#endif
        return IsCreated();
    }

    void CNamedRwLock::Destroy()
    {
        m_impl = std::make_unique<CNamedRwLockStubImpl>();
    }

    bool CNamedRwLock::IsCreated() const
    {
        return m_impl->IsCreated();
    }

    bool CNamedRwLock::HasOwnership() const
    {
        return m_impl->HasOwnership();
    }

    void CNamedRwLock::DropOwnership()
    {
        m_impl->DropOwnership();
    }

    int CNamedRwLock::GetReaderCount()
    {
      return m_impl->GetReaderCount();
    }

    bool CNamedRwLock::LockRead(int64_t timeout_) 
    {
      return m_impl->LockRead(timeout_);
    }

    bool CNamedRwLock::UnlockRead(int64_t timeout_)
    {
      return m_impl->UnlockRead(timeout_);
    }

    bool CNamedRwLock::Lock(int64_t timeout_)
    {
        return m_impl->Lock(timeout_);
    }

    void CNamedRwLock::Unlock()
    {
        m_impl->Unlock();
    }
}

