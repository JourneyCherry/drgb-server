#pragma once

namespace mylib{
namespace threads{

class PoolElement
{
	protected:
		virtual void Work() = 0;
		friend class Pool;
		friend class FixedPool;
};

}
}