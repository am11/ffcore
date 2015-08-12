#pragma once

namespace ff
{
	class ReaderWriterLock
	{
	public:
		UTIL_API ReaderWriterLock(bool lockable = true);
		UTIL_API ~ReaderWriterLock();

		UTIL_API void EnterRead() const;
		UTIL_API bool TryEnterRead() const;
		UTIL_API void EnterWrite() const;
		UTIL_API bool TryEnterWrite() const;
		UTIL_API void LeaveRead() const;
		UTIL_API void LeaveWrite() const;
		UTIL_API bool IsLockable() const;
		UTIL_API void SetLockable(bool lockable);

	private:
		SRWLOCK *_lockable;
		SRWLOCK _lock;

	private:
		ReaderWriterLock(const ReaderWriterLock &rhs);
		const ReaderWriterLock &operator=(const ReaderWriterLock &rhs);
	};

	class LockReader
	{
	public:
		UTIL_API LockReader(const ReaderWriterLock &lock);
		UTIL_API ~LockReader();

		void Unlock();

	private:
		const ReaderWriterLock *_lock;
	};

	class LockWriter
	{
	public:
		UTIL_API LockWriter(const ReaderWriterLock &lock);
		UTIL_API ~LockWriter();

		void Unlock();

	private:
		const ReaderWriterLock *_lock;
	};
}
