#pragma once

#include <span>
#include <cstdint>
#include <cstring>
#include <type_traits>
#include <stdexcept>

namespace EntitySystem
{
    struct EntityComponent;
}

namespace EntityReader
{
    class BufferReader
    {
    public:
        BufferReader(std::span<const uint8_t> buffer)
            : m_buffer(buffer), m_offset(0) 
        {
        }

        template<typename T>
        T Read()
        {
            static_assert(std::is_trivially_copyable_v<T>, "T must be trivially copyable");
            if (m_offset + sizeof(T) > m_buffer.size())
                throw std::out_of_range("BufferReader: Not enough data to read type");

            T value;
            std::memcpy(&value, m_buffer.data() + m_offset, sizeof(T));
            m_offset += sizeof(T);
            return value;
        }

        BufferReader ReadBuffer(size_t length)
        {
            if (m_offset + length > m_buffer.size())
                throw std::out_of_range("BufferReader: Not enough data to read span");

            BufferReader subReader(m_buffer.subspan(m_offset, length));
            m_offset += length;
            return subReader;
        }

        uint8_t* Data()
        {
            return const_cast<uint8_t*>(m_buffer.data());
        }

        size_t Size() const { return m_buffer.size(); }

        size_t Remaining() const { return m_buffer.size() - m_offset; }
        size_t Offset() const { return m_offset; }

        bool Done() const { return m_offset >= m_buffer.size(); };

    private:
        std::span<const uint8_t> m_buffer;
        size_t m_offset;
    };

    // EntityReader reads entity/component data from a binary file using the resource manager file type.
    class Reader
    {
    public:
        // Reads entities and components from a resource file (ResourceManager).
        // Each entity is created with the ID from the file, and components are added by component ID.
        // For each component, the buffer is passed to OnComponentData for initialization.
        void ReadEntitiesFromResource(size_t resourceHash);

    protected:
        // Called for each created component, passing the buffer with component data.
        // Override this in derived classes to handle component-specific deserialization.
        virtual void OnComponentData(EntitySystem::EntityComponent* component, size_t componentId, BufferReader& buffer) = 0;
    };

}