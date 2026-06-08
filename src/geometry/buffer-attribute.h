#pragma once
#include "common.h"

namespace THREE {

enum class AttributeType { Float32, Uint32, Int32, Uint16, Int16, Uint8, Int8 };

inline std::size_t attribute_type_size(AttributeType type) {
    switch (type) {
        case AttributeType::Float32: return 4;
        case AttributeType::Uint32: return 4;
        case AttributeType::Int32: return 4;
        case AttributeType::Uint16: return 2;
        case AttributeType::Int16: return 2;
        case AttributeType::Uint8: return 1;
        case AttributeType::Int8: return 1;
    }
    return 0;
}

class BufferAttribute {
public:
    AttributeType type = AttributeType::Float32;
    int itemSize = 3;
    int count = 0;
    bool normalized = false;
    std::vector<std::byte> data;

    BufferAttribute() = default;

    template <class T>
    static BufferAttribute fromSpan(std::span<const T> values, int itemSize, AttributeType type, bool normalized = false) {
        BufferAttribute a;
        a.type = type;
        a.itemSize = itemSize;
        a.normalized = normalized;
        a.count = itemSize > 0 ? static_cast<int>(values.size() / itemSize) : 0;
        a.data.resize(values.size() * sizeof(T));
        if (!values.empty()) std::memcpy(a.data.data(), values.data(), a.data.size());
        return a;
    }

    template <class T>
    static BufferAttribute fromVector(std::span<const T> values, int itemSize, AttributeType type, bool normalized = false) {
        return fromSpan<T>(values, itemSize, type, normalized);
    }

    template <class T>
    static BufferAttribute fromVector(const std::vector<T>& values, int itemSize, AttributeType type, bool normalized = false) {
        return fromSpan<T>(std::span<const T>(values.data(), values.size()), itemSize, type, normalized);
    }

    template <class T>
    std::span<const T> asSpan() const {
        return {reinterpret_cast<const T*>(data.data()), data.size() / sizeof(T)};
    }

    std::size_t byteSize() const { return data.size(); }
    std::size_t strideBytes() const { return attribute_type_size(type) * static_cast<std::size_t>(itemSize); }
};

} // namespace THREE
