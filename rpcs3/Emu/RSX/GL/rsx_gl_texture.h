
namespace rsx
{
	class texture;

	namespace gl
	{
		class texture
		{
			u32 m_id = 0;

		public:
			void create();

			int gl_wrap(int wrap);

			float max_aniso(int aniso);

			inline static u8 convert_4_to_8(u8 v)
			{
				// Swizzle bits: 00001234 -> 12341234
				return (v << 4) | (v);
			}

			inline static u8 convert_5_to_8(u8 v)
			{
				// Swizzle bits: 00012345 -> 12345123
				return (v << 3) | (v >> 2);
			}

			inline static u8 convert_6_to_8(u8 v)
			{
				// Swizzle bits: 00123456 -> 12345612
				return (v << 2) | (v >> 4);
			}

			void init(int index, rsx::texture& tex);
			void bind();
			void unbind();
			void remove();

			u32 id() const;
		};
	}
}
