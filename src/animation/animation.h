#pragma once
#include "common.h"
#include "core/object-3d.h"
#include "core/renderable.h"

namespace THREE {

enum class Interpolation { Step, Linear, Smooth };
enum class TrackValueType { Vec3, Quat, Float };

// three.js-compatible loop modes for the core subset.
enum class LoopMode { Once, Repeat, PingPong };

struct KeyframeTrack {
    // Path format mirrors a useful subset of three.js bindings:
    // "ObjectName.position", "ObjectName.quaternion", "ObjectName.scale",
    // "ObjectName.morphTargetInfluences[0]",
    // "ObjectName.morphTargetInfluences[Smile]".
    // Empty object name targets the mixer's root object.
    std::string targetPath;
    TrackValueType valueType = TrackValueType::Vec3;
    Interpolation interpolation = Interpolation::Linear;
    std::vector<float> times;
    std::vector<float> values;
};

struct AnimationClip {
    std::string name;
    float duration = 0.0f;
    std::vector<KeyframeTrack> tracks;
};

class AnimationAction {
public:
    AnimationClip* clip = nullptr;
    float time = 0.0f;
    float weight = 1.0f;
    float timeScale = 1.0f;
    LoopMode loop = LoopMode::Repeat;
    int repetitions = -1; // -1 = infinite for Repeat/PingPong.
    bool enabled = true;
    bool paused = false;
    bool clampWhenFinished = false;
    bool zeroSlopeAtStart = true;
    bool zeroSlopeAtEnd = true;

    // Runtime state similar to three.js AnimationAction internals.
    bool scheduled = false;
    bool running = false;
    int loopCount = 0;
    int direction = 1;
    float localWeight = 1.0f;
    float localTimeScale = 1.0f;

    bool fading = false;
    float fadeStartWeight = 1.0f;
    float fadeEndWeight = 1.0f;
    float fadeElapsed = 0.0f;
    float fadeDuration = 0.0f;

    bool warping = false;
    float warpStartScale = 1.0f;
    float warpEndScale = 1.0f;
    float warpElapsed = 0.0f;
    float warpDuration = 0.0f;

    AnimationAction& play() { enabled = true; paused = false; scheduled = true; running = true; return *this; }
    AnimationAction& stop() { enabled = false; scheduled = false; running = false; paused = false; time = 0.0f; loopCount = 0; direction = 1; return *this; }
    AnimationAction& reset() { time = 0.0f; enabled = true; paused = false; scheduled = true; running = true; loopCount = 0; direction = 1; return *this; }

    AnimationAction& setLoop(LoopMode mode, int reps = -1) { loop = mode; repetitions = reps; return *this; }
    AnimationAction& setDuration(float duration) {
        if (clip && duration > 1e-6f && clip->duration > 0.0f) timeScale = clip->duration / duration;
        return *this;
    }
    AnimationAction& setEffectiveWeight(float w) { weight = w; localWeight = w; return *this; }
    AnimationAction& setEffectiveTimeScale(float s) { timeScale = s; localTimeScale = s; return *this; }

    AnimationAction& fadeIn(float duration) { fading = true; fadeStartWeight = 0.0f; fadeEndWeight = weight; fadeElapsed = 0.0f; fadeDuration = std::max(duration, 1e-6f); enabled = true; scheduled = true; running = true; return *this; }
    AnimationAction& fadeOut(float duration) { fading = true; fadeStartWeight = localWeight; fadeEndWeight = 0.0f; fadeElapsed = 0.0f; fadeDuration = std::max(duration, 1e-6f); return *this; }
    AnimationAction& crossFadeFrom(AnimationAction& other, float duration, bool warp = false) { other.fadeOut(duration); fadeIn(duration); if (warp) syncWith(other); return *this; }
    AnimationAction& crossFadeTo(AnimationAction& other, float duration, bool warp = false) { other.crossFadeFrom(*this, duration, warp); return *this; }
    AnimationAction& syncWith(const AnimationAction& other) { time = other.time; timeScale = other.timeScale; localTimeScale = other.localTimeScale; return *this; }
    AnimationAction& halt(float duration) { warpTo(localTimeScale, 0.0f, duration); return *this; }
    AnimationAction& warp(float startScale, float endScale, float duration) { return warpTo(startScale, endScale, duration); }
    AnimationAction& warpTo(float startScale, float endScale, float duration) { warping = true; warpStartScale = startScale; warpEndScale = endScale; warpElapsed = 0.0f; warpDuration = std::max(duration, 1e-6f); return *this; }

    bool isRunning() const { return enabled && scheduled && running && !paused; }
    bool isScheduled() const { return scheduled; }
};

class AnimationMixer {
public:
    explicit AnimationMixer(Object3D* root = nullptr) : root(root) {}

    void setRoot(Object3D* r) { root = r; bindingCache.clear(); }

