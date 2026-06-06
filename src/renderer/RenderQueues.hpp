#pragma once
#include "renderer/RenderList.hpp"

namespace threecpp {

struct RenderQueueStats {
    int opaque = 0;
    int transmissive = 0;
    int transparent = 0;
};

inline bool material_is_transmissive(const Material& material) {
    return RenderList::classify(material) == RenderQueueBucket::Transmissive;
}

inline RenderQueueBucket classify_render_item(const RenderItem& item) {
    return item.bucket;
}

inline RenderQueueStats analyze_render_queues(const RenderList& list) {
    return {static_cast<int>(list.opaque.size()), static_cast<int>(list.transmissive.size()), static_cast<int>(list.transparent.size())};
}

} // namespace threecpp
