#pragma once
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

namespace gl
{
	//TODO
	class attribute
	{
		int m_index = 0;

	public:
		void set_index(int index)
		{
			m_index = index;
		}

		int index() const
		{
			return m_index;
		}

		void set_array(int size, int type, int stride, const void *pointer, bool normalized = false)
		{
			glEnableVertexAttribArray(m_index);
			glVertexAttribPointer(m_index, size, type, normalized, stride, pointer);
		}
	};

	//TODO
	class attributes
	{
		mutable std::unordered_map<int, attribute> m_attributes;

	public:
		attribute& operator[](int index) const
		{
			auto& res = m_attributes[index];
			res.set_index(index);
			return res;
		}

		void clear()
		{
			m_attributes.clear();
		}
	};

	class gpu_program
	{
	public:
		enum class target
		{
			fragment = GL_FRAGMENT_PROGRAM_ARB,
			vertex = GL_VERTEX_PROGRAM_ARB,
			geometry = GL_GEOMETRY_PROGRAM_NV
		};

	private:
		GLuint m_id = GL_NONE;
		target m_target = target::vertex;

	public:
		class save_binding_state
		{
			GLint m_last_binding;
			target m_target;

		public:
			save_binding_state(const gpu_program& new_binding)
			{
				//GLenum pname;
				//switch (m_target = new_binding.get_target())
				//{
				//case target::fragment: pname = GL_FRAGMENT_PROGRAM_BINDING_NV; break;
				//case target::vertex: pname = GL_VERTEX_PROGRAM_BINDING_NV; break;
				//case target::geometry:
				//	//pname = GL_GEOMETRY_PROGRAM_BINDING_NV;
				//	m_last_binding = new_binding.id();
				//	return;
				//}

				//glGetIntegerv(pname, &m_last_binding);
				new_binding.bind();
			}

			~save_binding_state()
			{
				//glBindBuffer((GLenum)m_target, m_last_binding);
			}
		};

		struct matrices_t
		{
			class matrix_t
			{
				GLenum m_mode;

			public:
				matrix_t(GLenum mode) : m_mode(mode)
				{
				}

				void operator = (const glm::mat4& rhs) { glMatrixMode(m_mode); glLoadMatrixf(glm::value_ptr(rhs)); }
				void operator = (const glm::dmat4& rhs) { glMatrixMode(m_mode); glLoadMatrixd(glm::value_ptr(rhs)); }
			};

			matrix_t operator[](GLenum index) const
			{
				return{ GL_MATRIX0_ARB + index };
			}
		};

		matrices_t matrix;
		attributes attrib;
		//local
		//env

		gpu_program() = default;
		gpu_program(GLuint id)
		{
			set_id(id);
		}

		gpu_program(target target_)
		{
			create(target_);
		}

		gpu_program(target target_, const std::string& source)
		{
			create(target_, source);
		}

		void bind() const
		{
			glBindProgramARB((GLenum)m_target, id());
		}

		void create(target target_)
		{
			if (created())
			{
				remove();
			}

			m_target = target_;
			glGenProgramsARB(1, &m_id);
		}

		void program_string(const std::string& src)
		{
			save_binding_state save(*this);
			glProgramStringARB((GLenum)m_target, GL_PROGRAM_FORMAT_ASCII_ARB, (GLsizei)src.length(), src.c_str());
		}

		void create(target target_, const std::string& source)
		{
			create(target_);
			program_string(source);
		}

		static std::string error()
		{
			return (const char*)glGetString(GL_PROGRAM_ERROR_STRING_ARB);
		}

		static void enable(target target_, bool enable = true)
		{
			if (enable)
				glEnable((GLenum)target_);
			else
				glDisable((GLenum)target_);
		}

		static void disable(target target_)
		{
			enable(target_, false);
		}

		void remove()
		{
			glDeleteProgramsARB(1, &m_id);
			m_id = GL_NONE;
		}

		GLuint id() const
		{
			return m_id;
		}

		target get_target() const
		{
			return m_target;
		}

		void set_id(GLuint id)
		{
			m_id = id;
		}

		bool created() const
		{
			return m_id != 0;
		}

		explicit operator bool() const
		{
			return created();
		}
	};

	struct gpu4_program_context
	{
		class mask_t
		{
			std::list<std::string> swizzles;

		public:
			size_t get_vector_size() const
			{
				return swizzles.back().length();
			}

			std::string get_mask() const
			{
				return fmt::merge(swizzles, ".");
			}

