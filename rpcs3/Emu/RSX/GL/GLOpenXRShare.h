#pragma once

#ifdef HAVE_OPENXR

#include "glutils/fbo.h"
#include "Utilities/geometry.h"
#include "Emu/Io/openxr/openxr_frame_share.h"

#include <array>
#include <functional>
#include <memory>
#include <vector>

namespace gl
{
	class command_context;
	class texture;

	// Copies the presented frames into the images shared by the OpenXR handler (see Emu/Io/openxr/openxr_frame_share.h).
	// This is the OpenGL counterpart of vk::openxr_frame_share. It needs EXT_memory_object and EXT_semaphore.
	class openxr_frame_share
	{
	public:
		// Draws the overlays into the texture, using area as the screen area
		using overlay_callback = std::function<void(gl::texture* dst, const areau& area)>;

		// Has to be called with the OpenGL context of RSX
		void destroy();

		// Hands finished copies over to the OpenXR handler and copies the presented frame into a free slot.
		// right_image is only used if eyes.separate_images is set.
		// If draw_overlays is set, the overlays are drawn into a copy of the images first, since RSX draws them directly to the window.
		// Has to be called once per flip with the OpenGL context of RSX. Changes the framebuffer bindings, so the caller has to bind its framebuffer again.
		void present(gl::command_context& cmd, gl::texture* left_image, gl::texture* right_image, const openxr::eye_layout& eyes, const overlay_callback& draw_overlays);

		// Returns a black texture of the given size. It's used as the frame while RSX has nothing to present,
		// so that the overlays (e.g. the progress dialogs while shaders are compiled) are shown in the headset as well.
		gl::texture* get_blank_texture(gl::command_context& cmd, u32 width, u32 height);

	private:
		gl::texture* add_overlays(gl::command_context& cmd, gl::texture* src, u32 index, std::initializer_list<areai> areas, const overlay_callback& draw_overlays);

		std::array<std::unique_ptr<gl::texture>, 2> m_overlay_textures; // Copies of the eye images with overlays
		std::unique_ptr<gl::texture> m_blank_texture; // See get_blank_texture

		struct imported_slot
		{
			GLuint memory = GL_NONE;
			GLuint texture = GL_NONE;
			GLuint semaphore = GL_NONE;         // Signaled by us after writing
			GLuint release_semaphore = GL_NONE; // Signaled by OpenXR after reading
			GLsync fence = nullptr; // Set while a copy is in flight
		};

		bool update_imports(gl::command_context& cmd);
		bool import_slots(gl::command_context& cmd);
		void destroy_slots();
		void check_finished_copies();

		std::vector<imported_slot> m_slots;
		u64 m_generation = 0;
		bool m_import_failed = false; // Only try once per generation

		fbo m_blit_src;
		fbo m_blit_dst;
	};
}

#endif
