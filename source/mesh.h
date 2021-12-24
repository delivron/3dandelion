#pragma once

#include <span>
#include <vector>
#include <cstdint>
#include <utility>

namespace ddn
{

class IMesh
{
public:
    virtual std::span<const uint8_t> GetVertices() const = 0;
    virtual size_t GetVertexSize() const = 0;
    virtual size_t GetVertexCount() const = 0;

    virtual std::span<const uint8_t> GetIndexes() const = 0;
    virtual size_t GetIndexSize() const = 0;
    virtual size_t GetIndexCount() const = 0;
};

template <typename Vertex, typename Index>
class Mesh : public IMesh
{
public:
    Mesh(std::vector<Vertex>&& vertices, std::vector<Index>&& indexes)
        : m_vertices(std::move(vertices))
        , m_indexes(std::move(indexes))
    {}

    std::span<const uint8_t> GetVertices() const override {
        return CreateBuffer(m_vertices);
    }

    size_t GetVertexSize() const override {
        return sizeof(Vertex);
    }

    size_t GetVertexCount() const override {
        return m_vertices.size();
    }

    std::span<const uint8_t> GetIndexes() const override {
        return CreateBuffer(m_indexes);
    }

    size_t GetIndexSize() const override {
        return sizeof(Index);
    }

    size_t GetIndexCount() const override {
        return m_indexes.size();
    }

private:
    template <typename T>
    std::span<const uint8_t> CreateBuffer(const std::vector<T>& elements) const {
        return std::span(reinterpret_cast<const uint8_t*>(elements.data()), sizeof(T) * elements.size());
    }

    std::vector<Vertex> m_vertices;
    std::vector<Index> m_indexes;
};

}  // namespace ddn