			mask_t& symplify()
			{
				if (swizzles.size() < 2)
					return *this;

				std::unordered_map<char, char> swizzle;

				static std::unordered_map<int, char> pos_to_swizzle =
				{
					{ 0, 'x' },
					{ 1, 'y' },
					{ 2, 'z' },
					{ 3, 'w' }
				};

				//if (!swizzles.empty())
				{
					auto it = swizzles.begin();

					const std::string& sw_front = *it;

					for (auto &i : pos_to_swizzle)
					{
						swizzle[i.second] = sw_front.length() > i.first ? sw_front[i.first] : 0;
					}

					for (++it; it != swizzles.end(); ++it)
					{
						std::unordered_map<char, char> new_swizzle;

						for (auto &sw : pos_to_swizzle)
						{
							new_swizzle[sw.second] = swizzle[it->length() <= sw.first ? '\0' : (*it)[sw.first]];
						}

						swizzle = new_swizzle;
					}
				}

				swizzles.clear();
				std::string new_swizzle;

				for (auto &i : pos_to_swizzle)
				{
					if (swizzle[i.second] != '\0')
						new_swizzle += swizzle[i.second];
				}

				swizzles.push_back(new_swizzle);

				return *this;
			}

			std::string get() const
			{
				if (swizzles.empty() || (swizzles.size() == 1 && swizzles.front() == "xyzw"))
				{
					return{};
				}

				return "." + get_mask();
			}

			mask_t& append(const std::string& mask)
			{
				swizzles.emplace_back(mask);
				return *this;
			}

			mask_t& append_front(const std::string& mask)
			{
				swizzles.emplace_front(mask);
				return *this;
			}

			mask_t& clear()
			{
				swizzles.clear();
				return *this;
			}
		};

		struct type_info
		{
			std::string name;
		};

		struct variable_info
		{
			type_info type;
			std::string name;
			std::string initialization;
		};

		struct predeclared_variable_info
		{
			std::string name;
		};

		struct constant_info
		{
			f32 value_f32[4];
		};

		struct argument
		{
			enum class type_t
			{
				predeclared_variable,
				variable,
				constant,
				label
			};

			type_t type;
			int index = -1;

		private:
			mask_t m_mask;
			bool m_neg = false;
			bool m_abs = false;

		public:
			argument& mask(mask_t mask) { m_mask = mask; return *this; }
			argument& mask(const std::string& mask) { m_mask.append(mask); return *this; }
			argument& neg(bool value = true) { m_neg = value; return *this; }
			argument& abs(bool value = true) { m_abs = value; return *this; }

			const mask_t& get_mask() const { return m_mask; }
			bool is_neg() const { return m_neg; }
			bool is_abs() const { return m_abs; }

			explicit operator bool() const
			{
				return index >= 0;
			}
		};

		struct operation
		{
			std::string label;
			std::string instruction;
			std::vector<argument> args;
		};

		std::string header;
		std::vector<std::string> extensions;
		std::vector<variable_info> variables;
		std::vector<predeclared_variable_info> predeclared_variables;
		std::vector<constant_info> constants;
		std::vector<operation> operations;

		std::string get(const argument& arg)
		{
			if (!arg)
				return{};

			std::string result;

			if (arg.is_neg())
				result += "-";

			if (arg.is_abs())
				result += "|";

			mask_t mask = arg.get_mask();

			switch (arg.type)
			{
			case argument::type_t::constant:
			{
				constant_info &info = constants[arg.index];

				if (info.value_f32[0] == info.value_f32[1] && info.value_f32[1] == info.value_f32[2] && info.value_f32[2] == info.value_f32[3])
				{
					result += "{" + std::to_string(info.value_f32[0]) + "}";
					mask.append_front("xxxx");
				}
				else if (info.value_f32[0] == info.value_f32[1] && info.value_f32[2] == info.value_f32[3])
				{
					result += "{" + std::to_string(info.value_f32[0]) + ", " + std::to_string(info.value_f32[0]) + "}";
					mask.append_front("xxyy");
				}
				else
				{
					result +=
						"{" +
						std::to_string(info.value_f32[0]) + "," +
						std::to_string(info.value_f32[1]) + "," +
						std::to_string(info.value_f32[2]) + "," +
						std::to_string(info.value_f32[3])
						+ "}";
				}
			}
			break;

			case argument::type_t::variable:
			{
				variable_info &info = variables[arg.index];

				result += info.name;
			}
			break;

			case argument::type_t::predeclared_variable:
			{
				predeclared_variable_info &info = predeclared_variables[arg.index];

				result += info.name;
			}
			break;

			case argument::type_t::label:
			{
				auto &info = operations[arg.index];

				if (info.label.empty())
				{
					throw std::runtime_error("bad label");
				}

				result += info.label;
			}
			break;
			}

			result += mask.symplify().get();

			if (arg.is_abs())
				result += "|";

			return result;
		}

