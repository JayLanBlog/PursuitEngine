#include "backlogger.h"
#include <deque>
#include <mutex>
#include <Engine/Component/font.h>
#include <Engine/Ecore/draw_image.h>
#include "Engine/Component/pgui.h"
#include "Engine/Device/vulkan_driver.h"
#include <Module/Util/input.h>

using namespace pf::graphics;
//using namespace pf::gui::ScrollBar;
namespace pf {

	namespace backlogger {
		std::string logfile_path = "";
		LogLevel logLevel = LogLevel::Default;

		const float speed = 4000.0f;
		const size_t deletefromline = 500;
		bool refitscroll = false;
		LogLevel unseen = LogLevel::None;
		std::mutex historyLock;
		std::deque<LogEntry> history;
		float pos = 5;
		float scroll = 0;
		int historyPos = 0;
		bool locked = false;
		bool enabled = false;
		bool was_ever_enabled = enabled;
		bool blockLuaExec = false;
		pf::font::Params font_params;
		pf::gui::Button toggleButton;
		pf::gui::TextInputField inputField;		
		Texture backgroundTex;
		pf::Color backgroundColor = pf::Color(17, 30, 43, 255);

		struct InternalState {
			// These must have common lifetime and destruction order, so keep them together in a struct:
			std::deque<LogEntry> entries;
			std::mutex entriesLock;
			std::string getText()
			{
				std::scoped_lock lck(entriesLock);
				std::string retval;
				_forEachLogEntry_unsafe([&](auto&& entry) {retval += entry.text; });
				return retval;
			}

			inline void _forEachLogEntry_unsafe(std::function<void(const LogEntry&)> cb)
			{
				for (auto& entry : entries)
				{
					cb(entry);
				}
			}

			void writeLogfile() {
				std::string filename;

				if (logfile_path.empty() || !pf::helper::DirectoryExists(pf::helper::GetDirectoryFromPath(logfile_path)))
				{
					filename = pf::helper::GetCurrentPath() + "/log.txt";
				}
				else
				{
					filename = logfile_path;
				}
				std::string text = getText();
				static std::mutex writelocker;
				std::scoped_lock lck(writelocker); // to not write the logfile from multiple threads
				pf::helper::FileWrite(filename, (const uint8_t*)text.c_str(), text.length());
			}

			~InternalState()
			{
				// The object will automatically write out the backlog to the temp folder when it's destroyed
				//	Should happen on application exit
				writeLogfile();
			}
		} internal_state;

		std::string getText() {
			return internal_state.getText();
		}
	
		void postin(const char* input, LogLevel level ) {
			if (logLevel > level)
			{
				return;
			}

			std::string str;
			switch (level)
			{
			default:
			case LogLevel::Default:
				str = "[Info] ";
				break;
			case LogLevel::Warning:
				str = "[Warning] ";
				break;
			case LogLevel::Error:
				str = "[Error] ";
				break;
			}

			str += input;
			str += '\n';

			LogEntry entry;
			entry.text = str;
			entry.level = level;

			internal_state.entriesLock.lock();
			internal_state.entries.push_back(entry);
			if (internal_state.entries.size() > deletefromline)
			{
				internal_state.entries.pop_front();
			}
			internal_state.entriesLock.unlock();

			refitscroll = true;

			switch (level)
			{
			default:
			case LogLevel::Default:
				pf::helper::DebugOut(str, pf::helper::DebugLevel::Normal);
				break;
			case LogLevel::Warning:
				pf::helper::DebugOut(str, pf::helper::DebugLevel::Warning);
				break;
			case LogLevel::Error:
				pf::helper::DebugOut(str, pf::helper::DebugLevel::Error);
				break;
			}

			unseen = std::max(unseen, level);

			if (level >= LogLevel::Error)
			{
				internal_state.writeLogfile();  // will lock mutex
			}

		}

		void SetLogLevel(LogLevel newLevel)
		{
			logLevel = newLevel;
		}

		void setFontRowspacing(float value)
		{
			font_params.spacingY = value;
		}

		void setFontSize(int value)
		{
			font_params.size = value;
		}

		LogLevel GetUnseenLogLevelMax()
		{
			return unseen;
		}

