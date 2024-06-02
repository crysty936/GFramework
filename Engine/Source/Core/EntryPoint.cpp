#include "AppModeBase.h"
#include "AppCore.h"

int main(int argc, char** argv)
{
	AppCore::Init();
	GEngine->Run();
}