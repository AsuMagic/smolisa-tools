#include "smol/asm/tokenizer.hpp"

#include <fmt/core.h>

Tokenizer::Tokenizer(std::string_view source) :
	m_source{source}, m_it{m_source.begin()}, m_token_begin_it{m_source.begin()}
{}

auto Tokenizer::consume_token() -> Token
{
	if (read() == 0)
	{
		return tokens::Eof{};
	}

	skip_spaces();

	if (m_last == ';')
	{
		while (m_last != '\n' && (read() != 0))
		{
		}
	}

	if (is_newline(m_last))
	{
		return tokens::Newline{};
	}

	if (m_last == ':')
	{
		return tokens::Colon{};
	}

	m_token_begin_it = m_it - 1;

	// Assembler directive
	if (m_last == '#')
	{
		while (is_alpha(read()))
		{
		}

		--m_it;

		const auto str = token_string();

		if (str == "#binary")
		{
			return tokens::Directive::IncludeBinaryFile;
		}

		if (str == "#offset")
		{
			m_token_begin_it = m_it;
			return tokens::Directive::ByteOffset;
		}

		return std::monostate{};
	}

	if (m_last == '\'')
	{
		const char ascii = read();

		if (read() == '\'')
		{
			return tokens::Immediate{Byte(ascii)};
		}

		return std::monostate{};
	}

	if (m_last == '\"')
	{
		return tokens::StringLiteral{parse_string_literal()};
	}

	if (m_last == '~')
	{
		while (is_alpha(read()))
		{
		}

		if (m_it != m_source.end())
		{
			// Backtrack just once
			--m_it;
		}

		const auto str = token_string();

		if (str == "~low")
		{
			return tokens::ByteSelector{.is_upper_byte = false};
		}

		if (str == "~high")
		{
			return tokens::ByteSelector{.is_upper_byte = true};
		}

		return std::monostate{};
	}

	if (is_identifier_begin(m_last) || m_last == '$')
	{
		while (is_identifier(read()))
		{
		}

		if (m_it != m_source.end())
		{
			// Backtrack just once
			--m_it;
		}

		auto str = token_string();

		if (const auto it = std::find_if(
				instruction_infos.begin(),
				instruction_infos.end(),
				[&](const auto& info) { return info.mnemonic == str; });
			it != instruction_infos.end())
		{
			return tokens::Mnemonic{Opcode(std::distance(instruction_infos.begin(), it))};
		}

		if (const auto it = std::find(register_infos.begin(), register_infos.end(), str); it != register_infos.end())
		{
			return tokens::RegisterReference{RegisterId(std::distance(register_infos.begin(), it))};
		}

		return tokens::Label{str};
	}

	if (is_digit(m_last))
	{
		return tokens::Immediate{parse_integral()};
	}

	// Check one last time whether we got EOF
	if (m_it == m_source.end())
	{
		return tokens::Eof{};
	}

	return std::monostate{};
}

void Tokenizer::dump_context() const
{
	fmt::print(
		stderr,
		"String: {}\nIt pair ({}, {})\n",
		token_string(),
		std::distance(m_source.begin(), m_it),
		std::distance(m_source.begin(), m_token_begin_it));
}

auto Tokenizer::read() -> char
{
	if (m_it != m_source.end())
	{
		m_last = *m_it;
		++m_it;

		return m_last;
	}

	m_last = '\0';
	return m_last;
}

auto Tokenizer::token_string() const -> std::string_view
{
	return {&*m_token_begin_it, std::size_t(std::distance(m_token_begin_it, m_it))};
}

auto Tokenizer::parse_integral() -> std::size_t
{
	while (!is_space(m_last) && !is_newline(m_last) && (read() != 0))
	{
	}

	if (m_it != m_source.end())
	{
		// Backtrack just once
		--m_it;
	}

	auto integral_string = token_string();

	int base = 10;

	if (integral_string.starts_with("0x"))
	{
		integral_string.remove_prefix(2);
		base = 16;
	}

	try
	{
		return std::stoul(std::string{integral_string}, nullptr, base);
	}
	catch (const std::exception& e)
	{
		throw;
	}
}

auto Tokenizer::parse_string_literal() -> std::string_view
{
	m_token_begin_it = m_it;
	read();

	while (m_last != '"' && !is_newline(read()) && m_it != m_source.end())
	{
	}

	if (m_last != '"')
	{
		throw std::runtime_error{"String literal must end with double quotes '\"' before the end of the line"};
	}

	auto offset_string = token_string();
	offset_string.remove_suffix(1);

	return offset_string;
}

void Tokenizer::skip_spaces()
{
	// Strip whitespace
	while (is_space(m_last) && (read() != 0))
	{
	}
}