    AnimationAction& clipAction(AnimationClip& clip) {
        for (auto& action : actions) {
            if (action->clip == &clip) return *action;
        }
        auto action = std::make_unique<AnimationAction>();
        action->clip = &clip;
        action->localWeight = action->weight;
        action->localTimeScale = action->timeScale;
        action->play();
        actions.emplace_back(std::move(action));
        return *actions.back();
    }

    void stopAllAction() {
        for (auto& action : actions) action->stop();
    }

    void update(float deltaTime) {
        if (!root) return;
        for (auto& actionPtr : actions) {
            auto& action = *actionPtr;
            updateActionState(action, deltaTime);
            if (!action.enabled || !action.scheduled || action.paused || !action.clip) continue;
            if (action.localWeight <= 0.0001f && !action.fading) continue;
            advanceActionTime(action, deltaTime);
            if (!action.running && !action.clampWhenFinished) continue;
            applyClip(*action.clip, action.time, glm::clamp(action.localWeight, 0.0f, 1.0f));
        }
    }

    std::size_t actionCount() const { return actions.size(); }

private:
    Object3D* root = nullptr;
    std::vector<std::unique_ptr<AnimationAction>> actions;
    std::unordered_map<std::string, Object3D*> bindingCache;

    void updateActionState(AnimationAction& action, float deltaTime) {
        if (!action.enabled) return;
        if (action.fading) {
            action.fadeElapsed += std::max(0.0f, deltaTime);
            float u = glm::clamp(action.fadeElapsed / action.fadeDuration, 0.0f, 1.0f);
            action.localWeight = glm::mix(action.fadeStartWeight, action.fadeEndWeight, u);
            if (u >= 1.0f) {
                action.fading = false;
                action.weight = action.localWeight;
                if (action.localWeight <= 0.0001f) action.enabled = false;
            }
        } else {
            action.localWeight = action.weight;
        }
        if (action.warping) {
            action.warpElapsed += std::max(0.0f, deltaTime);
            float u = glm::clamp(action.warpElapsed / action.warpDuration, 0.0f, 1.0f);
            action.localTimeScale = glm::mix(action.warpStartScale, action.warpEndScale, u);
            if (u >= 1.0f) {
                action.warping = false;
                action.timeScale = action.localTimeScale;
            }
        } else {
            action.localTimeScale = action.timeScale;
        }
    }

    void advanceActionTime(AnimationAction& action, float deltaTime) {
        AnimationClip* clip = action.clip;
        if (!clip || clip->duration <= 0.0f) return;
        float duration = clip->duration;
        action.time += deltaTime * action.localTimeScale * static_cast<float>(action.direction);

        if (action.loop == LoopMode::Once) {
            if (action.time >= duration) {
                action.time = duration;
                action.running = false;
                action.scheduled = action.clampWhenFinished;
                action.enabled = action.clampWhenFinished;
            } else if (action.time <= 0.0f) {
                action.time = 0.0f;
                action.running = false;
                action.scheduled = action.clampWhenFinished;
                action.enabled = action.clampWhenFinished;
            }
            return;
        }

        if (action.loop == LoopMode::Repeat) {
            while (action.time >= duration) {
                action.time -= duration;
                ++action.loopCount;
                if (action.repetitions >= 0 && action.loopCount >= action.repetitions) {
                    action.time = duration;
                    action.running = false;
                    action.scheduled = action.clampWhenFinished;
                    action.enabled = action.clampWhenFinished;
                    return;
                }
            }
            while (action.time < 0.0f) action.time += duration;
            return;
        }

        // PingPong.
        if (action.time >= duration) {
            action.time = duration - (action.time - duration);
            action.direction *= -1;
            ++action.loopCount;
        } else if (action.time < 0.0f) {
            action.time = -action.time;
            action.direction *= -1;
            ++action.loopCount;
        }
        if (action.repetitions >= 0 && action.loopCount >= action.repetitions) {
            action.time = glm::clamp(action.time, 0.0f, duration);
            action.running = false;
            action.scheduled = action.clampWhenFinished;
            action.enabled = action.clampWhenFinished;
        }
    }

    Object3D* findByName(const std::string& name) {
        if (!root) return nullptr;
        if (name.empty()) return root;
        if (auto it = bindingCache.find(name); it != bindingCache.end()) return it->second;
        Object3D* found = nullptr;
        root->traverse([&](Object3D& o) { if (!found && o.name == name) found = &o; });
        bindingCache[name] = found;
        return found;
    }

    static std::pair<std::string, std::string> splitPath(const std::string& path) {
        auto dot = path.rfind('.');
        if (dot == std::string::npos) return {"", path};
        return {path.substr(0, dot), path.substr(dot + 1)};
    }

    static std::size_t findInterval(const std::vector<float>& times, float t) {
        if (times.size() < 2) return 0;
        auto upper = std::upper_bound(times.begin(), times.end(), t);
        if (upper == times.begin()) return 0;
        std::size_t i = static_cast<std::size_t>(std::distance(times.begin(), upper) - 1);
        return std::min(i, times.size() - 2);
    }

