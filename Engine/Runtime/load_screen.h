#pragma once
#include "Engine/Ecore/render_path2d.h"
#include "Engine/Ecore/color.h"
#include "Module/Util/job_system.h"
#include "Module/Container/pvector.h"
#include "Engine/Ecore/fade_manager.h"

#include <functional>


namespace pf {

	class Application;

	class LoadingScreen :
		public RenderPath2D
	{
	protected:
		pf::jobsystem::context ctx;
		pf::vector<std::function<void(pf::jobsystem::JobArgs)>> tasks;
		std::function<void()> finish;
		uint32_t launchedTasks = 0;

	public:
		pf::Resource backgroundTexture;

		enum class BackgroundMode
		{
			Fill,	// fill the whole screen, will cut off parts of the image if aspects don't match
			Fit,	// fit the image completely inside the screen, will result in black bars on screen if aspects don't match
			Stretch	// fill the whole screen, and stretch the image if needed
		} background_mode = BackgroundMode::Fill;

		//Add a loading task which should be executed
		void addLoadingFunction(std::function<void(pf::jobsystem::JobArgs)> loadingFunction);
		//Helper for loading a whole renderable component
		void addLoadingComponent(RenderPath* component, Application* main, float fadeSeconds = 0, pf::Color fadeColor = pf::Color(0, 0, 0, 255), pf::FadeManager::FadeType fadetype = pf::FadeManager::FadeType::FadeToColor);
		//Set a function that should be called when the loading finishes
		void onFinished(std::function<void()> finishFunction);
		//See if the loading is currently running
		bool isActive() const;
		// See if there are any loading tasks that are still not finished
		bool isFinished() const;
		// Returns the percentage of loading tasks that are finished (0% - 100%)
		int getProgress() const;
		//Start Executing the tasks and mark the loading as active
		void Start() override;

		void Compose(pf::graphics::CommandList cmd) const override;
	};


}