		std::string make()
		{
			std::string result = header + "\n";

			result += "\n#extensions\n";
			for (auto &extension : extensions)
				result += "\tOPTION " + extension + ";\n";

			result += "\n#declarations\n";
			for (auto &variable : variables)
			{
				result += "\t" + variable.type.name + " " + variable.name +
					(variable.initialization.empty() ? std::string{} : " = " + variable.initialization)
					+ ";\n";
			}

			result += "\n#code\n";
			for (auto &operation : operations)
			{
				if (!operation.label.empty())
				{
					result += operation.label + ":\n";
				}

				result += "\t" + operation.instruction + " ";

				bool is_first = true;
				for (auto &arg : operation.args)
				{
					if (is_first)
					{
						is_first = false;
					}
					else
					{
						result += ", ";
					}

					result += get(arg);
				}

				result += ";\n";
			}

			return result + "END";
		}
	};

	template<typename _context_t = gpu4_program_context>
	struct gpu_program_builder
	{
		typedef _context_t context_t;
		context_t context;

		struct predeclared_variable_t : context_t::argument
		{
		};

		struct variable_t : context_t::argument
		{
		};

		struct constant_t : context_t::argument
		{
		};

		struct label_t : context_t::argument
		{
		};

		std::unordered_map<std::string, int> predeclared_variables;
		std::unordered_map<std::string, int> variables;
		std::unordered_map<std::string, int> constants;
		std::unordered_map<int, std::string> labels;

		variable_t variable(const std::string& type, const std::string& name, const std::string& initialization = std::string{})
		{
			auto finded = variables.find(name);
			int index;
			if (finded == variables.end())
			{
				context_t::variable_info info;
				info.type.name = type;
				info.name = name;
				info.initialization = initialization;

				index = (int)context.variables.size();
				context.variables.push_back(info);
				variables[name] = index;
			}
			else
			{
				index = finded->second;
			}

			variable_t result;
			result.type = context_t::argument::type_t::variable;
			result.index = index;

			return result;
		}

		predeclared_variable_t predeclared_variable(const std::string& name)
		{
			auto finded = predeclared_variables.find(name);
			int index;
			if (finded == predeclared_variables.end())
			{
				context_t::predeclared_variable_info info;
				info.name = name;
				index = (int)context.predeclared_variables.size();
				context.predeclared_variables.push_back(info);
				predeclared_variables[name] = index;
			}
			else
			{
				index = finded->second;
			}

			predeclared_variable_t result;
			result.type = context_t::argument::type_t::predeclared_variable;
			result.index = index;

			return result;
		}

		constant_t constant(f32 x, f32 y, f32 z, f32 w)
		{
			const auto& name = fmt::format("%08X%08X%08X%08X", (u32&)x, (u32&)y, (u32&)z, (u32&)w);
			auto finded = constants.find(name);
			int index;
			if (finded == constants.end())
			{
				context_t::constant_info info;
				info.value_f32[0] = x;
				info.value_f32[1] = y;
				info.value_f32[2] = z;
				info.value_f32[3] = w;

				index = (int)context.constants.size();
				context.constants.push_back(info);
				constants[name] = index;
			}
			else
			{
				index = finded->second;
			}

			constant_t result;
			result.type = context_t::argument::type_t::constant;
			result.index = index;

			return result;
		}

		constant_t constant(f32 x, f32 y)
		{
			return constant(x, x, y, y);
		}

		constant_t constant(f32 x)
		{
			return constant(x, x);
		}

		label_t label(int index, const std::string& name)
		{
			auto finded = labels.find(index);
			if (finded == labels.end())
			{
				labels[index] = name;
			}

			label_t result;
			result.type = context_t::argument::type_t::label;
			result.index = index;

			return result;
		}

		void op(const std::string& instruction)
		{
			context_t::operation operation;
			operation.instruction = instruction;
			context.operations.push_back(operation);
		}

		template<typename ...T>
		void op(const std::string& instruction, T... args)
		{
			context_t::operation operation;
			operation.instruction = instruction;
			operation.args = std::vector<context_t::argument> { static_cast<context_t::argument>(args)...};
			context.operations.push_back(operation);
		}

		void link()
		{
			//initialize labels
			for (auto &label : labels)
			{
				context.operations[label.first].label = label.second;
			}
		}
	};
}