		void DrawOutputText(
			const pf::Canvas& canvas,
			CommandList cmd,
			ColorSpace colorspace
		)
		{
			pf::font::SetCanvas(canvas); // always set here as it can be called from outside...
			pf::font::Params params = font_params;
			params.cursor = {};
			if (refitscroll)
			{
				float textheight = pf::font::TextHeight(getText(), params);
				float limit = canvas.GetLogicalHeight() - 50;
				if (scroll + textheight > limit)
				{
					scroll = limit - textheight;
				}
				refitscroll = false;
			}
			params.posX = 5;
			params.posY = pos + scroll;
			params.h_wrap = canvas.GetLogicalWidth() - params.posX;
			if (colorspace != ColorSpace::SRGB)
			{
				params.enableLinearOutputMapping(9);
			}

			static std::deque<LogEntry> entriesCopy;

			internal_state.entriesLock.lock();
			// Force copy because drawing text while locking is not safe because an error inside might try to lock again!
			entriesCopy = internal_state.entries;
			internal_state.entriesLock.unlock();

			for (auto& x : entriesCopy)
			{
				switch (x.level)
				{
				case LogLevel::Warning:
					params.color = pf::Color::Warning();
					break;
				case LogLevel::Error:
					params.color = pf::Color::Error();
					break;
				default:
					params.color = font_params.color;
					break;
				}
				params.cursor = pf::font::Draw(x.text, params, cmd);
			}

			unseen = LogLevel::None;
		}

		void Toggle()
		{
			enabled = !enabled;
			was_ever_enabled = true;
		}

		void Scroll(float dir)
		{
			scroll += dir;
		}


		void historyPrev()
		{
			std::scoped_lock lock(historyLock);
			if (!history.empty())
			{
				inputField.SetText(history[history.size() - 1 - historyPos].text);
				inputField.SetAsActive();
				if ((size_t)historyPos < history.size() - 1)
				{
					historyPos++;
				}
			}
		}

		void Update(const pf::Canvas& canvas, float dt) {
			if (!locked)
			{
				if (pf::input::Press(pf::input::KEYBOARD_BUTTON_HOME))
				{
					Toggle();
				}

				if (isActive())
				{
					if (pf::input::Press(pf::input::KEYBOARD_BUTTON_UP))
					{
						historyPrev();
					}
					if (pf::input::Press(pf::input::KEYBOARD_BUTTON_DOWN))
					{
						historyNext();
					}
					if (pf::input::Down(pf::input::KEYBOARD_BUTTON_PAGEUP))
					{
						Scroll(1000.0f * dt);
					}
					if (pf::input::Down(pf::input::KEYBOARD_BUTTON_PAGEDOWN))
					{
						Scroll(-1000.0f * dt);
					}

					Scroll(pf::input::GetPointer().z * 20);

					static bool created = false;
					if (!created)
					{
						created = true;
						inputField.Create("");
						inputField.SetCancelInputEnabled(false);
						inputField.OnInputAccepted([](pf::gui::EventArgs args) {
							historyPos = 0;
							postin(args.sValue);
							LogEntry entry;
							entry.text = args.sValue;
							entry.level = LogLevel::Default;
							{
								std::scoped_lock lock(historyLock);
								history.push_back(entry);
								if (history.size() > deletefromline)
								{
									history.pop_front();
								}
							}
							if (!blockLuaExec)
							{
								Luaer::RunText(args.sValue);
							}
							else
							{
								postin("Lua execution is disabled", LogLevel::Error);
							}
							inputField.SetText("");
							});
						pf::Color theme_color_idle = pf::Color(30, 40, 60, 200);
						pf::Color theme_color_focus = pf::Color(70, 150, 170, 220);
						pf::Color theme_color_active = pf::Color::White();
						pf::Color theme_color_deactivating = pf::Color::lerp(theme_color_focus, pf::Color::White(), 0.5f);
						inputField.SetColor(theme_color_idle); // all states the same, it's gonna be always active anyway
						inputField.font.params.color = pf::Color(160, 240, 250, 255);
						inputField.font.params.shadowColor = pf::Color::Transparent();

						toggleButton.Create("V");
						toggleButton.OnClick([](pf::gui::EventArgs args) {
							Toggle();
							});
						toggleButton.SetColor(theme_color_idle, pf::gui::IDLE);
						toggleButton.SetColor(theme_color_focus, pf::gui::FOCUS);
						toggleButton.SetColor(theme_color_active, pf::gui::ACTIVE);
						toggleButton.SetColor(theme_color_deactivating, pf::gui::DEACTIVATING);
						toggleButton.SetShadowRadius(5);
						toggleButton.SetShadowColor(pf::Color(80, 140, 180, 100));
						toggleButton.font.params.color = pf::Color(160, 240, 250, 255);
						toggleButton.font.params.rotation = XM_PI;
						toggleButton.font.params.size = 24;
						toggleButton.font.params.scaling = 3;
						toggleButton.font.params.shadowColor = pf::Color::Transparent();
						for (int i = 0; i < arraysize(toggleButton.sprites); ++i)
						{
							toggleButton.sprites[i].params.enableCornerRounding();
							toggleButton.sprites[i].params.corners_rounding[2].radius = 50;
						}
					}
					if (inputField.GetState() != pf::gui::ACTIVE)
					{
						inputField.SetAsActive();
					}

				}
				else
				{
					inputField.Deactivate();
				}
			}

			if (enabled)
			{
				pos += speed * dt;
			}
			else
			{
				pos -= speed * dt;
			}
			pos = pf::math::Clamp(pos, -canvas.GetLogicalHeight(), 0);

			inputField.SetSize(XMFLOAT2(canvas.GetLogicalWidth() - 40, 20));
			inputField.SetPos(XMFLOAT2(20, canvas.GetLogicalHeight() - 40 + pos));
			inputField.Update(canvas, dt);

			toggleButton.SetSize(XMFLOAT2(100, 100));
			toggleButton.SetPos(XMFLOAT2(canvas.GetLogicalWidth() - toggleButton.GetSize().x - 20, 20 + pos));
			toggleButton.Update(canvas, dt);
		}

