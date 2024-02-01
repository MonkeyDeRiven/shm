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
	CNamedRwLockImpl::CNamedRwLockImpl(const std::string& name_, bool recoverable_) : m_mutex_handle(nullptr), m_reader_count(0), m_writer_active(false)
	{
		// create mutex
		const std::string writer_mutex_name = name_ + "_mtx";
		m_mutex_handle = ::CreateMutex(
			nullptr,																							// default security descriptor
			false,																								// mutex not owned
			writer_mutex_name.c_str());		                        // object name

		// create event, functioning as conditional variable
		const std::string event_name = name_ + "_event";
		m_event_handle = ::CreateEvent(
			nullptr,																							// default security descriptor
			true,																									// auto resets the signal state to non signaled, after a waiting process has been released
			false,																								// initial state non signaled
			event_name.c_str());																	// object name
	}

	CNamedRwLockImpl::~CNamedRwLockImpl()
	{
		// check mutex
		if (m_mutex_handle != nullptr) {
			// release it
			ReleaseMutex(m_mutex_handle);

			// close it
			CloseHandle(m_mutex_handle);
		}

		// check event
		if (m_event_handle != nullptr) {
			// close it
			CloseHandle(m_event_handle);
		}
	}

	bool CNamedRwLockImpl::IsCreated() const
	{
		return m_mutex_handle != nullptr && m_event_handle != nullptr;
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
		// lock mutex
		DWORD result = WaitForSingleObject(m_mutex_handle, static_cast<DWORD>(timeout_));
		if (result == WAIT_OBJECT_0) {
			// check if the writer is active
			if (m_writer_active) {
				// unlock the mutex to allow writer to unlock the rw-lock while waiting 
				ReleaseMutex(m_mutex_handle);
				// wait for writer to unlock and signal
				result = WaitForSingleObject(m_event_handle, static_cast<DWORD>(timeout_));
				// check if event was signaled before timeout
				if (result != WAIT_OBJECT_0) {
					return false;
				}
				// lock the mutex again, because the locking process is not done yet
				result = WaitForSingleObject(m_event_handle, static_cast<DWORD>(timeout_));
				// check if mutex was locked before timeout
				if (result != WAIT_OBJECT_0) {
					return false;
				}
			}
			// aquire reader lock and unlock mutex
			m_reader_count++;
			ReleaseMutex(m_mutex_handle);
			return true;
		}
		return false;
	}

	bool CNamedRwLockImpl::UnlockRead(int64_t timeout_)
	{
		// lock mutex
		DWORD result = WaitForSingleObject(m_mutex_handle, static_cast<DWORD>(timeout_));
		if (result == WAIT_OBJECT_0) {
			// release reader lock
			m_reader_count--;
			if (m_reader_count == 0) {
				// signal writer 
				SetEvent(m_event_handle);
			}
			ReleaseMutex(m_mutex_handle);
			return true;
		}
		return false;
	}

	bool CNamedRwLockImpl::Lock(int64_t timeout_)
	{
		DWORD result = WaitForSingleObject(m_mutex_handle, static_cast<DWORD>(timeout_));
		if (result == WAIT_OBJECT_0) {
			// check if any reader is active
			if (m_reader_count > 0 || m_writer_active) {
				// release mutex to allow readers to unlock while waiting
				ReleaseMutex(m_mutex_handle);
				// wait for readers to unlock and signal
				result = WaitForSingleObject(m_event_handle, static_cast<DWORD>(timeout_));
				// check if event was signaled before timeout
				if (result != WAIT_OBJECT_0) {
					return false;
				}
				// lock the mutex again, because the locking process is not done yet
				result = WaitForSingleObject(m_mutex_handle, static_cast<DWORD>(timeout_));
				// check if mutex was locked before timeout
				if (result != WAIT_OBJECT_0) {
					return false;
				}
			}
			// aquire writer lock and release mutex
			m_writer_active = true;
			ReleaseMutex(m_mutex_handle);
			return true;
		}
		return false;
	}

	bool CNamedRwLockImpl::Unlock()
	{
		// no timeout could lead to deadlock! 
		DWORD result = WaitForSingleObject(m_mutex_handle, INFINITE);
		if (result == WAIT_OBJECT_0) {
			m_writer_active = false;
			ReleaseMutex(m_mutex_handle);
			SetEvent(m_event_handle);
		}
	}
}
