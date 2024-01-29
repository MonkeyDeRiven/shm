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
#include <cstdint>

namespace eCAL
{
    class CNamedRwLockImplBase
    {
    public:
        virtual ~CNamedRwLockImplBase() = default;

        virtual bool IsCreated() const = 0;
        virtual bool IsRecoverable() const = 0;
        virtual bool WasRecovered() const = 0;
        virtual bool HasOwnership() const = 0;
        virtual int GetReaderCount() = 0;

        virtual void DropOwnership() = 0;

        virtual bool LockRead(int64_t timeout_) = 0;
        virtual bool UnlockRead(int64_t timeout_) = 0;
        virtual bool Lock(int64_t timeout_) = 0;
        virtual void Unlock() = 0;
    };

    class CNamedRwLockStubImpl : public CNamedRwLockImplBase
    {
    public:
        ~CNamedRwLockStubImpl()
        {
        }

        bool IsCreated() const final
        {
            return false;
        }

        bool IsRecoverable() const final
        {
          return false;
        }

        bool WasRecovered() const final
        {
          return false;
        }

        bool HasOwnership() const final
        {
            return false;
        }

        int GetReaderCount() final
        {
          return 0;
        }

        void DropOwnership() final
        {
        }

        bool LockRead(int64_t /*timeout_*/) final
        {
          return false;
        }

        bool UnlockRead(int64_t /*timeout_*/) final
        {
          return false;
        }

        bool Lock(int64_t /*timeout_*/) final 
        {
          return false;
        }

        void Unlock() final
        {
        };
    };
}