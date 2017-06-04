#include "log.h"
#include "config.h"
#include <common/file.h>
#include <common/fmt.h>

#include <iostream>
#include <cinttypes>
#include "thread.h"

#include <rpcs3/version.h>

namespace rpcs3
{
	inline namespace core
	{
		namespace log
		{
			std::unique_ptr<manager> g_manager;

			u32 message::size() const
			{
				//1 byte for NULL terminator
				return (u32)(sizeof(message::size_type) + sizeof(log::type) + sizeof(log::severity) + sizeof(std::string::value_type) * text.size() + 1);
			}

			void message::serialize(char *output) const
			{
				message::size_type size = this->size();
				memcpy(output, &size, sizeof(message::size_type));
				output += sizeof(message::size_type);
				memcpy(output, &type, sizeof(log::type));
				output += sizeof(log::type);
				memcpy(output, &serverity, sizeof(log::severity));
				output += sizeof(log::severity);
				memcpy(output, text.c_str(), text.size());
				output += sizeof(std::string::value_type) * text.size();
				*output = '\0';

			}
			message message::deserialize(char *input, u32* size_out)
			{
				message msg;
				message::size_type msg_size = *(reinterpret_cast<message::size_type*>(input));
				input += sizeof(message::size_type);
				msg.type = *(reinterpret_cast<log::type*>(input));
				input += sizeof(log::type);
				msg.serverity = *(reinterpret_cast<log::severity*>(input));
				input += sizeof(log::severity);

				if (msg_size > 9000)
				{
					//int wtf = 6;
				}

				msg.text.append(input, msg_size - 1 - sizeof(severity) - sizeof(log::type));
				if (size_out)
				{
					*size_out = msg_size;
				}
				return msg;
			}

			channel::channel() : channel("unknown")
			{}

			channel::channel(const std::string& name) :
				name(name)
				, m_enabled(true)
				, m_log_level(severity::warning)
			{}

			void channel::log(const message &msg)
			{
				std::lock_guard<std::mutex> lock(m_mtx_listener);
				for (auto &listener : m_listeners)
				{
					listener->log(msg);
				}
			}

			void channel::add_listener(std::shared_ptr<listener> listener)
			{
				std::lock_guard<std::mutex> lock(m_mtx_listener);
				m_listeners.insert(listener);
			}
			void channel::remove_listener(std::shared_ptr<listener> listener)
			{
				std::lock_guard<std::mutex> lock(m_mtx_listener);
				m_listeners.erase(listener);
			}

			struct cout_listener : listener
			{
				void log(const message &msg) override
				{
					std::cerr << msg.text << std::endl;
				}
			};

			struct file_listener : listener
			{
				fs::file file;
				bool prepend_channel_name;

				file_listener(const std::string& name = rpcs3::name() + std::string(".log"), bool prependChannel = true)
					: file(rpcs3::config.system.log_path.value() + name, fom::rewrite)
					, prepend_channel_name(prependChannel)
				{
					if (!file)
					{
						std::cout << "Can't create log file! (" + name + ")";
					}
				}

				void log(const message &msg) override
				{
					std::string text = msg.text;
					if (prepend_channel_name)
					{
						text.insert(0, g_type_name_table[static_cast<u32>(msg.type)].name);

						if (msg.type == log::type::tty)
						{
							text = fmt::escape(text);
							if (text[text.length() - 1] != '\n')
							{
								text += '\n';
							}
						}
					}

					file << text;
				}
			};

			manager::manager()
#ifdef BUFFERED_LOGGING
				: m_exiting(false), m_log_consumer()
#endif
			{
				auto it = m_channels.begin();
				std::shared_ptr<listener> listener(new file_listener());
				for (const log::type_name& type_name : g_type_name_table)
				{
					it->name = type_name.name;
					it->add_listener(listener);
					it++;
				}
				std::shared_ptr<log::listener> tty_listener(new file_listener("TTY", false));
				channel(log::type::tty).add_listener(tty_listener);
#ifdef BUFFERED_LOGGING
				m_log_consumer = std::thread(&manager::consume_log, this);
#endif
			}

			manager::~manager()
			{
#ifdef BUFFERED_LOGGING
				m_exiting = true;
				m_buffer_ready.notify_all();
				m_log_consumer.join();
			}

			void manager::consume_log()
			{
				std::unique_lock<std::mutex> lock(m_mtx_status);
				while (!m_exiting)
				{
					m_buffer_ready.wait(lock);
					m_buffer.lock_get();
					size_t size = m_buffer.size();
					std::vector<char> local_messages(size);
					m_buffer.pop(&local_messages.front(), size);
					m_buffer.unlock_get();

					u32 cursor = 0;
					u32 removed = 0;
					while (cursor < size)
					{
						log::message msg = log::message::deserialize(local_messages.data() + cursor, &removed);
						cursor += removed;
						channel(msg.type).log(msg);
					}
				}
#endif
			}

			void manager::log(message msg)
			{
				//don't do any formatting changes or filtering to the TTY output since we
				//use the raw output to do diffs with the output of a real PS3 and some
				//programs write text in single bytes to the console
				if (msg.type != log::type::tty)
				{
					std::string prefix;
					switch (msg.serverity)
					{
					case severity::success:
						prefix = "S ";
						break;
					case severity::notice:
						prefix = "! ";
						break;
					case severity::warning:
						prefix = "W ";
						break;
					case severity::error:
						prefix = "E ";
						break;
					}
					if (auto thr = get_current_thread_ctrl())
					{
						prefix += "{" + thr->get_name() + "} ";
					}
					msg.text.insert(0, prefix);
					msg.text.append(1, '\n');
				}
#ifdef BUFFERED_LOGGING
				size_t size = msg.size();
				std::vector<char> temp_buffer(size);
				msg.serialize(temp_buffer.data());
				m_buffer.push(temp_buffer.begin(), temp_buffer.end());
				m_buffer_ready.notify_one();
#else
				m_channels[static_cast<u32>(msg.type)].log(msg);
#endif
			}

			void manager::add_listener(std::shared_ptr<listener> listener)
			{
				for (auto& channel : m_channels)
				{
					channel.add_listener(listener);
				}
			}

			void manager::remove_listener(std::shared_ptr<listener> listener)
			{
				for (auto& channel : m_channels)
				{
					channel.remove_listener(listener);
				}
			}

			manager& manager::instance()
			{
				if (!g_manager)
				{
					g_manager.reset(new manager());
				}

				return *g_manager;
			}

			channel &manager::channel(log::type type)
			{
				return m_channels[static_cast<u32>(type)];
			}
		}

		void log_message(log::type type, log::severity sev, const char* text)
		{
			log_message(type, sev, std::string(text));
		}

		void log_message(log::type type, log::severity sev, std::string text)
		{
			if (log::g_manager)
			{
				// another msvc bug makes this not work, uncomment this when it's fixed
				//g_manager->log({logType, severity, text});
				log::message msg{ type, sev, std::move(text) };
				log::g_manager->log(msg);
			}
			else
			{
				std::string severity =
					sev == log::severity::notice ? "Notice" :
					sev == log::severity::warning ? "Warning" :
					sev == log::severity::success ? "Success" :
					sev == log::severity::error ? "Error" : "Unknown";

				std::cout << severity << ": " << text;
			}
		}
	}
}