    glm::vec3 sampleVec3(const KeyframeTrack& track, float t) const {
        if (track.times.empty() || track.values.size() < 3) return glm::vec3(0.0f);
        std::size_t i = findInterval(track.times, t);
        std::size_t a = i * 3;
        glm::vec3 v0(track.values[a + 0], track.values[a + 1], track.values[a + 2]);
        if (track.interpolation == Interpolation::Step || i + 1 >= track.times.size()) return v0;
        std::size_t b = (i + 1) * 3;
        glm::vec3 v1(track.values[b + 0], track.values[b + 1], track.values[b + 2]);
        float span = std::max(1e-6f, track.times[i + 1] - track.times[i]);
        float u = glm::clamp((t - track.times[i]) / span, 0.0f, 1.0f);
        if (track.interpolation == Interpolation::Smooth) u = u * u * (3.0f - 2.0f * u);
        return glm::mix(v0, v1, u);
    }

    float sampleFloat(const KeyframeTrack& track, float t) const {
        if (track.times.empty() || track.values.empty()) return 0.0f;
        std::size_t i = findInterval(track.times, t);
        float v0 = track.values[i];
        if (track.interpolation == Interpolation::Step || i + 1 >= track.times.size()) return v0;
        float v1 = track.values[i + 1];
        float span = std::max(1e-6f, track.times[i + 1] - track.times[i]);
        float u = glm::clamp((t - track.times[i]) / span, 0.0f, 1.0f);
        if (track.interpolation == Interpolation::Smooth) u = u * u * (3.0f - 2.0f * u);
        return glm::mix(v0, v1, u);
    }

    glm::quat sampleQuat(const KeyframeTrack& track, float t) const {
        if (track.times.empty() || track.values.size() < 4) return glm::quat(1,0,0,0);
        std::size_t i = findInterval(track.times, t);
        std::size_t a = i * 4;
        glm::quat q0(track.values[a + 3], track.values[a + 0], track.values[a + 1], track.values[a + 2]);
        if (track.interpolation == Interpolation::Step || i + 1 >= track.times.size()) return glm::normalize(q0);
        std::size_t b = (i + 1) * 4;
        glm::quat q1(track.values[b + 3], track.values[b + 0], track.values[b + 1], track.values[b + 2]);
        float span = std::max(1e-6f, track.times[i + 1] - track.times[i]);
        float u = glm::clamp((t - track.times[i]) / span, 0.0f, 1.0f);
        if (track.interpolation == Interpolation::Smooth) u = u * u * (3.0f - 2.0f * u);
        return glm::normalize(glm::slerp(q0, q1, u));
    }

    void applyClip(AnimationClip& clip, float time, float weight) {
        for (const auto& track : clip.tracks) {
            auto [objectName, property] = splitPath(track.targetPath);
            Object3D* target = findByName(objectName);
            if (!target) continue;
            if (property == "position" && track.valueType == TrackValueType::Vec3) {
                target->position = glm::mix(target->position, sampleVec3(track, time), weight);
                target->matrixWorldNeedsUpdate = true;
            } else if (property == "scale" && track.valueType == TrackValueType::Vec3) {
                target->scale = glm::mix(target->scale, sampleVec3(track, time), weight);
                target->matrixWorldNeedsUpdate = true;
            } else if (property == "quaternion" && track.valueType == TrackValueType::Quat) {
                target->quaternion = glm::normalize(glm::slerp(target->quaternion, sampleQuat(track, time), weight));
                target->matrixWorldNeedsUpdate = true;
            } else if (property.rfind("morphTargetInfluences", 0) == 0 && track.valueType == TrackValueType::Float) {
                auto* mesh = dynamic_cast<Mesh*>(target);
                if (!mesh) continue;
                mesh->syncMorphTargets();
                auto lb = property.find('[');
                auto rb = property.find(']', lb == std::string::npos ? 0 : lb + 1);
                if (lb == std::string::npos || rb == std::string::npos || rb <= lb + 1) continue;
                std::string key = property.substr(lb + 1, rb - lb - 1);
                int index = -1;
                if (!key.empty() && std::all_of(key.begin(), key.end(), [](unsigned char c){ return std::isdigit(c); })) {
                    index = std::stoi(key);
                } else if (auto it = mesh->morphTargetDictionary.find(key); it != mesh->morphTargetDictionary.end()) {
                    index = it->second;
                }
                if (index >= 0) {
                    float current = (static_cast<std::size_t>(index) < mesh->morphTargetInfluences.size()) ? mesh->morphTargetInfluences[static_cast<std::size_t>(index)] : 0.0f;
                    mesh->setMorphTargetInfluence(index, glm::mix(current, sampleFloat(track, time), weight));
                }
            }
        }
    }
};

} // namespace THREE
