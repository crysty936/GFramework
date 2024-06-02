#pragma once

//#ifndef NDEBUG

#define LOG_INFO(x, ...)	{Logger::Get().Print(x, Severity::Info,		__VA_ARGS__);}
#define LOG_WARNING(x, ...)	{Logger::Get().Print(x, Severity::Warning,	__VA_ARGS__);}
#define LOG_ERROR(x, ...)	{Logger::Get().Print(x, Severity::Error,	__VA_ARGS__);}

#define LOG_ONCE_INFO(inMessage, ...)						\
  (([&](){										\
static bool bExecuted = false;								\
  if(!bExecuted)									\
  {															\
bExecuted = true;											\
LOG_INFO(inMessage, __VA_ARGS__);}							\
  }()))														\


#define LOG_ONCE_WARNING(inMessage, ...)						\
  (([&](){										\
static bool bExecuted = false;								\
  if(!bExecuted)									\
  {															\
bExecuted = true;											\
LOG_WARNING(inMessage, __VA_ARGS__);}							\
  }()))														\

#define LOG_ONCE_ERROR(inMessage, ...)						\
  (([&](){										\
static bool bExecuted = false;								\
  if(!bExecuted)									\
  {															\
bExecuted = true;											\
LOG_ERROR(inMessage, __VA_ARGS__);}							\
  }()))														\

//#else
//#define LOG_INFO(x, ...)
//#define LOG_WARNING(x, ...)
//#define LOG_ERROR(x, ...)
//#endif

enum class Severity
{
	Info,
	Warning,
	Error
};

class Logger
{
private:
	Logger();
	~Logger();

public:

	inline static Logger& Get() { return Instance; }
	void Print(const char* inFormat, Severity inSeverity, ...);

private:
	// To be developed for argument {#} replacing

// 	enum FormatValueType : char
// 	{
// 		intType,
// 		uintType,
// 		floatType,
// 		doubleType,
// 		stringType
// 	};
// 
// 	union allFormatValues
// 	{
// 		allFormatValues(int32_t inValue) : int_value{ inValue } {}
// 		allFormatValues(uint32_t inValue) : uint_value{ inValue } {}
// 		allFormatValues(float inValue) : float_value{ inValue } {}
// 		allFormatValues(double inValue) : double_value{ inValue } {}
// 		allFormatValues(const char* inValue) : string_value{ inValue } {}
// 
// 
// 		int32_t int_value;
// 		uint32_t uint_value;
// 		float float_value;
// 		double double_value;
// 		const char* string_value;
// 	};

	// 	struct ArgsContainer
// 	{
// 		ArgsContainer(int32_t inValue)
// 			:value{ inValue }, Type{ FormatValueType::intType } {}
// 
// 		ArgsContainer(uint32_t inValue)
// 			:value{ inValue }, Type{ FormatValueType::uintType } {}
// 
// 		ArgsContainer(float inValue)
// 			:value{ inValue }, Type{ FormatValueType::floatType } {}
// 
// 		ArgsContainer(double inValue)
// 			:value{ inValue }, Type{ FormatValueType::doubleType } {}
// 
// 		ArgsContainer(const char* inValue)
// 			:value{ inValue }, Type{ FormatValueType::stringType } {}
// 
// 		allFormatValues value;
// 		FormatValueType Type;
// 	};
//  	template<typename... Args>
//  	void TypedArgsFormatTest(const char* inFormat, const Args&... inArgs)
//  	{
// 
//  		ArgsContainer args[] = { inArgs... };
// 
// 		ArgsContainer& arg1 = args[0];
// 
// 		if (arg1.Type == FormatValueType::intType)
// 		{
// 			int32_t value = arg1.value.int_value;
// 
// 			int a = 0;
// 		}
// 
//  		ArgsContainer& arg2 = args[1];
//  
//  		if (arg2.Type == FormatValueType::stringType)
//  		{
//  			const char* value = arg2.value.string_value;
//  
//  			int a = 0;
//  		}
//  	}

private:
	static Logger Instance;
};

