#pragma once

#include <exception>
#include <string>
#include <functional>
#include <vector>
#include <memory>
#include <unordered_map>
#include <algorithm>

#include "OpenGL.h"
#include "../GCM.h"

#include "Utilities/geometry.h"

#define GL_FRAGMENT_TEXTURES_START 0
#define GL_VERTEX_TEXTURES_START   GL_FRAGMENT_TEXTURES_START + 16
#define GL_STENCIL_MIRRORS_START   GL_VERTEX_TEXTURES_START + 4
#define GL_STREAM_BUFFER_START     GL_STENCIL_MIRRORS_START + 16

inline static void _SelectTexture(int unit) { glActiveTexture(GL_TEXTURE0 + unit); }

namespace gl
{
#ifdef _DEBUG
	struct __glcheck_impl_t
	{
		const char* file;
		const char* function;
		int line;

		constexpr __glcheck_impl_t(const char* file, const char* function, int line)
			: file(file)
			, function(function)
			, line(line)
		{
		}

		~__glcheck_impl_t() noexcept(false)
		{
			if (GLenum err = glGetError())
			{
				std::string error;
				switch (err)
				{
				case GL_INVALID_OPERATION:      error = "invalid operation";      break;
				case GL_INVALID_ENUM:           error = "invalid enum";           break;
				case GL_INVALID_VALUE:          error = "invalid value";          break;
				case GL_OUT_OF_MEMORY:          error = "out of memory";          break;
				case GL_INVALID_FRAMEBUFFER_OPERATION:  error = "invalid framebuffer operation";  break;
				default: error = "unknown error"; break;
				}

				fmt::throw_exception("OpenGL error: %s\n(in file %s:%d, function %s)", error, file, line, function);
			}
		}
	};
#define __glcheck ::gl::__glcheck_impl_t{ __FILE__, __FUNCTION__, __LINE__ },
#else
#define __glcheck
#endif

	//Function call wrapped in ARB_DSA vs EXT_DSA compat check
#define DSA_CALL(func, texture_name, target, ...)\
	if (::gl::get_driver_caps().ARB_dsa_supported)\
		gl##func(texture_name, __VA_ARGS__);\
	else\
		gl##func##EXT(texture_name, target, __VA_ARGS__);

	class capabilities;
	class blitter;

	void enable_debugging();
	capabilities& get_driver_caps();
	bool is_primitive_native(rsx::primitive_type in);
	GLenum draw_mode(rsx::primitive_type in);

	// Texture helpers
	std::array<GLenum, 4> apply_swizzle_remap(const std::array<GLenum, 4>& swizzle_remap, const std::pair<std::array<u8, 4>, std::array<u8, 4>>& decoded_remap);

	class exception : public std::exception
	{
	protected:
		std::string m_what;

	public:
		const char* what() const noexcept override
		{
			return m_what.c_str();
		}
	};

	class capabilities
	{
	public:
		bool EXT_dsa_supported = false;
		bool ARB_dsa_supported = false;
		bool ARB_buffer_storage_supported = false;
		bool ARB_texture_buffer_supported = false;
		bool ARB_shader_draw_parameters_supported = false;
		bool ARB_depth_buffer_float_supported = false;
		bool ARB_texture_barrier_supported = false;
		bool NV_texture_barrier_supported = false;
		bool initialized = false;
		bool vendor_INTEL = false;  //has broken GLSL compiler
		bool vendor_AMD = false;    //has broken ARB_multidraw
		bool vendor_NVIDIA = false; //has NaN poisoning issues
		bool vendor_MESA = false;   //requires CLIENT_STORAGE bit set for streaming buffers

		void initialize()
		{
			int find_count = 8;
			int ext_count = 0;
			glGetIntegerv(GL_NUM_EXTENSIONS, &ext_count);

			for (int i = 0; i < ext_count; i++)
			{
				if (!find_count) break;

				const std::string ext_name = reinterpret_cast<const char*>(glGetStringi(GL_EXTENSIONS, i));

				if (ext_name == "GL_ARB_shader_draw_parameters")
				{
					ARB_shader_draw_parameters_supported = true;
					find_count--;
					continue;
				}

				if (ext_name == "GL_EXT_direct_state_access")
				{
					EXT_dsa_supported = true;
					find_count--;
					continue;
				}

				if (ext_name == "GL_ARB_direct_state_access")
				{
					ARB_dsa_supported = true;
					find_count--;
					continue;
				}

				if (ext_name == "GL_ARB_buffer_storage")
				{
					ARB_buffer_storage_supported = true;
					find_count--;
					continue;
				}

				if (ext_name == "GL_ARB_texture_buffer_object")
				{
					ARB_texture_buffer_supported = true;
					find_count--;
					continue;
				}

				if (ext_name == "GL_ARB_depth_buffer_float")
				{
					ARB_depth_buffer_float_supported = true;
					find_count--;
					continue;
				}

				if (ext_name == "GL_ARB_texture_barrier")
				{
					ARB_texture_barrier_supported = true;
					find_count--;
					continue;
				}

				if (ext_name == "GL_NV_texture_barrier")
				{
					NV_texture_barrier_supported = true;
					find_count--;
					continue;
				}
			}

			//Workaround for intel drivers which have terrible capability reporting
			std::string vendor_string = reinterpret_cast<const char*>(glGetString(GL_VENDOR));
			if (!vendor_string.empty())
			{
				std::transform(vendor_string.begin(), vendor_string.end(), vendor_string.begin(), ::tolower);
			}
			else
			{
				LOG_ERROR(RSX, "Failed to get vendor string from driver. Are we missing a context?");
				vendor_string = "intel"; //lowest acceptable value
			}

			if (vendor_string.find("intel") != std::string::npos)
			{
				int version_major = 0;
				int version_minor = 0;

				glGetIntegerv(GL_MAJOR_VERSION, &version_major);
				glGetIntegerv(GL_MINOR_VERSION, &version_minor);

				vendor_INTEL = true;

				//Texture buffers moved into core at GL 3.3
				if (version_major > 3 || (version_major == 3 && version_minor >= 3))
					ARB_texture_buffer_supported = true;

				//Check for expected library entry-points for some required functions
				if (!ARB_buffer_storage_supported && glBufferStorage && glMapBufferRange)
					ARB_buffer_storage_supported = true;

				if (!ARB_dsa_supported && glGetTextureImage && glTextureBufferRange)
					ARB_dsa_supported = true;

				if (!EXT_dsa_supported && glGetTextureImageEXT && glTextureBufferRangeEXT)
					EXT_dsa_supported = true;
			}
			else if (vendor_string.find("nvidia") != std::string::npos)
			{
				vendor_NVIDIA = true;
			}
			else if (vendor_string.find("x.org") != std::string::npos)
			{
				vendor_MESA = true;
			}
#ifdef _WIN32
			else if (vendor_string.find("amd") != std::string::npos || vendor_string.find("ati") != std::string::npos)
			{
				vendor_AMD = true;
			}
#endif

			initialized = true;
		}
	};

	class fence
	{
		GLsync m_value = nullptr;
		GLenum flags = GL_SYNC_FLUSH_COMMANDS_BIT;
		bool signaled = false;

	public:

		fence() {}
		~fence() {}

		void create()
		{
			m_value = glFenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE, 0);
			flags = GL_SYNC_FLUSH_COMMANDS_BIT;
		}

		void destroy()
		{
			glDeleteSync(m_value);
			m_value = nullptr;
		}

		void reset()
		{
			if (m_value != nullptr)
				destroy();

			create();
		}

		bool is_empty()
		{
			return (m_value == nullptr);
		}

		bool check_signaled()
		{
			verify(HERE), m_value != nullptr;

			if (signaled)
				return true;

			if (flags)
			{
				GLenum err = glClientWaitSync(m_value, flags, 0);
				flags = 0;

				if (!(err == GL_ALREADY_SIGNALED || err == GL_CONDITION_SATISFIED))
					return false;
			}
			else
			{
				GLint status = GL_UNSIGNALED;
				GLint tmp;

				glGetSynciv(m_value, GL_SYNC_STATUS, 4, &tmp, &status);

				if (status != GL_SIGNALED)
					return false;
			}

			signaled = true;
			return true;
		}

