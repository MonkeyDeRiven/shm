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

#include "ecal_named_rw_lock_impl.h"

#include "ecal_win_main.h"
#include <iostream>
namespace eCAL
{
	CNamedRwLockImpl::CNamedRwLockImpl(const std::string& name_) : m_writer_mutex_handle(nullptr), m_reader_mutex_handle(nullptr), m_reader_count(0)
	{
		const std::string writer_mutex_name = name_ + "write_mtx";
		m_writer_mutex_handle = ::CreateMutex(
			nullptr,																							// no security descriptor
			false,																								// mutex not owned
			writer_mutex_name.c_str());		                        // object name

		const std::string reader_mutex_name = name_ + "read_mtx";
		m_reader_mutex_handle = ::CreateMutex(
			nullptr,																							// no security descriptor
			false,																								// mutex not owned
			reader_mutex_name.c_str());														// object name
	}

	CNamedRwLockImpl::~CNamedRwLockImpl()
	{
		// check writer mutex
		if (m_writer_mutex_handle != nullptr) {
			// release it
			ReleaseMutex(m_writer_mutex_handle);

			// close it
			CloseHandle(m_writer_mutex_handle);
		}

		// check reader mutex
		if (m_reader_mutex_handle != nullptr) {
			// release it
			ReleaseMutex(m_reader_mutex_handle);

			// close it
			CloseHandle(m_reader_mutex_handle);
		}
	}

	bool CNamedRwLockImpl::IsCreated() const
	{
		return m_writer_mutex_handle != nullptr && m_reader_mutex_handle != nullptr;
	}

	bool CNamedRwLockImpl::HasOwnership() const
	{
		return false;
	}

	void CNamedRwLockImpl::DropOwnership()
	{
	}

	int CNamedRwLockImpl::GetReaderCount()
	{
		return m_reader_count;
	}

	bool CNamedRwLockImpl::LockRead(int64_t timeout_)
	{
		//check mutex handles
		if (IsCreated() == false)
			return false;

		//wait for read mutex access
		DWORD result = WaitForSingleObject(m_reader_mutex_handle, static_cast<DWORD>(timeout_));
		if (result == WAIT_OBJECT_0)
		{
			// aquire reader lock
			m_reader_count++;
			if (m_reader_count == 1)
			{
				// block write access
				result = WaitForSingleObject(m_writer_mutex_handle, static_cast<DWORD>(timeout_));
				if (result == WAIT_OBJECT_0) {
					ReleaseMutex(m_reader_mutex_handle);
					return true;
				}
				m_reader_count--;
				ReleaseMutex(m_reader_mutex_handle);
				return false;
			}
			ReleaseMutex(m_reader_mutex_handle);
			return true;
		}
		return false;
	}

	bool CNamedRwLockImpl::UnlockRead(int64_t timeout_)
	{
		// check mutex handles
		if (IsCreated() == false)
			return false;

		// wait for read mutex access
		DWORD result = WaitForSingleObject(m_reader_mutex_handle, static_cast<DWORD>(timeout_));
		if (result == WAIT_OBJECT_0) {
			m_reader_count--;
			// release write access
			if (m_reader_count == 0) {
				// unblock writer
				ReleaseMutex(m_writer_mutex_handle);
			}
			ReleaseMutex(m_reader_mutex_handle);
			return true;
		}
		return false;
	}

	bool CNamedRwLockImpl::Lock(int64_t timeout_)
	{
		// check mutex handle
		if (m_writer_mutex_handle == nullptr)
			return false;

		// wait for access
		const DWORD result = WaitForSingleObject(m_writer_mutex_handle, static_cast<DWORD>(timeout_));
		if (result == WAIT_OBJECT_0)
			return true;
		return false;
	}

	void CNamedRwLockImpl::Unlock()
	{
		// check mutex handle
		if (m_writer_mutex_handle == nullptr)
			return;

		// release it
		ReleaseMutex(m_writer_mutex_handle);
	}
}
