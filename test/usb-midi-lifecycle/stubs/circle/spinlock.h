#pragma once
#include <functional>
#include <stdexcept>
inline unsigned LockDepth = 0;
inline std::function<void ()> OnUnlock;
class CSpinLock
{
public:
	void Acquire ()
	{
		if (m_Locked)
		{
			throw std::runtime_error ("recursive lock");
		}
		m_Locked = true;
		++LockDepth;
	}
	void Release ()
	{
		if (!m_Locked)
		{
			throw std::runtime_error ("unbalanced unlock");
		}
		m_Locked = false;
		--LockDepth;
		if (!LockDepth && OnUnlock)
		{
			auto hook = OnUnlock;
			OnUnlock = nullptr;
			hook ();
		}
	}

private:
	bool m_Locked = false;
};
