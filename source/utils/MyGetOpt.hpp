#pragma once
#include <getopt.h>
#include <string>
#include <functional>
#include "MyExcepts.hpp"

namespace MyCommon
{
	class MyGetOpt
	{
		protected:
			bool splitarg(std::string, std::string&, std::string&);

		public:
			virtual void ClearOpt() = 0;
			virtual void GetOpt(int, char**) = 0;
	};
}