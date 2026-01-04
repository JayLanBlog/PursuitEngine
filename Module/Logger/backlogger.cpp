#include "backlogger.h"
#include <deque>
#include <mutex>

namespace pf {

	namespace backlogger {
		std::string logfile_path = "";
		LogLevel logLevel = LogLevel::Default;
		const size_t deletefromline = 500;
		bool refitscroll = false;
		LogLevel unseen = LogLevel::None;
		std::mutex historyLock;
		std::deque<LogEntry> history;
		int historyPos = 0;
		bool locked = false;
		bool enabled = false;
		bool was_ever_enabled = enabled;
		bool blockLuaExec = false;

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

		void postin(const std::string& input, LogLevel level ) {
			postin(input.c_str(), level);
		}

		void historyPre() {
			std::scoped_lock lock(historyLock);
			if (!history.empty())
			{
				//inputField.SetText(history[history.size() - 1 - historyPos].text);
				//inputField.SetAsActive();
				if ((size_t)historyPos < history.size() - 1)
				{
					historyPos++;
				}
			}
		}

		void historyNext() {
			std::scoped_lock lock(historyLock);
			if (!history.empty())
			{
				if (historyPos > 0)
				{
					historyPos--;
				}
				//inputField.SetText(history[history.size() - 1 - historyPos].text);
				//inputField.SetAsActive();
			}
		}

		bool isActive() {
			return enabled;
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

