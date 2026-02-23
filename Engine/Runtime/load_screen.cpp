#include "load_screen.h"
#include "runner.h"
#include "Module/Util/event_handle.h"
#include <thread>

using namespace pf::graphics;


namespace pf {

	bool LoadingScreen::isActive() const
	{
		return pf::jobsystem::IsBusy(ctx);
	}

	bool LoadingScreen::isFinished() const
	{
		return tasks.empty();
	}

	int LoadingScreen::getProgress() const
	{
		if (launchedTasks == 0)
			return 100;
		uint32_t counter = pf::jobsystem::GetRemainingJobCount(ctx);
		float percent = 1 - float(counter) / float(launchedTasks);
		return (int)std::round(percent * 100);
	}

	void LoadingScreen::addLoadingFunction(std::function<void(pf::jobsystem::JobArgs)> loadingFunction)
	{
		if (loadingFunction != nullptr)
		{
			tasks.push_back(loadingFunction);
		}
	}

	void LoadingScreen::addLoadingComponent(RenderPath* component, Application* main, float fadeSeconds, pf::Color fadeColor, pf::FadeManager::FadeType fadetype)
	{
		addLoadingFunction([=](pf::jobsystem::JobArgs args) {
			component->Load();
			});
		onFinished([=] {
			main->ActivatePath(component, fadeSeconds, fadeColor, fadetype);
			});
	}

	void LoadingScreen::onFinished(std::function<void()> finishFunction)
	{
		if (finishFunction != nullptr)
			finish = finishFunction;
	}

	void LoadingScreen::Start()
	{
		launchedTasks = (uint32_t)tasks.size();
		for (auto& x : tasks)
		{
			pf::jobsystem::Execute(ctx, x);
		}
		std::thread([this]() {
			pf::jobsystem::Wait(ctx);
			pf::eventhandler::Subscribe_Once(pf::eventhandler::EVENT_THREAD_SAFE_POINT, [this](uint64_t) {
				if (finish != nullptr)
					finish();
				tasks.clear();
				launchedTasks = 0;
				finish = nullptr;
				});
			}).detach();

			RenderPath2D::Start();
	}

	void LoadingScreen::Compose(pf::graphics::CommandList cmd) const
	{
		if (backgroundTexture.IsValid())
		{
			pf::image::Params fx;
			const Texture& tex = backgroundTexture.GetTexture();
			const TextureDesc& desc = tex.GetDesc();

			const float canvas_aspect = GetLogicalWidth() / GetLogicalHeight();
			const float image_aspect = float(desc.width) / float(desc.height);

			switch (background_mode)
			{
			default:
			case pf::LoadingScreen::BackgroundMode::Fill:
				if (canvas_aspect > image_aspect)
				{
					// display aspect is wider than image:
					fx.siz.x = GetLogicalWidth();
					fx.siz.y = GetLogicalHeight() / image_aspect * canvas_aspect;
				}
				else
				{
					// image aspect is wider or equal to display
					fx.siz.x = GetLogicalWidth() / canvas_aspect * image_aspect;
					fx.siz.y = GetLogicalHeight();
				}
				fx.pos = XMFLOAT3(GetLogicalWidth() * 0.5f, GetLogicalHeight() * 0.5f, 0);
				fx.pivot = XMFLOAT2(0.5f, 0.5f);
				break;
			case pf::LoadingScreen::BackgroundMode::Fit:
				if (canvas_aspect > image_aspect)
				{
					// display aspect is wider than image:
					fx.siz.x = GetLogicalWidth() / canvas_aspect * image_aspect;
					fx.siz.y = GetLogicalHeight();
				}
				else
				{
					// image aspect is wider or equal to display
					fx.siz.x = GetLogicalWidth();
					fx.siz.y = GetLogicalHeight() * canvas_aspect / image_aspect;
				}
				fx.pos = XMFLOAT3(GetLogicalWidth() * 0.5f, GetLogicalHeight() * 0.5f, 0);
				fx.pivot = XMFLOAT2(0.5f, 0.5f);
				break;
			case pf::LoadingScreen::BackgroundMode::Stretch:
				fx.enableFullScreen();
				break;
			}

			fx.blendFlag = pf::enums::BLENDMODE_ALPHA;
			if (colorspace != ColorSpace::SRGB)
			{
				fx.enableLinearOutputMapping(hdr_scaling);
			}

			pf::image::Draw(&tex, fx, cmd);
		}

		RenderPath2D::Compose(cmd);
	}
}