		bool wait_for_signal()
		{
			verify(HERE), m_value != nullptr;

			if (signaled == GL_FALSE)
			{
				GLenum err = GL_WAIT_FAILED;
				bool done = false;

				while (!done)
				{
					if (flags)
					{
						err = glClientWaitSync(m_value, flags, 0);
						flags = 0;

						switch (err)
						{
						default:
							LOG_ERROR(RSX, "gl::fence sync returned unknown error 0x%X", err);
						case GL_ALREADY_SIGNALED:
						case GL_CONDITION_SATISFIED:
							done = true;
							break;
						case GL_TIMEOUT_EXPIRED:
							continue;
						}
					}
					else
					{
						GLint status = GL_UNSIGNALED;
						GLint tmp;

						glGetSynciv(m_value, GL_SYNC_STATUS, 4, &tmp, &status);

						if (status == GL_SIGNALED)
							break;
					}
				}

				signaled = (err == GL_ALREADY_SIGNALED || err == GL_CONDITION_SATISFIED);
			}

			glDeleteSync(m_value);
			m_value = nullptr;

			return signaled;
		}
	};

	template<typename Type, uint BindId, uint GetStateId>
	class save_binding_state_base
	{
		GLint m_last_binding;

	public:
		save_binding_state_base(const Type& new_state) : save_binding_state_base()
		{
			new_state.bind();
		}

		save_binding_state_base()
		{
			glGetIntegerv(GetStateId, &m_last_binding);
		}

		~save_binding_state_base()
		{
			glBindBuffer(BindId, m_last_binding);
		}
	};

	enum class filter
	{
		nearest = GL_NEAREST,
		linear = GL_LINEAR
	};

	enum class min_filter
	{
		nearest = GL_NEAREST,
		linear = GL_LINEAR,
		nearest_mipmap_nearest = GL_NEAREST_MIPMAP_NEAREST,
		nearest_mipmap_linear = GL_NEAREST_MIPMAP_LINEAR,
		linear_mipmap_nearest = GL_LINEAR_MIPMAP_NEAREST,
		linear_mipmap_linear = GL_LINEAR_MIPMAP_LINEAR
	};

	enum class buffers
	{
		none = 0,
		color = GL_COLOR_BUFFER_BIT,
		depth = GL_DEPTH_BUFFER_BIT,
		stencil = GL_STENCIL_BUFFER_BIT,

		color_depth = color | depth,
		color_depth_stencil = color | depth | stencil,
		color_stencil = color | stencil,

		depth_stencil = depth | stencil
	};

	class pixel_pack_settings
	{
		bool m_swap_bytes = false;
		bool m_lsb_first = false;
		int m_row_length = 0;
		int m_image_height = 0;
		int m_skip_rows = 0;
		int m_skip_pixels = 0;
		int m_skip_images = 0;
		int m_alignment = 4;

	public:
		void apply() const
		{
			glPixelStorei(GL_PACK_SWAP_BYTES, m_swap_bytes ? GL_TRUE : GL_FALSE);
			glPixelStorei(GL_PACK_LSB_FIRST, m_lsb_first ? GL_TRUE : GL_FALSE);
			glPixelStorei(GL_PACK_ROW_LENGTH, m_row_length);
			glPixelStorei(GL_PACK_IMAGE_HEIGHT, m_image_height);
			glPixelStorei(GL_PACK_SKIP_ROWS, m_skip_rows);
			glPixelStorei(GL_PACK_SKIP_PIXELS, m_skip_pixels);
			glPixelStorei(GL_PACK_SKIP_IMAGES, m_skip_images);
			glPixelStorei(GL_PACK_ALIGNMENT, m_alignment);
		}

		pixel_pack_settings& swap_bytes(bool value = true)
		{
			m_swap_bytes = value;
			return *this;
		}
		pixel_pack_settings& lsb_first(bool value = true)
		{
			m_lsb_first = value;
			return *this;
		}
		pixel_pack_settings& row_length(int value)
		{
			m_row_length = value;
			return *this;
		}
		pixel_pack_settings& image_height(int value)
		{
			m_image_height = value;
			return *this;
		}
		pixel_pack_settings& skip_rows(int value)
		{
			m_skip_rows = value;
			return *this;
		}
		pixel_pack_settings& skip_pixels(int value)
		{
			m_skip_pixels = value;
			return *this;
		}
		pixel_pack_settings& skip_images(int value)
		{
			m_skip_images = value;
			return *this;
		}
		pixel_pack_settings& alignment(int value)
		{
			m_alignment = value;
			return *this;
		}
	};

	class pixel_unpack_settings
	{
		bool m_swap_bytes = false;
		bool m_lsb_first = false;
		int m_row_length = 0;
		int m_image_height = 0;
		int m_skip_rows = 0;
		int m_skip_pixels = 0;
		int m_skip_images = 0;
		int m_alignment = 4;

	public:
		void apply() const
		{
			glPixelStorei(GL_UNPACK_SWAP_BYTES, m_swap_bytes ? GL_TRUE : GL_FALSE);
			glPixelStorei(GL_UNPACK_LSB_FIRST, m_lsb_first ? GL_TRUE : GL_FALSE);
			glPixelStorei(GL_UNPACK_ROW_LENGTH, m_row_length);
			glPixelStorei(GL_UNPACK_IMAGE_HEIGHT, m_image_height);
			glPixelStorei(GL_UNPACK_SKIP_ROWS, m_skip_rows);
			glPixelStorei(GL_UNPACK_SKIP_PIXELS, m_skip_pixels);
			glPixelStorei(GL_UNPACK_SKIP_IMAGES, m_skip_images);
			glPixelStorei(GL_UNPACK_ALIGNMENT, m_alignment);
		}

		pixel_unpack_settings& swap_bytes(bool value = true)
		{
			m_swap_bytes = value;
			return *this;
		}
		pixel_unpack_settings& lsb_first(bool value = true)
		{
			m_lsb_first = value;
			return *this;
		}
		pixel_unpack_settings& row_length(int value)
		{
			m_row_length = value;
			return *this;
		}
		pixel_unpack_settings& image_height(int value)
		{
			m_image_height = value;
			return *this;
		}
		pixel_unpack_settings& skip_rows(int value)
		{
			m_skip_rows = value;
			return *this;
		}
		pixel_unpack_settings& skip_pixels(int value)
		{
			m_skip_pixels = value;
			return *this;
		}
		pixel_unpack_settings& skip_images(int value)
		{
			m_skip_images = value;
			return *this;
		}
		pixel_unpack_settings& alignment(int value)
		{
			m_alignment = value;
			return *this;
		}
	};

	class vao;
	class attrib_t;

	class buffer_pointer
	{
	public:
		enum class type
		{
			s8 = GL_BYTE,
			u8 = GL_UNSIGNED_BYTE,
			s16 = GL_SHORT,
			u16 = GL_UNSIGNED_SHORT,
			s32 = GL_INT,
			u32 = GL_UNSIGNED_INT,
			f16 = GL_HALF_FLOAT,
			f32 = GL_FLOAT,
			f64 = GL_DOUBLE,
			fixed = GL_FIXED,
			s32_2_10_10_10_rev = GL_INT_2_10_10_10_REV,
			u32_2_10_10_10_rev = GL_UNSIGNED_INT_2_10_10_10_REV,
			u32_10f_11f_11f_rev = GL_UNSIGNED_INT_10F_11F_11F_REV
		};

	private:
		vao* m_vao;
		u32 m_offset;
		u32 m_stride;
		u32 m_size = 4;
		type m_type = type::f32;
		bool m_normalize = false;

	public:
		buffer_pointer(vao* vao, u32 offset = 0, u32 stride = 0)
			: m_vao(vao)
			, m_offset(offset)
			, m_stride(stride)
		{
		}

		const class ::gl::vao& get_vao() const
		{
			return *m_vao;
		}

		class ::gl::vao& get_vao()
		{
			return *m_vao;
		}

		buffer_pointer& offset(u32 value)
		{
			m_offset = value;
			return *this;
		}

		u32 offset() const
		{
			return m_offset;
		}

		buffer_pointer& stride(u32 value)
		{
			m_stride = value;
			return *this;
		}

		u32 stride() const
		{
			return m_stride;
		}

		buffer_pointer& size(u32 value)
		{
			m_size = value;
			return *this;
		}

		u32 size() const
		{
			return m_size;
		}

		buffer_pointer& set_type(type value)
		{
			m_type = value;
			return *this;
		}

		type get_type() const
		{
			return m_type;
		}

		buffer_pointer& normalize(bool value)
		{
			m_normalize = value;
			return *this;
		}

		bool normalize() const
		{
			return m_normalize;
		}

		buffer_pointer& operator >> (u32 value)
		{
			return stride(value);
		}

		buffer_pointer& config(type type_ = type::f32, u32 size_ = 4, bool normalize_ = false)
		{
			return set_type(type_).size(size_).normalize(normalize_);
		}
	};

	class buffer
	{
	public:
		enum class target
		{
			pixel_pack = GL_PIXEL_PACK_BUFFER,
			pixel_unpack = GL_PIXEL_UNPACK_BUFFER,
			array = GL_ARRAY_BUFFER,
			element_array = GL_ELEMENT_ARRAY_BUFFER,
			uniform = GL_UNIFORM_BUFFER,
			texture = GL_TEXTURE_BUFFER
		};
		enum class access
		{
			read = GL_READ_ONLY,
			write = GL_WRITE_ONLY,
			read_write = GL_READ_WRITE
		};

	protected:
		GLuint m_id = GL_NONE;
		GLsizeiptr m_size = 0;
		target m_target = target::array;

	public:
		buffer() = default;
		buffer(const buffer&) = delete;

		buffer(GLuint id)
		{
			set_id(id);
		}

		~buffer()
		{
			if (created())
				remove();
		}

		class save_binding_state
		{
			GLint m_last_binding;
			GLenum m_target;

		public:
			save_binding_state(target target_, const buffer& new_state) : save_binding_state(target_)
			{
				new_state.bind(target_);
			}

			save_binding_state(target target_)
			{
				GLenum pname;
				switch (target_)
				{
				case target::pixel_pack: pname = GL_PIXEL_PACK_BUFFER_BINDING; break;
				case target::pixel_unpack: pname = GL_PIXEL_UNPACK_BUFFER_BINDING; break;
				case target::array: pname = GL_ARRAY_BUFFER_BINDING; break;
				case target::element_array: pname = GL_ELEMENT_ARRAY_BUFFER_BINDING; break;
				case target::uniform: pname = GL_UNIFORM_BUFFER_BINDING; break;
				case target::texture: pname = GL_TEXTURE_BUFFER_BINDING; break;
				}

				glGetIntegerv(pname, &m_last_binding);
				m_target = (GLenum)target_;
			}


			~save_binding_state()
			{
				glBindBuffer(m_target, m_last_binding);
			}
		};

		void recreate()
		{
			if (created())
			{
				remove();
			}

			create();
		}

		void recreate(GLsizeiptr size, const void* data = nullptr)
		{
			if (created())
			{
				remove();
			}

			create(size, data);
		}

		void create()
		{
			glGenBuffers(1, &m_id);
		}

		void create(GLsizeiptr size, const void* data_ = nullptr, GLenum usage = GL_STREAM_DRAW)
		{
			create();
			data(size, data_, usage);
		}

		void create(target target_, GLsizeiptr size, const void* data_ = nullptr, GLenum usage = GL_STREAM_DRAW)
		{
			create();
			m_target = target_;
			data(size, data_, usage);
		}

		void data(GLsizeiptr size, const void* data_ = nullptr, GLenum usage = GL_STREAM_DRAW)
		{
			target target_ = current_target();
			save_binding_state save(target_, *this);
			glBufferData((GLenum)target_, size, data_, usage);
			m_size = size;
		}

		void sub_data(GLintptr offset, GLsizeiptr size, const void* data_ = nullptr)
		{
			target target_ = current_target();
			save_binding_state save(target_, *this);
			glBufferSubData((GLenum)target_, offset, size, data_);
		}

		void bind(target target_) const
		{
			glBindBuffer((GLenum)target_, m_id);
		}

		void bind() const
		{
			bind(current_target());
		}

		target current_target() const
		{
			return m_target;
		}

		void remove()
		{
			glDeleteBuffers(1, &m_id);
			m_id = 0;
		}

		GLsizeiptr size() const
		{
			return m_size;
		}

		uint id() const
		{
			return m_id;
		}

		void set_id(uint id)
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

		void map(std::function<void(GLubyte*)> impl, access access_)
		{
			target target_ = current_target();
			save_binding_state save(target_, *this);

			if (GLubyte* ptr = (GLubyte*)glMapBuffer((GLenum)target_, (GLenum)access_))
			{
				impl(ptr);
				glUnmapBuffer((GLenum)target_);
			}
		}

		class mapper
		{
			buffer *m_parent;
			GLubyte *m_data;

		public:
			mapper(buffer& parent, access access_)
			{
				m_parent = &parent;
				m_data = parent.map(access_);
			}

			~mapper()
			{
				m_parent->unmap();
			}

			GLubyte* get() const
			{
				return m_data;
			}
		};

		GLubyte* map(access access_)
		{
			bind(current_target());

			return (GLubyte*)glMapBuffer((GLenum)current_target(), (GLenum)access_);
		}

		void unmap()
		{
			glUnmapBuffer((GLenum)current_target());
		}
	};

	class ring_buffer : public buffer
	{
	protected:

		u32 m_data_loc = 0;
		void *m_memory_mapping = nullptr;

		fence m_fence;

	public:

		virtual void recreate(GLsizeiptr size, const void* data = nullptr)
		{
			if (m_id)
			{
				m_fence.wait_for_signal();
				remove();
			}

			buffer::create();

			GLbitfield buffer_storage_flags = GL_MAP_WRITE_BIT | GL_MAP_PERSISTENT_BIT | GL_MAP_COHERENT_BIT;
			if (gl::get_driver_caps().vendor_MESA) buffer_storage_flags |= GL_CLIENT_STORAGE_BIT;

			glBindBuffer((GLenum)m_target, m_id);
			glBufferStorage((GLenum)m_target, size, data, buffer_storage_flags);
			m_memory_mapping = glMapBufferRange((GLenum)m_target, 0, size, GL_MAP_WRITE_BIT | GL_MAP_PERSISTENT_BIT | GL_MAP_COHERENT_BIT);

			verify(HERE), m_memory_mapping != nullptr;
			m_data_loc = 0;
			m_size = ::narrow<u32>(size);
		}

		void create(target target_, GLsizeiptr size, const void* data_ = nullptr)
		{
			m_target = target_;
			recreate(size, data_);
		}

		virtual std::pair<void*, u32> alloc_from_heap(u32 alloc_size, u16 alignment)
		{
			u32 offset = m_data_loc;
			if (m_data_loc) offset = align(offset, alignment);

			if ((offset + alloc_size) > m_size)
			{
				if (!m_fence.is_empty())
				{
					m_fence.wait_for_signal();
				}
				else
				{
					LOG_ERROR(RSX, "OOM Error: Ring buffer was likely being used without notify() being called");
					glFinish();
				}

				m_data_loc = 0;
				offset = 0;
			}

			//Align data loc to 256; allows some "guard" region so we dont trample our own data inadvertently
			m_data_loc = align(offset + alloc_size, 256);
			return std::make_pair(((char*)m_memory_mapping) + offset, offset);
		}

		virtual void remove()
		{
			if (m_memory_mapping)
			{
				glBindBuffer((GLenum)m_target, m_id);
				glUnmapBuffer((GLenum)m_target);

				m_memory_mapping = nullptr;
				m_data_loc = 0;
				m_size = 0;
			}

			glDeleteBuffers(1, &m_id);
			m_id = 0;
		}

		virtual void reserve_storage_on_heap(u32 /*alloc_size*/) {}

		virtual void unmap() {}

		void bind_range(u32 index, u32 offset, u32 size) const
		{
			glBindBufferRange((GLenum)current_target(), index, id(), offset, size);
		}

		//Notification of a draw command
		virtual void notify()
		{
			//Insert fence about 25% into the buffer
			if (m_fence.is_empty() && (m_data_loc > (m_size >> 2)))
				m_fence.reset();
		}
	};

	class legacy_ring_buffer : public ring_buffer
	{
		u32 m_mapped_bytes = 0;
		u32 m_mapping_offset = 0;
		u32 m_alignment_offset = 0;

	public:

		void recreate(GLsizeiptr size, const void* data = nullptr) override
		{
			if (m_id)
				remove();

			buffer::create();
			buffer::data(size, data);

			m_memory_mapping = nullptr;
			m_data_loc = 0;
			m_size = ::narrow<u32>(size);
		}

		void create(target target_, GLsizeiptr size, const void* data_ = nullptr)
		{
			m_target = target_;
			recreate(size, data_);
		}

		void reserve_storage_on_heap(u32 alloc_size) override
		{
			verify (HERE), m_memory_mapping == nullptr;

			u32 offset = m_data_loc;
			if (m_data_loc) offset = align(offset, 256);

			const u32 block_size = align(alloc_size + 16, 256);	//Overallocate just in case we need to realign base

			if ((offset + block_size) > m_size)
			{
				buffer::data(m_size, nullptr);
				m_data_loc = 0;
			}

			glBindBuffer((GLenum)m_target, m_id);
			m_memory_mapping = glMapBufferRange((GLenum)m_target, m_data_loc, block_size, GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_RANGE_BIT | GL_MAP_UNSYNCHRONIZED_BIT);
			m_mapped_bytes = block_size;
			m_mapping_offset = m_data_loc;
			m_alignment_offset = 0;

			//When using debugging tools, the mapped base might not be aligned as expected
			const u64 mapped_address_base = (u64)m_memory_mapping;
			if (mapped_address_base & 0xF)
			{
				//Unaligned result was returned. We have to modify the base address a bit
				//We lose some memory here, but the 16 byte overallocation above makes up for it
				const u64 new_base = (mapped_address_base & ~0xF) + 16;
				const u64 diff_bytes = new_base - mapped_address_base;

				m_memory_mapping = (void *)new_base;
				m_mapped_bytes -= ::narrow<u32>(diff_bytes);
				m_alignment_offset = ::narrow<u32>(diff_bytes);
			}

			verify(HERE), m_mapped_bytes >= alloc_size;
		}

		std::pair<void*, u32> alloc_from_heap(u32 alloc_size, u16 alignment) override
		{
			u32 offset = m_data_loc;
			if (m_data_loc) offset = align(offset, alignment);

			u32 padding = (offset - m_data_loc);
			u32 real_size = align(padding + alloc_size, alignment);	//Ensures we leave the loc pointer aligned after we exit

			if (real_size > m_mapped_bytes)
			{
				//Missed allocation. We take a performance hit on doing this.
				//Overallocate slightly for the next allocation if requested size is too small
				unmap();
				reserve_storage_on_heap(std::max(real_size, 4096U));

				offset = m_data_loc;
				if (m_data_loc) offset = align(offset, alignment);

				padding = (offset - m_data_loc);
				real_size = align(padding + alloc_size, alignment);
			}

			m_data_loc = offset + real_size;
			m_mapped_bytes -= real_size;

			u32 local_offset = (offset - m_mapping_offset);
			return std::make_pair(((char*)m_memory_mapping) + local_offset, offset + m_alignment_offset);
		}

		void remove() override
		{
			ring_buffer::remove();
			m_mapped_bytes = 0;
		}

		void unmap() override
		{
			buffer::bind();
			buffer::unmap();

			m_memory_mapping = nullptr;
			m_mapped_bytes = 0;
			m_mapping_offset = 0;
		}

		void notify() override {}
	};

	class buffer_view
	{
		buffer* m_buffer = nullptr;
		u32 m_offset = 0;
		u32 m_range = 0;
		GLenum m_format = GL_R8UI;

	public:
		buffer_view(buffer *_buffer, u32 offset, u32 range, GLenum format = GL_R8UI)
			: m_buffer(_buffer), m_offset(offset), m_range(range), m_format(format)
		{}

		buffer_view()
		{}

		void update(buffer *_buffer, u32 offset, u32 range, GLenum format = GL_R8UI)
		{
			verify(HERE), _buffer->size() >= (offset + range);
			m_buffer = _buffer;
			m_offset = offset;
			m_range = range;
			m_format = format;
		}

		u32 offset() const
		{
			return m_offset;
		}

		u32 range() const
		{
			return m_range;
		}

		u32 format() const
		{
			return m_format;
		}

		buffer* value() const
		{
			return m_buffer;
		}

		bool in_range(u32 address, u32 size, u32& new_offset) const
		{
			if (address < m_offset)
				return false;

			const u32 _offset = address - m_offset;
			if (m_range < _offset)
				return false;

			const auto remaining = m_range - _offset;
			if (size <= remaining)
			{
				new_offset = _offset;
				return true;
			}

			return false;
		}
	};

	class vao
	{
		template<buffer::target BindId, uint GetStateId>
		class entry
		{
			vao& m_parent;

		public:
			using save_binding_state = save_binding_state_base<entry, (GLuint)BindId, GetStateId>;

			entry(vao* parent) noexcept : m_parent(*parent)
			{
			}

			entry& operator = (const buffer& buf) noexcept
			{
				m_parent.bind();
				buf.bind(BindId);

				return *this;
			}
		};

		GLuint m_id = GL_NONE;

	public:
		entry<buffer::target::pixel_pack, GL_PIXEL_PACK_BUFFER_BINDING> pixel_pack_buffer{ this };
		entry<buffer::target::pixel_unpack, GL_PIXEL_UNPACK_BUFFER_BINDING> pixel_unpack_buffer{ this };
		entry<buffer::target::array, GL_ARRAY_BUFFER_BINDING> array_buffer{ this };
		entry<buffer::target::element_array, GL_ELEMENT_ARRAY_BUFFER_BINDING> element_array_buffer{ this };

		vao() = default;
		vao(vao&) = delete;

		vao(vao&& vao_) noexcept
		{
			swap(vao_);
		}
		vao(GLuint id) noexcept
		{
			set_id(id);
		}

		~vao() noexcept
		{
			if (created())
				remove();
		}

		void swap(vao& vao_) noexcept
		{
			auto my_old_id = id();
			set_id(vao_.id());
			vao_.set_id(my_old_id);
		}

		vao& operator = (const vao& rhs) = delete;
		vao& operator = (vao&& rhs) noexcept
		{
			swap(rhs);
			return *this;
		}

		void bind() const noexcept
		{
			glBindVertexArray(m_id);
		}

		void create() noexcept
		{
			glGenVertexArrays(1, &m_id);
		}

		void remove() noexcept
		{
			glDeleteVertexArrays(1, &m_id);
			m_id = GL_NONE;
		}

		uint id() const noexcept
		{
			return m_id;
		}

		void set_id(uint id) noexcept
		{
			m_id = id;
		}

		bool created() const noexcept
		{
			return m_id != GL_NONE;
		}

		explicit operator bool() const noexcept
		{
			return created();
		}

		void enable_for_attributes(std::initializer_list<GLuint> indexes) noexcept
		{
			for (auto &index : indexes)
			{
				glEnableVertexAttribArray(index);
			}
		}

		void disable_for_attributes(std::initializer_list<GLuint> indexes) noexcept
		{
			for (auto &index : indexes)
			{
				glDisableVertexAttribArray(index);
			}
		}

		void enable_for_attribute(GLuint index) noexcept
		{
			enable_for_attributes({ index });
		}

		void disable_for_attribute(GLuint index) noexcept
		{
			disable_for_attributes({ index });
		}

		buffer_pointer operator + (u32 offset) const noexcept
		{
			return{ (vao*)this, offset };
		}

		buffer_pointer operator >> (u32 stride) const noexcept
		{
			return{ (vao*)this, {}, stride };
		}

		operator buffer_pointer() const noexcept
		{
			return{ (vao*)this };
		}

		attrib_t operator [] (u32 index) const noexcept;
	};

	class attrib_t
	{
		GLint m_location;

	public:
		attrib_t(GLint location)
			: m_location(location)
		{
		}

		GLint location() const
		{
			return m_location;
		}

		void operator = (float rhs) const { glDisableVertexAttribArray(location()); glVertexAttrib1f(location(), rhs); }
		void operator = (double rhs) const { glDisableVertexAttribArray(location()); glVertexAttrib1d(location(), rhs); }

		void operator = (const color1f& rhs) const { glDisableVertexAttribArray(location()); glVertexAttrib1f(location(), rhs.r); }
		void operator = (const color1d& rhs) const { glDisableVertexAttribArray(location()); glVertexAttrib1d(location(), rhs.r); }
		void operator = (const color2f& rhs) const { glDisableVertexAttribArray(location()); glVertexAttrib2f(location(), rhs.r, rhs.g); }
		void operator = (const color2d& rhs) const { glDisableVertexAttribArray(location()); glVertexAttrib2d(location(), rhs.r, rhs.g); }
		void operator = (const color3f& rhs) const { glDisableVertexAttribArray(location()); glVertexAttrib3f(location(), rhs.r, rhs.g, rhs.b); }
		void operator = (const color3d& rhs) const { glDisableVertexAttribArray(location()); glVertexAttrib3d(location(), rhs.r, rhs.g, rhs.b); }
		void operator = (const color4f& rhs) const { glDisableVertexAttribArray(location()); glVertexAttrib4f(location(), rhs.r, rhs.g, rhs.b, rhs.a); }
		void operator = (const color4d& rhs) const { glDisableVertexAttribArray(location()); glVertexAttrib4d(location(), rhs.r, rhs.g, rhs.b, rhs.a); }

		void operator = (buffer_pointer& pointer) const
		{
			pointer.get_vao().enable_for_attribute(m_location);
			glVertexAttribPointer(location(), pointer.size(), (GLenum)pointer.get_type(), pointer.normalize(),
				pointer.stride(), (const void*)(size_t)pointer.offset());
		}
	};

	class texture
	{
	public:
		enum class type
		{
			ubyte = GL_UNSIGNED_BYTE,
			ushort = GL_UNSIGNED_SHORT,
			uint = GL_UNSIGNED_INT,

			ubyte_3_3_2 = GL_UNSIGNED_BYTE_3_3_2,
			ubyte_2_3_3_rev = GL_UNSIGNED_BYTE_2_3_3_REV,

			ushort_5_6_5 = GL_UNSIGNED_SHORT_5_6_5,
			ushort_5_6_5_rev = GL_UNSIGNED_SHORT_5_6_5_REV,
			ushort_4_4_4_4 = GL_UNSIGNED_SHORT_4_4_4_4,
			ushort_4_4_4_4_rev = GL_UNSIGNED_SHORT_4_4_4_4_REV,
			ushort_5_5_5_1 = GL_UNSIGNED_SHORT_5_5_5_1,
			ushort_1_5_5_5_rev = GL_UNSIGNED_SHORT_1_5_5_5_REV,

			uint_8_8_8_8 = GL_UNSIGNED_INT_8_8_8_8,
			uint_8_8_8_8_rev = GL_UNSIGNED_INT_8_8_8_8_REV,
			uint_10_10_10_2 = GL_UNSIGNED_INT_10_10_10_2,
			uint_2_10_10_10_rev = GL_UNSIGNED_INT_2_10_10_10_REV,
			uint_24_8 = GL_UNSIGNED_INT_24_8,
			float32_uint8 = GL_FLOAT_32_UNSIGNED_INT_24_8_REV,

			sbyte = GL_BYTE,
			sshort = GL_SHORT,
			sint = GL_INT,
			f16 = GL_HALF_FLOAT,
			f32 = GL_FLOAT,
			f64 = GL_DOUBLE,
		};

		enum class channel
		{
			zero = GL_ZERO,
			one = GL_ONE,
			r = GL_RED,
			g = GL_GREEN,
			b = GL_BLUE,
			a = GL_ALPHA,
		};

		enum class format
		{
			r = GL_RED,
			rg = GL_RG,
			rgb = GL_RGB,
			rgba = GL_RGBA,

			bgr = GL_BGR,
			bgra = GL_BGRA,

			stencil = GL_STENCIL_INDEX,
			depth = GL_DEPTH_COMPONENT,
			depth_stencil = GL_DEPTH_STENCIL
		};

		enum class internal_format
		{
			red = GL_RED,
			r = GL_R,
			rg = GL_RG,
			rgb = GL_RGB,
			rgba = GL_RGBA,

			bgr = GL_BGR,
			bgra = GL_BGRA,

			stencil = GL_STENCIL_INDEX,
			stencil8 = GL_STENCIL_INDEX8,
			depth = GL_DEPTH_COMPONENT,
			depth16 = GL_DEPTH_COMPONENT16,
			depth_stencil = GL_DEPTH_STENCIL,
			depth24_stencil8 = GL_DEPTH24_STENCIL8,
			depth32f_stencil8 = GL_DEPTH32F_STENCIL8,

			compressed_rgb_s3tc_dxt1 = GL_COMPRESSED_RGB_S3TC_DXT1_EXT,
			compressed_rgba_s3tc_dxt1 = GL_COMPRESSED_RGBA_S3TC_DXT1_EXT,
			compressed_rgba_s3tc_dxt3 = GL_COMPRESSED_RGBA_S3TC_DXT3_EXT,
			compressed_rgba_s3tc_dxt5 = GL_COMPRESSED_RGBA_S3TC_DXT5_EXT,

			//Sized internal formats, see opengl spec document on glTexImage2D, table 3
			rgba8 = GL_RGBA8,
			r5g6b5 = GL_RGB565,
			r8 = GL_R8,
			rg8 = GL_RG8,
			r32f = GL_R32F,
			rgba16f = GL_RGBA16F,
			rgba32f = GL_RGBA32F
		};

		enum class wrap
		{
			repeat = GL_REPEAT,
			mirrored_repeat = GL_MIRRORED_REPEAT,
			clamp_to_edge = GL_CLAMP_TO_EDGE,
			clamp_to_border = GL_CLAMP_TO_BORDER,
			mirror_clamp = GL_MIRROR_CLAMP_EXT,
			//mirror_clamp_to_edge = GL_MIRROR_CLAMP_TO_EDGE,
			mirror_clamp_to_border = GL_MIRROR_CLAMP_TO_BORDER_EXT
		};

		enum class compare_mode
		{
			none = GL_NONE,
			ref_to_texture = GL_COMPARE_REF_TO_TEXTURE
		};

		enum class compare_func
		{
			never = GL_NEVER,
			less = GL_LESS,
			equal = GL_EQUAL,
			lequal = GL_LEQUAL,
			greater = GL_GREATER,
			notequal = GL_NOTEQUAL,
			gequal = GL_GEQUAL,
			always = GL_ALWAYS
		};

		enum class target
		{
			texture1D = GL_TEXTURE_1D,
			texture2D = GL_TEXTURE_2D,
			texture3D = GL_TEXTURE_3D,
			textureCUBE = GL_TEXTURE_CUBE_MAP,
			textureBuffer = GL_TEXTURE_BUFFER
		};

		enum class channel_type
		{
			none = GL_NONE,
			signed_normalized = GL_SIGNED_NORMALIZED,
			unsigned_normalized = GL_UNSIGNED_NORMALIZED,
			float_ = GL_FLOAT,
			int_ = GL_INT,
			uint_ = GL_UNSIGNED_INT
		};

		enum class channel_name
		{
			red = GL_TEXTURE_RED_TYPE,
			green = GL_TEXTURE_GREEN_TYPE,
			blue = GL_TEXTURE_BLUE_TYPE,
			alpha = GL_TEXTURE_ALPHA_TYPE,
			depth = GL_TEXTURE_DEPTH_TYPE
		};

	protected:
		GLuint m_id = 0;
		GLuint m_width = 0;
		GLuint m_height = 0;
		GLuint m_depth = 0;
		GLuint m_mipmaps = 0;
		GLuint m_pitch = 0;
		GLuint m_compressed = GL_FALSE;

		target m_target = target::texture2D;
		internal_format m_internal_format = internal_format::rgba8;
		std::array<GLenum, 4> m_component_layout;

	private:
		class save_binding_state
		{
			GLenum target = GL_NONE;
			GLuint old_binding = GL_NONE;

		public:
			save_binding_state(GLenum target)
			{
				this->target = target;
				switch (target)
				{
				case GL_TEXTURE_1D:
					glGetIntegerv(GL_TEXTURE_BINDING_1D, (GLint*)&old_binding);
					break;
				case GL_TEXTURE_2D:
					glGetIntegerv(GL_TEXTURE_BINDING_2D, (GLint*)&old_binding);
					break;
				case GL_TEXTURE_3D:
					glGetIntegerv(GL_TEXTURE_BINDING_3D, (GLint*)&old_binding);
					break;
				case GL_TEXTURE_CUBE_MAP:
					glGetIntegerv(GL_TEXTURE_BINDING_CUBE_MAP, (GLint*)&old_binding);
					break;
				case GL_TEXTURE_BUFFER:
					glGetIntegerv(GL_TEXTURE_BINDING_BUFFER, (GLint*)&old_binding);
					break;
				}
			}

			~save_binding_state()
			{
				glBindTexture(target, old_binding);
			}
		};
	public:
		texture(const texture&) = delete;
		texture(texture&& texture_) = delete;

		texture(GLenum target, GLuint width, GLuint height, GLuint depth, GLuint mipmaps, GLenum sized_format)
		{
			save_binding_state save(target);
			glGenTextures(1, &m_id);
			glBindTexture(target, m_id); //Must bind to initialize the new texture

			switch (target)
			{
			default:
				fmt::throw_exception("Invalid image target 0x%X" HERE, target);
			case GL_TEXTURE_1D:
				glTexStorage1D(target, mipmaps, sized_format, width);
				height = depth = 1;
				break;
			case GL_TEXTURE_2D:
			case GL_TEXTURE_CUBE_MAP:
				glTexStorage2D(target, mipmaps, sized_format, width, height);
				depth = 1;
				break;
			case GL_TEXTURE_3D:
				glTexStorage3D(target, mipmaps, sized_format, width, height, depth);
				break;
			case GL_TEXTURE_BUFFER:
				break;
			}

			if (target != GL_TEXTURE_BUFFER)
			{
				glTexParameteri(target, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
				glTexParameteri(target, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
				glTexParameteri(target, GL_TEXTURE_WRAP_S, GL_REPEAT);
				glTexParameteri(target, GL_TEXTURE_WRAP_T, GL_REPEAT);
				glTexParameteri(target, GL_TEXTURE_WRAP_R, GL_REPEAT);
				glTexParameteri(target, GL_TEXTURE_BASE_LEVEL, 0);
				glTexParameteri(target, GL_TEXTURE_MAX_LEVEL, mipmaps - 1);

				m_width = width;
				m_height = height;
				m_depth = depth;
				m_mipmaps = mipmaps;

				switch (sized_format)
				{
				case GL_DEPTH_COMPONENT16:
				{
					m_pitch = width * 2;
					break;
				}
				case GL_DEPTH24_STENCIL8:
				case GL_DEPTH32F_STENCIL8:
				{
					m_pitch = width * 4;
					break;
				}
				case GL_COMPRESSED_RGBA_S3TC_DXT1_EXT:
				{
					m_compressed = true;
					m_pitch = width / 2;
					break;
				}
				case GL_COMPRESSED_RGBA_S3TC_DXT3_EXT:
				case GL_COMPRESSED_RGBA_S3TC_DXT5_EXT:
				{
					m_compressed = true;
					m_pitch = width;
					break;
				}
				default:
				{
					GLenum query_target = (target == GL_TEXTURE_CUBE_MAP) ? GL_TEXTURE_CUBE_MAP_POSITIVE_X : target;
					GLint r, g, b, a;

					glGetTexLevelParameteriv(query_target, 0, GL_TEXTURE_RED_SIZE, &r);
					glGetTexLevelParameteriv(query_target, 0, GL_TEXTURE_GREEN_SIZE, &g);
					glGetTexLevelParameteriv(query_target, 0, GL_TEXTURE_BLUE_SIZE, &b);
					glGetTexLevelParameteriv(query_target, 0, GL_TEXTURE_ALPHA_SIZE, &a);

					m_pitch = width * (r + g + b + a) / 8;
					break;
				}
				}

				if (!m_pitch)
				{
					fmt::throw_exception("Unhandled GL format 0x%X" HERE, sized_format);
				}
			}

			m_target = static_cast<texture::target>(target);
			m_internal_format = static_cast<internal_format>(sized_format);
			m_component_layout = { GL_ALPHA, GL_RED, GL_GREEN, GL_BLUE };
		}

		virtual ~texture()
		{
			glDeleteTextures(1, &m_id);
		}

		void set_native_component_layout(const std::array<GLenum, 4>& layout)
		{
			m_component_layout[0] = layout[0];
			m_component_layout[1] = layout[1];
			m_component_layout[2] = layout[2];
			m_component_layout[3] = layout[3];
		}

		target get_target() const noexcept
		{
			return m_target;
		}

		static bool compressed_format(internal_format format_) noexcept
		{
			switch (format_)
			{
			case internal_format::compressed_rgb_s3tc_dxt1:
			case internal_format::compressed_rgba_s3tc_dxt1:
			case internal_format::compressed_rgba_s3tc_dxt3:
			case internal_format::compressed_rgba_s3tc_dxt5:
				return true;
			default:
				return false;
			}
		}

		uint id() const noexcept
		{
			return m_id;
		}

		explicit operator bool() const noexcept
		{
			return (m_id != 0);
		}

		GLuint width() const
		{
			return m_width;
		}

		GLuint height() const
		{
			return m_height;
		}

		GLuint depth() const
		{
			return m_depth;
		}

		GLuint levels() const
		{
			return m_mipmaps;
		}

		GLuint pitch() const
		{
			return m_pitch;
		}

		GLboolean compressed() const
		{
			return m_compressed;
		}

		sizei size2D() const
		{
			return{ (int)m_width, (int)m_height };
		}

		texture::internal_format get_internal_format() const
		{
			return m_internal_format;
		}

		std::array<GLenum, 4> get_native_component_layout() const
		{
			return m_component_layout;
		}

		void copy_from(const void* src, texture::format format, texture::type type, class pixel_unpack_settings pixel_settings)
		{
			pixel_settings.apply();

			switch ((GLenum)m_target)
			{
			case GL_TEXTURE_1D:
			{
				DSA_CALL(TextureSubImage1D, m_id, GL_TEXTURE_1D, 0, 0, m_width, (GLenum)format, (GLenum)type, src);
				break;
			}
			case GL_TEXTURE_2D:
			{
				DSA_CALL(TextureSubImage2D, m_id, GL_TEXTURE_2D, 0, 0, 0, m_width, m_height, (GLenum)format, (GLenum)type, src);
				break;
			}
			case GL_TEXTURE_3D:
			{
				DSA_CALL(TextureSubImage3D, m_id, GL_TEXTURE_3D, 0, 0, 0, 0, m_width, m_height, m_depth, (GLenum)format, (GLenum)type, src);
				break;
			}
			case GL_TEXTURE_CUBE_MAP:
			{
				if (::gl::get_driver_caps().ARB_dsa_supported)
				{
					glTextureSubImage3D(m_id, 0, 0, 0, 0, m_width, m_height, 6, (GLenum)format, (GLenum)type, src);
				}
				else
				{
					LOG_WARNING(RSX, "Cubemap upload via texture::copy_from is halfplemented!");
					u8* ptr = (u8*)src;
					for (int face = 0; face < 6; ++face)
					{
						glTextureSubImage2DEXT(m_id, GL_TEXTURE_CUBE_MAP_POSITIVE_X + face, 0, 0, 0, m_width, m_height, (GLenum)format, (GLenum)type, ptr);
						ptr += (m_width * m_height * 4); //TODO
					}
				}
				break;
			}
			}
		}

		void copy_from(buffer &buf, u32 gl_format_type, u32 offset, u32 length)
		{
			if (get_target() != target::textureBuffer)
				fmt::throw_exception("OpenGL error: texture cannot copy from buffer" HERE);

			DSA_CALL(TextureBufferRange, m_id, GL_TEXTURE_BUFFER, gl_format_type, buf.id(), offset, length);
		}

		void copy_from(buffer_view &view)
		{
			copy_from(*view.value(), view.format(), view.offset(), view.range());
		}

		void copy_from(const buffer& buf, texture::format format, texture::type type, class pixel_unpack_settings pixel_settings)
		{
			buffer::save_binding_state save_buffer(buffer::target::pixel_unpack, buf);
			copy_from(nullptr, format, type, pixel_settings);
		}

		void copy_from(void* src, texture::format format, texture::type type)
		{
			copy_from(src, format, type, pixel_unpack_settings());
		}

		void copy_from(const buffer& buf, texture::format format, texture::type type)
		{
			copy_from(buf, format, type, pixel_unpack_settings());
		}

		void copy_to(void* dst, texture::format format, texture::type type, class pixel_pack_settings pixel_settings) const
		{
			pixel_settings.apply();
			if (gl::get_driver_caps().ARB_dsa_supported)
				glGetTextureImage(m_id, 0, (GLenum)format, (GLenum)type, m_width * m_height * 16, dst);
			else
				glGetTextureImageEXT(m_id, (GLenum)m_target, 0, (GLenum)format, (GLenum)type, dst);
		}

		void copy_to(const buffer& buf, texture::format format, texture::type type, class pixel_pack_settings pixel_settings) const
		{
			buffer::save_binding_state save_buffer(buffer::target::pixel_pack, buf);
			copy_to(nullptr, format, type, pixel_settings);
		}

		void copy_to(void* dst, texture::format format, texture::type type) const
		{
			copy_to(dst, format, type, pixel_pack_settings());
		}

		void copy_to(const buffer& buf, texture::format format, texture::type type) const
		{
			copy_to(buf, format, type, pixel_pack_settings());
		}
	};

	enum image_aspect : u32
	{
		color = 1,
		depth = 2,
		stencil = 4
	};

	class texture_view
	{
		GLuint m_id = 0;
		GLenum m_target = 0;
		GLenum m_format = 0;
		GLenum m_aspect_flags = 0;
		texture *m_image_data = nullptr;

		GLenum component_swizzle[4];

		void create(texture* data, GLenum target, GLenum sized_format, GLenum aspect_flags, const GLenum* argb_swizzle = nullptr)
		{
			m_target = target;
			m_format = sized_format;
			m_image_data = data;
			m_aspect_flags = aspect_flags;

			const auto num_levels = data->levels();
			const auto num_layers = (target != GL_TEXTURE_CUBE_MAP) ? 1 : 6;

			glGenTextures(1, &m_id);
			glTextureView(m_id, target, data->id(), sized_format, 0, num_levels, 0, num_layers);

			if (argb_swizzle)
			{
				component_swizzle[0] = argb_swizzle[1];
				component_swizzle[1] = argb_swizzle[2];
				component_swizzle[2] = argb_swizzle[3];
				component_swizzle[3] = argb_swizzle[0];

				glBindTexture(m_target, m_id);
				glTexParameteriv(m_target, GL_TEXTURE_SWIZZLE_RGBA, (GLint*)component_swizzle);
			}
			else
			{
				component_swizzle[0] = GL_RED;
				component_swizzle[1] = GL_GREEN;
				component_swizzle[2] = GL_BLUE;
				component_swizzle[3] = GL_ALPHA;
			}

			if (aspect_flags & image_aspect::stencil)
			{
				constexpr u32 depth_stencil_mask = (image_aspect::depth | image_aspect::stencil);
				verify("Invalid aspect mask combination" HERE), (aspect_flags & depth_stencil_mask) != depth_stencil_mask;

				glBindTexture(m_target, m_id);
				glTexParameteri(m_target, GL_DEPTH_STENCIL_TEXTURE_MODE, GL_STENCIL_INDEX);
			}
		}

	public:
		texture_view(const texture_view&) = delete;
		texture_view(texture_view&&) = delete;

		texture_view(texture* data, GLenum target, GLenum sized_format,
			const GLenum* argb_swizzle = nullptr,
			GLenum aspect_flags = image_aspect::color | image_aspect::depth)
		{
			create(data, target, sized_format, aspect_flags, argb_swizzle);
		}

		texture_view(texture* data, const GLenum* argb_swizzle = nullptr,
			GLenum aspect_flags = image_aspect::color | image_aspect::depth)
		{
			GLenum target = (GLenum)data->get_target();
			GLenum sized_format = (GLenum)data->get_internal_format();
			create(data, target, sized_format, aspect_flags, argb_swizzle);
		}

		~texture_view()
		{
			glDeleteTextures(1, &m_id);
		}

		GLuint id() const
		{
			return m_id;
		}

		GLenum target() const
		{
			return m_target;
		}

		GLenum internal_format() const
		{
			return m_format;
		}

		GLenum aspect() const
		{
			return m_aspect_flags;
		}

		bool compare_swizzle(GLenum* argb_swizzle) const
		{
			return (argb_swizzle[0] == component_swizzle[3] &&
				argb_swizzle[1] == component_swizzle[0] &&
				argb_swizzle[2] == component_swizzle[1] &&
				argb_swizzle[3] == component_swizzle[2]);
		}

		void bind() const
		{
			glBindTexture(m_target, m_id);
		}

		texture* image() const
		{
			return m_image_data;
		}

		std::array<GLenum, 4> component_mapping() const
		{
			return{ component_swizzle[3], component_swizzle[0], component_swizzle[1], component_swizzle[2] };
		}

		u32 encoded_component_map() const
		{
			// Unused, OGL supports proper component swizzles
			return 0u;
		}
	};

	class viewable_image : public texture
	{
		std::unordered_multimap<u32, std::unique_ptr<texture_view>> views;

public:
		using texture::texture;

		texture_view* get_view(u32 remap_encoding, const std::pair<std::array<u8, 4>, std::array<u8, 4>>& remap, GLenum aspect_flags = image_aspect::color | image_aspect::depth)
		{
			auto found = views.equal_range(remap_encoding);
			for (auto It = found.first; It != found.second; ++It)
			{
				if (It->second->aspect() & aspect_flags)
				{
					return It->second.get();
				}
			}

			auto mapping = apply_swizzle_remap(get_native_component_layout(), remap);
			auto view = std::make_unique<texture_view>(this, mapping.data(), aspect_flags);
			auto result = view.get();
			views.emplace(remap_encoding, std::move(view));
			return result;
		}

		void set_native_component_layout(const std::array<GLenum, 4>& layout)
		{
			if (m_component_layout[0] != layout[0] ||
				m_component_layout[1] != layout[1] ||
				m_component_layout[2] != layout[2] ||
				m_component_layout[3] != layout[3])
			{
				texture::set_native_component_layout(layout);
				views.clear();
			}
		}
	};

	class rbo
	{
		GLuint m_id = 0;

	public:
		rbo() = default;

		rbo(GLuint id)
		{
			set_id(id);
		}

		~rbo()
		{
			if (created())
				remove();
		}

		class save_binding_state
		{
			GLint m_old_value;

		public:
			save_binding_state(const rbo& new_state)
			{
				glGetIntegerv(GL_RENDERBUFFER_BINDING, &m_old_value);
				new_state.bind();
			}

			~save_binding_state()
			{
				glBindRenderbuffer(GL_RENDERBUFFER, m_old_value);
			}
		};

		void recreate()
		{
			if (created())
				remove();

			create();
		}

		void recreate(texture::format format, u32 width, u32 height)
		{
			if (created())
				remove();

			create(format, width, height);
		}

		void create()
		{
			glGenRenderbuffers(1, &m_id);
		}

		void create(texture::format format, u32 width, u32 height)
		{
			create();
			storage(format, width, height);
		}

		void bind() const
		{
			glBindRenderbuffer(GL_RENDERBUFFER, m_id);
		}

		void storage(texture::format format, u32 width, u32 height)
		{
			save_binding_state save(*this);
			glRenderbufferStorage(GL_RENDERBUFFER, (GLenum)format, width, height);
		}

		void remove()
		{
			glDeleteRenderbuffers(1, &m_id);
			m_id = 0;
		}

		uint id() const
		{
			return m_id;
		}

		void set_id(uint id)
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

	enum class indices_type
	{
		ubyte = GL_UNSIGNED_BYTE,
		ushort = GL_UNSIGNED_SHORT,
		uint = GL_UNSIGNED_INT
	};

	class fbo
	{
		GLuint m_id = GL_NONE;
		size2i m_size;

	protected:
		std::unordered_map<GLenum, GLuint> m_resource_bindings;

	public:
		fbo() = default;

		fbo(GLuint id)
		{
			set_id(id);
		}

		~fbo()
		{
			if (created())
				remove();
		}

		class save_binding_state
		{
			GLint m_last_binding;
			bool reset = true;

		public:
			save_binding_state(const fbo& new_binding)
			{
				glGetIntegerv(GL_FRAMEBUFFER_BINDING, &m_last_binding);

				if (m_last_binding != new_binding.id())
					new_binding.bind();
				else
					reset = false;
			}

			~save_binding_state()
			{
				if (reset)
					glBindFramebuffer(GL_FRAMEBUFFER, m_last_binding);
			}
		};

		class attachment
		{
		public:
			enum class type
			{
				color = GL_COLOR_ATTACHMENT0,
				depth = GL_DEPTH_ATTACHMENT,
				stencil = GL_STENCIL_ATTACHMENT,
				depth_stencil = GL_DEPTH_STENCIL_ATTACHMENT
			};

		protected:
			GLuint m_id = GL_NONE;
			fbo &m_parent;

		public:
			attachment(fbo& parent, type type)
				: m_id((int)type)
				, m_parent(parent)
			{
			}

			void set_id(uint id)
			{
				m_id = id;
			}

			uint id() const
			{
				return m_id;
			}

			GLuint resource_id() const
			{
				const auto found = m_parent.m_resource_bindings.find(m_id);
				if (found != m_parent.m_resource_bindings.end())
				{
					return found->second;
				}

				return GL_NONE;
			}

			void operator = (const rbo& rhs)
			{
				save_binding_state save(m_parent);
				m_parent.m_resource_bindings[m_id] = rhs.id();
				glFramebufferRenderbuffer(GL_FRAMEBUFFER, m_id, GL_RENDERBUFFER, rhs.id());
			}

			void operator = (const texture& rhs)
			{
				save_binding_state save(m_parent);

				verify(HERE), rhs.get_target() == texture::target::texture2D;
				m_parent.m_resource_bindings[m_id] = rhs.id();
				glFramebufferTexture2D(GL_FRAMEBUFFER, m_id, GL_TEXTURE_2D, rhs.id(), 0);
			}

			void operator = (const GLuint rhs)
			{
				save_binding_state save(m_parent);
				m_parent.m_resource_bindings[m_id] = rhs;
				glFramebufferTexture2D(GL_FRAMEBUFFER, m_id, GL_TEXTURE_2D, rhs, 0);
			}
		};

		class indexed_attachment : public attachment
		{
		public:
			indexed_attachment(fbo& parent, type type) : attachment(parent, type)
			{
			}

			attachment operator[](int index) const
			{
				return{ m_parent, type(id() + index) };
			}

			std::vector<attachment> range(int from, int count) const
			{
				std::vector<attachment> result;

				for (int i = from; i < from + count; ++i)
					result.push_back((*this)[i]);

				return result;
			}

			using attachment::operator =;
		};

		indexed_attachment color{ *this, attachment::type::color };
		attachment depth{ *this, attachment::type::depth };
		attachment stencil{ *this, attachment::type::stencil };
		attachment depth_stencil{ *this, attachment::type::depth_stencil };

		enum class target
		{
			read_frame_buffer = GL_READ_FRAMEBUFFER,
			draw_frame_buffer = GL_DRAW_FRAMEBUFFER
		};

		void create();
		void bind() const;
		void blit(const fbo& dst, areai src_area, areai dst_area, buffers buffers_ = buffers::color, filter filter_ = filter::nearest) const;
		void bind_as(target target_) const;
		void remove();
		bool created() const;
		bool check() const;

		void recreate();
		void draw_buffer(const attachment& buffer) const;
		void draw_buffers(const std::initializer_list<attachment>& indexes) const;

		void read_buffer(const attachment& buffer) const;

		void draw_arrays(rsx::primitive_type mode, GLsizei count, GLint first = 0) const;
		void draw_arrays(const buffer& buffer, rsx::primitive_type mode, GLsizei count, GLint first = 0) const;
		void draw_arrays(const vao& buffer, rsx::primitive_type mode, GLsizei count, GLint first = 0) const;

		void draw_elements(rsx::primitive_type mode, GLsizei count, indices_type type, const GLvoid *indices) const;
		void draw_elements(const buffer& buffer, rsx::primitive_type mode, GLsizei count, indices_type type, const GLvoid *indices) const;
		void draw_elements(rsx::primitive_type mode, GLsizei count, indices_type type, const buffer& indices, size_t indices_buffer_offset = 0) const;
		void draw_elements(const buffer& buffer_, rsx::primitive_type mode, GLsizei count, indices_type type, const buffer& indices, size_t indices_buffer_offset = 0) const;
		void draw_elements(rsx::primitive_type mode, GLsizei count, const GLubyte *indices) const;
		void draw_elements(const buffer& buffer, rsx::primitive_type mode, GLsizei count, const GLubyte *indices) const;
		void draw_elements(rsx::primitive_type mode, GLsizei count, const GLushort *indices) const;
		void draw_elements(const buffer& buffer, rsx::primitive_type mode, GLsizei count, const GLushort *indices) const;
		void draw_elements(rsx::primitive_type mode, GLsizei count, const GLuint *indices) const;
		void draw_elements(const buffer& buffer, rsx::primitive_type mode, GLsizei count, const GLuint *indices) const;

		void clear(buffers buffers_) const;
		void clear(buffers buffers_, color4f color_value, double depth_value, u8 stencil_value) const;

		void copy_from(const void* pixels, sizei size, gl::texture::format format_, gl::texture::type type_, class pixel_unpack_settings pixel_settings = pixel_unpack_settings()) const;
		void copy_from(const buffer& buf, sizei size, gl::texture::format format_, gl::texture::type type_, class pixel_unpack_settings pixel_settings = pixel_unpack_settings()) const;

		void copy_to(void* pixels, coordi coord, gl::texture::format format_, gl::texture::type type_, class pixel_pack_settings pixel_settings = pixel_pack_settings()) const;
		void copy_to(const buffer& buf, coordi coord, gl::texture::format format_, gl::texture::type type_, class pixel_pack_settings pixel_settings = pixel_pack_settings()) const;

		static fbo get_binded_draw_buffer();
		static fbo get_binded_read_buffer();
		static fbo get_binded_buffer();

		GLuint id() const;
		void set_id(GLuint id);

		void set_extents(size2i extents);
		size2i get_extents() const;

		bool matches(const std::array<GLuint, 4>& color_targets, GLuint depth_stencil_target) const;
		bool references_any(const std::vector<GLuint>& resources) const;

		explicit operator bool() const
		{
			return created();
		}
	};

	extern const fbo screen;

	namespace glsl
	{
		class compilation_exception : public exception
		{
		public:
			explicit compilation_exception(const std::string& what_arg)
			{
				m_what = "compilation failed: '" + what_arg + "'";
			}
		};

		class link_exception : public exception
		{
		public:
			explicit link_exception(const std::string& what_arg)
			{
				m_what = "linkage failed: '" + what_arg + "'";
			}
		};

		class validation_exception : public exception
		{
		public:
			explicit validation_exception(const std::string& what_arg)
			{
				m_what = "compilation failed: '" + what_arg + "'";
			}
		};

		class not_found_exception : public exception
		{
		public:
			explicit not_found_exception(const std::string& what_arg)
			{
				m_what = what_arg + " not found.";
			}
		};

		class shader
		{
		public:
			enum class type
			{
				fragment = GL_FRAGMENT_SHADER,
				vertex = GL_VERTEX_SHADER,
				geometry = GL_GEOMETRY_SHADER
			};

		private:

			GLuint m_id = GL_NONE;
			type shader_type = type::vertex;

		public:

			shader() = default;

			shader(GLuint id)
			{
				set_id(id);
			}

			shader(type type_)
			{
				create(type_);
			}

			shader(type type_, const std::string& src)
			{
				create(type_);
				source(src);
			}

			~shader()
			{
				if (created())
					remove();
			}

			void recreate(type type_)
			{
				if (created())
					remove();

				create(type_);
			}

			void create(type type_)
			{
				m_id = glCreateShader((GLenum)type_);
				shader_type = type_;
			}

			void source(const std::string& src) const
			{
				const char* str = src.c_str();
				const GLint length = (GLint)src.length();

				{
					std::string base_name;
					switch (shader_type)
					{
					case type::vertex:
						base_name = "shaderlog/VertexProgram";
						break;
					case type::fragment:
						base_name = "shaderlog/FragmentProgram";
						break;
					case type::geometry:
						base_name = "shaderlog/GeometryProgram";
						break;
					}

					fs::file(fs::get_config_dir() + base_name + std::to_string(m_id) + ".glsl", fs::rewrite).write(str);
				}

				glShaderSource(m_id, 1, &str, &length);
			}

			shader& compile()
			{
				glCompileShader(m_id);

				GLint status = GL_FALSE;
				glGetShaderiv(m_id, GL_COMPILE_STATUS, &status);

				if (status == GL_FALSE)
				{
					GLint length = 0;
					glGetShaderiv(m_id, GL_INFO_LOG_LENGTH, &length);

					std::string error_msg;
					if (length)
					{
						std::unique_ptr<GLchar[]> buf(new char[length + 1]);
						glGetShaderInfoLog(m_id, length, nullptr, buf.get());
						error_msg = buf.get();
					}

					throw compilation_exception(error_msg);
				}

				return *this;
			}

			void remove()
			{
				glDeleteShader(m_id);
				m_id = 0;
			}

			uint id() const
			{
				return m_id;
			}

			void set_id(uint id)
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

		class program
		{
			GLuint m_id = 0;

		public:
			class uniform_t
			{
				program& m_program;
				GLint m_location;

			public:
				uniform_t(program& program, GLint location)
					: m_program(program)
					, m_location(location)
				{
				}

				GLint location() const
				{
					return m_location;
				}

				void operator = (int rhs) const { glProgramUniform1i(m_program.id(), location(), rhs); }
				void operator = (float rhs) const { glProgramUniform1f(m_program.id(), location(), rhs); }
				void operator = (const color1i& rhs) const { glProgramUniform1i(m_program.id(), location(), rhs.r); }
				void operator = (const color1f& rhs) const { glProgramUniform1f(m_program.id(), location(), rhs.r); }
				void operator = (const color2i& rhs) const { glProgramUniform2i(m_program.id(), location(), rhs.r, rhs.g); }
				void operator = (const color2f& rhs) const { glProgramUniform2f(m_program.id(), location(), rhs.r, rhs.g); }
				void operator = (const color3i& rhs) const { glProgramUniform3i(m_program.id(), location(), rhs.r, rhs.g, rhs.b); }
				void operator = (const color3f& rhs) const { glProgramUniform3f(m_program.id(), location(), rhs.r, rhs.g, rhs.b); }
				void operator = (const color4i& rhs) const { glProgramUniform4i(m_program.id(), location(), rhs.r, rhs.g, rhs.b, rhs.a); }
				void operator = (const color4f& rhs) const { glProgramUniform4f(m_program.id(), location(), rhs.r, rhs.g, rhs.b, rhs.a); }
				void operator = (const areaf& rhs) const { glProgramUniform4f(m_program.id(), location(), rhs.x1, rhs.y1, rhs.x2, rhs.y2); }
				void operator = (const areai& rhs) const { glProgramUniform4i(m_program.id(), location(), rhs.x1, rhs.y1, rhs.x2, rhs.y2); }
			};

			class uniforms_t
			{
				program& m_program;
				std::unordered_map<std::string, GLint> locations;
				int active_texture = 0;

			public:
				uniforms_t(program* program) : m_program(*program)
				{
				}

				void clear()
				{
					locations.clear();
					active_texture = 0;
				}

				bool has_location(const std::string &name, int *location = nullptr)
				{
					auto found = locations.find(name);
					if (found != locations.end())
					{
						if (location)
						{
							*location = found->second;
						}

						return (found->second >= 0);
					}

					auto result = glGetUniformLocation(m_program.id(), name.c_str());
					locations[name] = result;

					if (location)
					{
						*location = result;
					}

					return (result >= 0);
				}

				GLint location(const std::string &name)
				{
					auto found = locations.find(name);
					if (found != locations.end())
					{
						if (found->second >= 0)
						{
							return found->second;
						}
						else
						{
							throw not_found_exception(name);
						}
					}

					auto result = glGetUniformLocation(m_program.id(), name.c_str());

					if (result < 0)
					{
						throw not_found_exception(name);
					}

					locations[name] = result;
					return result;
				}

				int texture(GLint location, int active_texture, const gl::texture_view& texture)
				{
					glActiveTexture(GL_TEXTURE0 + active_texture);
					texture.bind();
					(*this)[location] = active_texture;

					return active_texture;
				}

				uniform_t operator[](GLint location)
				{
					return{ m_program, location };
				}

				uniform_t operator[](const std::string &name)
				{
					return{ m_program, location(name) };
				}

				void swap(uniforms_t& uniforms)
				{
					locations.swap(uniforms.locations);
					std::swap(active_texture, uniforms.active_texture);
				}
			} uniforms{ this };

			program& recreate()
			{
				if (created())
					remove();

				return create();
			}

			program& create()
			{
				m_id = glCreateProgram();
				return *this;
			}

			void remove()
			{
				glDeleteProgram(m_id);
				m_id = 0;
				uniforms.clear();
			}

			static program get_current_program()
			{
				GLint id;
				glGetIntegerv(GL_CURRENT_PROGRAM, &id);
				return{ (GLuint)id };
			}

			void use()
			{
				glUseProgram(m_id);
			}

			void link()
			{
				glLinkProgram(m_id);

				GLint status = GL_FALSE;
				glGetProgramiv(m_id, GL_LINK_STATUS, &status);

				if (status == GL_FALSE)
				{
					GLint length = 0;
					glGetProgramiv(m_id, GL_INFO_LOG_LENGTH, &length);

					std::string error_msg;
					if (length)
					{
						std::unique_ptr<GLchar[]> buf(new char[length + 1]);
						glGetProgramInfoLog(m_id, length, nullptr, buf.get());
						error_msg = buf.get();
					}

					throw link_exception(error_msg);
				}
			}

			void validate()
			{
				glValidateProgram(m_id);

				GLint status = GL_FALSE;
				glGetProgramiv(m_id, GL_VALIDATE_STATUS, &status);

				if (status == GL_FALSE)
				{
					GLint length = 0;
					glGetProgramiv(m_id, GL_INFO_LOG_LENGTH, &length);

					std::string error_msg;
					if (length)
					{
						std::unique_ptr<GLchar[]> buf(new char[length + 1]);
						glGetProgramInfoLog(m_id, length, nullptr, buf.get());
						error_msg = buf.get();
					}

					LOG_ERROR(RSX, "Validation failed: %s", error_msg.c_str());
				}
			}

			void make()
			{
				link();
			}

			uint id() const
			{
				return m_id;
			}

			void set_id(uint id)
			{
				uniforms.clear();
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

			program& attach(const shader& shader_)
			{
				glAttachShader(m_id, shader_.id());
				return *this;
			}

			program& bind_attribute_location(const std::string& name, int index)
			{
				glBindAttribLocation(m_id, index, name.c_str());
				return *this;
			}

			program& bind_fragment_data_location(const std::string& name, int color_number)
			{
				glBindFragDataLocation(m_id, color_number, name.c_str());
				return *this;
			}

			int attribute_location(const std::string& name)
			{
				return glGetAttribLocation(m_id, name.c_str());
			}

			int uniform_location(const std::string& name)
			{
				return glGetUniformLocation(m_id, name.c_str());
			}

			program& operator += (const shader& rhs)
			{
				return attach(rhs);
			}

			program& operator += (std::initializer_list<shader> shaders)
			{
				for (auto &shader : shaders)
					*this += shader;
				return *this;
			}

			program() = default;
			program(const program&) = delete;
			program(program&& program_)
			{
				swap(program_);
			}

			program(GLuint id)
			{
				set_id(id);
			}

			~program()
			{
				if (created())
					remove();
			}

			void swap(program& program_)
			{
				auto my_old_id = id();
				set_id(program_.id());
				program_.set_id(my_old_id);
				uniforms.swap(program_.uniforms);
			}

			program& operator = (const program& rhs) = delete;
			program& operator = (program&& rhs)
			{
				swap(rhs);
				return *this;
			}
		};

		class shader_view : public shader
		{
		public:
			shader_view(GLuint id) : shader(id)
			{
			}

			~shader_view()
			{
				set_id(0);
			}
		};

		class program_view : public program
		{
		public:
			program_view(GLuint id) : program(id)
			{
			}

			~program_view()
			{
				set_id(0);
			}
		};
	}
}
