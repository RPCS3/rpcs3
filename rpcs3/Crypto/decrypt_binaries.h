#pragma once

class decrypt_binaries_t
{
    std::vector<u128> m_klics;
    std::vector<std::string> m_modules;
    usz m_index = 0;

public:
    decrypt_binaries_t(std::vector<std::string> modules) noexcept
        : m_modules(std::move(modules))
    {
    }

    usz decrypt(std::string_view klic_input = {});

    bool done() const
    {
        return m_index >= m_modules.size();
    }

    const std::string& operator[](usz index) const
    {
        return ::at32(m_modules, index);
    }
};
