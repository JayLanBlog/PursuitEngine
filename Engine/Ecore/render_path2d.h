#pragma once
#include "Engine/Component/pgui.h"
#include "render_path.h"
#include "Module/Container/container_include.h"
#include "video.h"
#include <string>



namespace pf
{
	class Sprite;
	class SpriteFont;

	class RenderPath2D :
		public RenderPath
	{
	protected:
		pf::graphics::Texture rtStencilExtracted;
		pf::graphics::Texture stencilScaled;

		pf::graphics::Texture rtFinal;
		pf::graphics::Texture rtFinal_MSAA;

		pf::gui::GUI GUI;

		XMUINT2 current_buffersize{};
		float current_layoutscale{};

		float hdr_scaling = 9.0f;

		uint32_t msaaSampleCount = 1;
		uint32_t msaaSampleCount2D = 1;

		pf::vector<pf::video::VideoInstance*> video_decodes;

	public:
		// Delete GPU resources and initialize them to default
		virtual void DeleteGPUResources();
		// create resolution dependent resources, such as render targets
		virtual void ResizeBuffers();
		// update DPI dependent elements, such as GUI elements, sprites
		virtual void ResizeLayout();

		void Update(float dt) override;
		void FixedUpdate() override;
		void PreRender() override;
		void Render() const override;
		void Compose(pf::graphics::CommandList cmd) const override;

		virtual void setMSAASampleCount(uint32_t value) { msaaSampleCount = value; }
		constexpr uint32_t getMSAASampleCount() const { return msaaSampleCount; }

		virtual void setMSAASampleCount2D(uint32_t value) { msaaSampleCount2D = value; }
		constexpr uint32_t getMSAASampleCount2D() const { return msaaSampleCount2D; }

		const pf::graphics::Texture& GetRenderResult() const { return rtFinal; }
		virtual const pf::graphics::Texture* GetDepthStencil() const { return nullptr; }
		virtual const pf::graphics::Texture* GetGUIBlurredBackground() const { return nullptr; }

		void AddSprite(pf::Sprite* sprite, const std::string& layer = "");
		void RemoveSprite(pf::Sprite* sprite);
		void ClearSprites();
		int GetSpriteOrder(pf::Sprite* sprite);

		void AddFont(pf::SpriteFont* font, const std::string& layer = "");
		void RemoveFont(pf::SpriteFont* font);
		void ClearFonts();
		int GetFontOrder(pf::SpriteFont* font);

		void AddVideoSprite(pf::video::VideoInstance* videoinstance, pf::Sprite* sprite, const std::string& layer = "");

		struct RenderItem2D
		{
			pf::Sprite* sprite = nullptr;
			pf::SpriteFont* font = nullptr;
			pf::video::VideoInstance* videoinstance = nullptr;
			int order = 0;
		};
		struct RenderLayer2D
		{
			pf::vector<RenderItem2D> items;
			std::string name;
			int order = 0;
		};
		pf::vector<RenderLayer2D> layers{ 1 };
		void AddLayer(const std::string& name);
		void SetLayerOrder(const std::string& name, int order);
		void SetSpriteOrder(pf::Sprite* sprite, int order);
		void SetFontOrder(pf::SpriteFont* font, int order);
		void SortLayers();
		void CleanLayers();

		const pf::gui::GUI& GetGUI() const { return GUI; }
		pf::gui::GUI& GetGUI() { return GUI; }

		float resolutionScale = 1.0f;
		XMUINT2 GetInternalResolution() const
		{
			return XMUINT2(
				uint32_t((float)GetPhysicalWidth() * resolutionScale),
				uint32_t((float)GetPhysicalHeight() * resolutionScale)
			);
		}

		float GetHDRScaling() const { return hdr_scaling; }
		void SetHDRScaling(float value) { hdr_scaling = value; }
	};

}