		void Draw(
			const pf::Canvas& canvas,
			pf::graphics::CommandList cmd,
			pf::graphics::ColorSpace colorspace
		) {
			if (!was_ever_enabled)
				return;
			if (pos <= -canvas.GetLogicalHeight())
				return;

			GraphicsDevice* device = GetDevice();
			device->EventBegin("Backlog", cmd);

			pf::image::Params fx = pf::image::Params((float)canvas.GetLogicalWidth(), (float)canvas.GetLogicalHeight());
			fx.pos = XMFLOAT3(0, pos, 0);
			fx.opacity = pf::math::Lerp(0.9f, 0, saturate(-pos / canvas.GetLogicalHeight()));
			if (colorspace != ColorSpace::SRGB)
			{
				fx.enableLinearOutputMapping(9);
			}
			fx.color = backgroundColor;
			pf::image::Draw(backgroundTex.IsValid() ? &backgroundTex : nullptr, fx, cmd);

			pf::image::Params inputbg;
			inputbg.color = pf::Color(80, 140, 180, 200);
			inputbg.pos = inputField.translation;
			inputbg.pos.x -= 8;
			inputbg.pos.y -= 8;
			inputbg.siz = inputField.GetSize();
			inputbg.siz.x += 16;
			inputbg.siz.y += 16;
			inputbg.enableCornerRounding();
			inputbg.corners_rounding[0].radius = 10;
			inputbg.corners_rounding[1].radius = 10;
			inputbg.corners_rounding[2].radius = 10;
			inputbg.corners_rounding[3].radius = 10;
			if (colorspace != ColorSpace::SRGB)
			{
				inputbg.enableLinearOutputMapping(9);
			}
			pf::image::Draw(nullptr, inputbg, cmd);

			if (colorspace != ColorSpace::SRGB)
			{
				inputField.sprites[inputField.GetState()].params.enableLinearOutputMapping(9);
				inputField.font.params.enableLinearOutputMapping(9);
				toggleButton.sprites[inputField.GetState()].params.enableLinearOutputMapping(9);
				toggleButton.font.params.enableLinearOutputMapping(9);
			}
			inputField.Render(canvas, cmd);

			Rect rect;
			rect.left = 0;
			rect.right = (int32_t)canvas.GetPhysicalWidth();
			rect.top = 0;
			rect.bottom = (int32_t)canvas.GetPhysicalHeight();
			device->BindScissorRects(1, &rect, cmd);

			toggleButton.Render(canvas, cmd);

			rect.bottom = int32_t(canvas.LogicalToPhysical(inputField.GetPos().y - 15));
			device->BindScissorRects(1, &rect, cmd);

			DrawOutputText(canvas, cmd, colorspace);

			rect.left = 0;
			rect.right = std::numeric_limits<int>::max();
			rect.top = 0;
			rect.bottom = std::numeric_limits<int>::max();
			device->BindScissorRects(1, &rect, cmd);
			device->EventEnd(cmd);
		}


		void postin(const std::string& input, LogLevel level ) {
			postin(input.c_str(), level);
		}

	
		void historyNext() {
			std::scoped_lock lock(historyLock);
			if (!history.empty())
			{
				if (historyPos > 0)
				{
					historyPos--;
				}
				inputField.SetText(history[history.size() - 1 - historyPos].text);
				inputField.SetAsActive();
			}
		}

		bool isActive() {
			return enabled;
		}
		void clear()
		{
			std::scoped_lock lck(internal_state.entriesLock);
			internal_state.entries.clear();
			scroll = 0;
		}

		void Lock() {
			locked = true;
			enabled = false;
		}

		void Unlock() {
			locked = false;
		}

		void BlockLuaExecution() 
		{
			blockLuaExec = true;
		}

		void UnblockLuaExecution() {
			blockLuaExec = false;
		}

		void SaveLogToFile(const std::string& path) {
			logfile_path = path;
		}

		// Use getText() instead, unless absolutely necessary.
		void _forEachLogEntry_unsafe(std::function<void(const LogEntry&)> cb) {
			internal_state._forEachLogEntry_unsafe(cb);
		}
	}
}

