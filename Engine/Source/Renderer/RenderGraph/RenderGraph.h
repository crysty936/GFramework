#pragma once
#include <utility>
#include "EASTL/vector.h"

struct RenderPassBase
{
	RenderPassBase() = default;
	~RenderPassBase() = default;

	virtual void Execute(class RGBuilder& inRGBuilder) = 0;

};


template<typename LambdaType>
struct RenderPass
{
	RenderPass(LambdaType&& inPassLambda)
		: PassLambda(std::forward(inPassLambda))
	{

	}

	void Execute(class RGBuilder& inRGBuilder) override;
	
	
	
	LambdaType PassLambda;
};


template<typename LambdaType>
void RenderPass<LambdaType>::Execute(class RGBuilder& inRGBuilder)
{
	PassLambda(inRGBuilder);



}

class RGBuilder
{
public:
	RGBuilder(struct ID3D12CommandList* inCmdList);
	~RGBuilder();

	template<typename LambdaType>
	void AddPass(LambdaType&& inPassFunc);


private:
	struct ID3D12CommandList* CmdList;
	eastl::vector<RenderPassBase*> RegisteredPasses;


	static inline bool bRecording = false;


};



template<typename LambdaType>
void RGBuilder::AddPass(LambdaType&& inPassFunc)
{
	RenderPass<LambdaType>* newPass = new RenderPass<LambdaType>(std::forward(inPassFunc));
	RegisteredPasses.push_back(newPass);


}
