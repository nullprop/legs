#pragma once

#include <cstring>
#include <memory>

#include <legs/renderer/vma_usage.hpp>

namespace legs
{
enum BufferType
{
    VertexBuffer,
    IndexBuffer,
    UniformBuffer,
};

enum BufferLocation
{
    HostBuffer,
    DeviceBuffer,
};

class Buffer
{
  public:
    Buffer() = delete;
    Buffer(
        BufferType     bufferType,
        BufferLocation bufferLocation,
        uint32_t       elementSize,
        uint32_t       elementCount
    );

    ~Buffer();

    Buffer(const Buffer&)            = delete;
    Buffer(Buffer&&)                 = delete;
    Buffer& operator=(const Buffer&) = delete;
    Buffer& operator=(Buffer&&)      = delete;

    template<class IT>
    requires std::contiguous_iterator<IT>
    void Write(const IT it, uint32_t length)
    {
        if (m_bufferLocation != HostBuffer)
        {
            throw std::runtime_error("Tried mapping a non-host buffer");
        }

        size_t size = length * m_elementSize;
        if (size > m_size)
        {
            throw std::runtime_error("Tried to write more data than allocated");
        }

        Write(static_cast<void*>(it), size);
        m_elementCount = length;
    }

    void Write(void* data, size_t size);

    void CopyToDevice(void* commandBuffer, std::shared_ptr<Buffer> deviceBuffer);

    void Clear();

    void Bind(void* commandBuffer);

    void Draw(void* commandBuffer);
    void Draw(void* commandBuffer, uint32_t indexOffset, uint32_t indexCount, int32_t vertexOffset);

    void Map(void** data);
    void Unmap();

    VkBuffer GetVkBuffer() const
    {
        return m_vkBuffer;
    }

    void SetElementCount(uint32_t count)
    {
        m_elementCount = count;
    }

    uint32_t GetElementCount() const
    {
        return m_elementCount;
    }

    constexpr BufferType GetType() const
    {
        return m_bufferType;
    }

    BufferLocation GetLocation() const
    {
        return m_bufferLocation;
    }

    size_t GetSize() const
    {
        return m_size;
    }

    size_t GetElementSize()
    {
        return m_elementSize;
    }

  private:
    VkBuffer      m_vkBuffer;
    VmaAllocation m_vmaAllocation;

    BufferType     m_bufferType;
    BufferLocation m_bufferLocation;
    uint32_t       m_elementSize;
    uint32_t       m_elementCount;
    size_t         m_size;
    bool           m_isMapped = false;
};
} // namespace legs
