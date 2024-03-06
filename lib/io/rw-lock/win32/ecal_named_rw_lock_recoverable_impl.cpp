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
/*
#include "ecal_named_rw_lock_recoverable_impl.h"

#include "ecal_win_main.h"
#include <tlhelp32.h>
#include <iostream>
namespace eCAL
{
	CNamedRwLockRecoverableImpl::CNamedRwLockRecoverableImpl(const std::string& name_, bool recoverable_) : m_mutex_handle(nullptr), m_holds_read_lock(false), m_holds_write_lock(false)
	{
		// create mutex
		const std::string writer_mutex_name = name_ + "_mtx";
		m_mutex_handle = ::CreateMutex(
			nullptr,																							// default security descriptor
			false,																								// mutex not owned
			writer_mutex_name.c_str());		                        // object name

		if (m_mutex_handle == nullptr)
			throw CreateMutexException();

		// create event, functioning as conditional variable
		const std::string event_name = name_ + "_event";
		m_event_handle = ::CreateEvent(
			nullptr,																							// default security descriptor
			true,																									// auto resets the signal state to non signaled, after a waiting process has been released
			false,																								// initial state non signaled
			event_name.c_str());																	// object name

		if (m_event_handle == nullptr)
			throw CreateEventException();

		// create shared memory for lock state
		const std::string shared_memory_name = name_ + "_shm";
		m_shm_handle = ::CreateFileMapping(
			INVALID_HANDLE_VALUE,																	// allocate virtual memory
			nullptr,																							// default security descriptor
			PAGE_READWRITE,																				// allow read and write operations										
			0,																										// high-order DWORD of the maximum size
			sizeof(state),																				// low-order DWORD of the maximum size 
			shared_memory_name.c_str());													// object name

		if (m_shm_handle == nullptr)
			throw SHMException();

		// if this is the first call of CreateFileMapping
		if (GetLastError() != ERROR_ALREADY_EXISTS) {
			// map lock state struct into the newly allocated shared memory
			state* temp_state = (state*)MapViewOfFile(m_shm_handle, FILE_MAP_WRITE, 0, 0, sizeof(state));
			// could lead to dereferencing a nullptr
			*temp_state = state();
		}

		// get a pointer to the state
		m_lock_state = (state*)MapViewOfFile(m_shm_handle, PAGE_READONLY, 0, 0, sizeof(state));
	}

	CNamedRwLockRecoverableImpl::~CNamedRwLockRecoverableImpl()
	{
		// check read lock
		if (m_holds_read_lock)
			UnlockRead(INFINITE);
		if (m_holds_write_lock)
			Unlock();
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

		// check shm
		if (m_shm_handle != nullptr) {
			//close it
			CloseHandle(m_shm_handle);
		}
	}

	bool CNamedRwLockRecoverableImpl::IsCreated() const
	{
		return m_mutex_handle != nullptr && m_event_handle != nullptr;
	}

	bool CNamedRwLockRecoverableImpl::IsRecoverable() const
	{
		return false;
	}

	bool CNamedRwLockRecoverableImpl::WasRecovered() const
	{
		return false;
	}

	bool CNamedRwLockRecoverableImpl::HasOwnership() const
	{
		return false;
	}

	void CNamedRwLockRecoverableImpl::DropOwnership()
	{
	}

	int CNamedRwLockRecoverableImpl::GetReaderCount()
	{
		return m_lock_state->reader_count;
	}

	bool CNamedRwLockRecoverableImpl::IsBroken()
	{
		// take snapshot of current process table
		HANDLE h_process_snap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);

	}

	bool CNamedRwLockRecoverableImpl::LockRead(int64_t timeout_)
	{
		if (!IsCreated())
			return false;

		// lock mutex
		DWORD result = WaitForSingleObject(m_mutex_handle, static_cast<DWORD>(timeout_));
		if (result == WAIT_OBJECT_0) {
			// check if the writer is active
			if (m_lock_state->writer_active) {
				// unlock the mutex to allow writer to unlock the rw-lock while waiting 
				ReleaseMutex(m_mutex_handle);
				// wait for writer to unlock and signal
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
			// aquire reader lock and unlock mutex
			m_lock_state->reader_count++;
			m_lock_state->pids.push_back(GetCurrentProcessId());
			ReleaseMutex(m_mutex_handle);
			m_holds_read_lock = true;
			return true;
		}
		return false;
	}

	bool CNamedRwLockRecoverableImpl::UnlockRead(int64_t timeout_)
	{
		if (!IsCreated())
			return false;

		// dont change the lock state if the lock is not held by the caller 
		if (!m_holds_read_lock)
			return false;

		// lock mutex
		DWORD result = WaitForSingleObject(m_mutex_handle, static_cast<DWORD>(timeout_));
		if (result == WAIT_OBJECT_0) {
			// release reader lock
			m_lock_state->reader_count--;
			m_lock_state->pids.erase(std::find(m_lock_state->pids.begin(), m_lock_state->pids.end(), GetCurrentProcessId()));
			if (m_lock_state->reader_count == 0) {
				// signal writer 
				SetEvent(m_event_handle);
			}
			ReleaseMutex(m_mutex_handle);
			m_holds_read_lock = false;
			return true;
		}
		return false;
	}

	bool CNamedRwLockRecoverableImpl::Lock(int64_t timeout_)
	{
		if (!IsCreated())
			return false;

		DWORD result = WaitForSingleObject(m_mutex_handle, static_cast<DWORD>(timeout_));
		if (result == WAIT_OBJECT_0) {
			// check if any reader is active
			if (m_lock_state->reader_count > 0 || m_lock_state->writer_active) {
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
			m_lock_state->writer_active = true;
			m_lock_state->pids.push_back(GetCurrentProcessId());
			ReleaseMutex(m_mutex_handle);
			m_holds_write_lock = true;
			return true;
		}
		return false;
	}

	bool CNamedRwLockRecoverableImpl::Unlock()
	{
		if (!IsCreated())
			return false;

		// dont change the lock state if the lock is not held by the caller 
		if (!m_holds_write_lock)
			return false;

		// no timeout could lead to deadlock! 
		DWORD result = WaitForSingleObject(m_mutex_handle, INFINITE);
		if (result == WAIT_OBJECT_0) {
			m_lock_state->writer_active = false;
			m_lock_state->pids.erase(std::find(m_lock_state->pids.begin(), m_lock_state->pids.end(), GetCurrentProcessId()));
			ReleaseMutex(m_mutex_handle);
			SetEvent(m_event_handle);
			m_holds_write_lock = false;
			return true;
		}
		return false;
	}
}
*/
