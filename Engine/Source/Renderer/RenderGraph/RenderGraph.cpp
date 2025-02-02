#include "RenderGraph.h"

RGBuilder::RGBuilder(ID3D12CommandList* inCmdList)
	: CmdList(inCmdList)
{
	bRecording = true;


}

RGBuilder::~RGBuilder()
{
	bRecording = false;

}
