#include "Utils/Utils.h"
#include "EAStdC/EADateTime.h"

namespace Utils
{
	eastl::string GetTimeString()
	{
		time_t rawtime;
		tm timeinfo;
		char buffer[80];

		time(&rawtime);
		errno_t err = localtime_s(&timeinfo, &rawtime);
		ASSERT(err == 0);

		EA::StdC::Strftime(buffer, sizeof(buffer), "%d-%m-%H-%M-%S", &timeinfo);
		eastl::string str(buffer);

		return str;
	}

}
