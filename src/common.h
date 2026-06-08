#pragma once

#include <algorithm>
#include <array>
#include <atomic>
#include <cassert>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <cctype>
#include <cstdlib>
#include <cstdio>
#include <cstddef>
#include <filesystem>
#include <functional>
#include <limits>
#include <initializer_list>
#include <iterator>
#include <memory>
#include <optional>
#include <span>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <unordered_map>
#include <utility>
#include <variant>
#include <vector>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/constants.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtc/type_ptr.hpp>


#ifndef THREECPP_ENABLE_BASISU
#define THREECPP_ENABLE_BASISU 0
#endif
#ifndef THREECPP_ENABLE_ASSIMP
#define THREECPP_ENABLE_ASSIMP 0
#endif
#ifndef THREECPP_USE_ANGLE
#define THREECPP_USE_ANGLE 0
#endif

namespace THREE {

using ObjectId = std::uint64_t;

inline ObjectId next_object_id() {
    static std::atomic<ObjectId> id{1};
    return id.fetch_add(1, std::memory_order_relaxed);
}

template <class T, class... Args>
std::shared_ptr<T> make_ref(Args&&... args) {
    return std::make_shared<T>(std::forward<Args>(args)...);
}

} // namespace THREE
