#pragma once

namespace eXngine::Renderers
{
	class AbstractRenderer
	{
	public:
		virtual void Initialize() = 0;
		virtual void OnRender() = 0;
		virtual void OnExit() = 0;
	};
}