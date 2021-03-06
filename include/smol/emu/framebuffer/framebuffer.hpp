#pragma once

#ifdef SMOLISA_FRAMEBUFFER

#	include "smol/common/types.hpp"

#	include <SFML/Graphics.hpp>
#	include <cstddef>
#	include <optional>
#	include <vector>

struct FrameBufferConfig
{
	std::string_view font_path = "./assets/fontsheet.png";
};

class FrameBuffer
{
	public:
	enum class Region
	{
		FrameData,
		PaletteData,
		VsyncWait,
		Invalid
	};

	struct Char
	{
		char code;
		char palette_front_entry : 4, palette_back_entry : 4;
	};

	struct PaletteEntry
	{
		Byte r, g, b;
	};

	static constexpr Addr pixel_data_address     = 0x0000;
	static constexpr Addr pixel_data_end_address = 0x0F9F;

	static constexpr Addr palette_address     = 0x0FA0;
	static constexpr Addr palette_end_address = 0x0FCF;

	static constexpr Addr vsync_wait_address = 0x0FD0;

	static constexpr std::size_t width = 80, height = 25;

	static constexpr std::size_t sizeof_fb_char       = 2;
	static constexpr std::size_t sizeof_palette_entry = 3;

	auto get_char(std::size_t x, std::size_t y) const -> Char;

	void set_palette_entry(std::size_t index, PaletteEntry entry);
	auto get_palette_entry(std::size_t index) const -> PaletteEntry;

	explicit FrameBuffer(FrameBufferConfig config = {});

	void clear();
	void rebuild();
	auto display() -> bool;

	void update_char(Char c, std::size_t x, std::size_t y);

	void display_simple_string(std::string_view s, std::size_t origin_x, std::size_t origin_y);

	static auto byte_region(Addr a) -> Region;

	auto set_byte(Addr a, Byte b) -> bool;
	auto get_byte(Addr a) const -> std::optional<Byte>;

	private:
	std::vector<char> m_character_data;
	std::vector<char> m_palette_data;
	sf::Image         m_image;
	sf::Image         m_font;

	sf::RenderWindow m_window;

	std::size_t m_glyph_width, m_glyph_height;
};

#endif
