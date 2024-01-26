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

#include <string>
#include <memory>
#include <cstdint>

namespace eCAL
{
    class CNamedRwLockImplBase;

    class CNamedRwLock
    {
    public:
        CNamedRwLock(const std::string& name_);
        CNamedRwLock();
        ~CNamedRwLock();

        CNamedRwLock(const CNamedRwLock&) = delete;
        CNamedRwLock& operator=(const CNamedRwLock&) = delete;
        CNamedRwLock(CNamedRwLock&& named_rw_lock);
        CNamedRwLock& operator=(CNamedRwLock&& named_rw_lock);

        bool Create(const std::string& name_);
        void Destroy();

        bool IsCreated() const;
        bool HasOwnership() const;
        int GetReaderCount();

        void DropOwnership();

        bool LockRead(int64_t timeout_);
        bool UnlockRead(int64_t timeout_);
        bool Lock(int64_t timeout_);
        void Unlock();

    private:
        std::unique_ptr<CNamedRwLockImplBase> m_impl;
    };
}