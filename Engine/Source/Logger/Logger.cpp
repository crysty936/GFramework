#include <stdint.h>
#include <stdarg.h>
#include <stdio.h>
#include <iostream>
#include <string>
#include <type_traits>
#include "Logger/Logger.h"
#include "Core/EngineUtils.h"
#include "Core/WindowsPlatform.h"
// Fmt logger is a good source for this

Logger Logger::Instance;

Logger::Logger()
{
	static bool bLoggerExists = false;

    ASSERT(!bLoggerExists);

	bLoggerExists = true;

//     Typed format test
//     int32_t t1 = 1;
//     const char* t2 = "test param";
//     TypedArgsFormatTest("test arguments {0} {1}", t1, t2);
}

Logger::~Logger() = default;

void Logger::Print(const char* inFormat, Severity inSeverity, ...)
{
 //#ifdef NDEBUG
 //    return;
 //#endif

	// Try to use a stack allocated buffer
 	constexpr int32_t bufferSize = 1024;

  	char stackArray[bufferSize];
 
  	va_list argumentList;
  
  	va_start(argumentList, &inSeverity);
  
  	int32_t result = vsprintf_s(stackArray, bufferSize, inFormat, argumentList);
  
  	va_end(argumentList);
  
    // Means our allocated buffer is not enough, the log to be printed is too big
    ASSERT(result != -1);

    switch (inSeverity)
    {
    case Severity::Info:
    {
		WindowsPlatform::SetCLITextColor(CLITextColor::White);
        break;
    }
    case Severity::Warning:
    {
		WindowsPlatform::SetCLITextColor(CLITextColor::Yellow);
        break;
    }
    case Severity::Error:
    {
        WindowsPlatform::SetCLITextColor(CLITextColor::Red);
        break;
    }
    }

    std::cout << stackArray << std::endl;

	WindowsPlatform::SetCLITextColor(CLITextColor::White);
}
