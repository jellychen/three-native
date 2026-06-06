# v4.6 AnimationMixer parity

This version expands the animation system toward the three.js `AnimationMixer` / `AnimationAction` model.

## Added

- `AnimationAction::play()`
- `AnimationAction::stop()`
- `AnimationAction::reset()`
- `AnimationAction::setLoop()`
- `AnimationAction::setEffectiveWeight()`
- `AnimationAction::setEffectiveTimeScale()`
- `AnimationAction::fadeIn()`
- `AnimationAction::fadeOut()`
- `AnimationAction::crossFadeFrom()`
- `AnimationAction::crossFadeTo()`
- `AnimationAction::syncWith()`
- `AnimationAction::warp()` / `halt()`
- `LoopMode::Once`
- `LoopMode::Repeat`
- `LoopMode::PingPong`
- `clampWhenFinished`
- action reuse in `AnimationMixer::clipAction()`
- stable action references via internal `unique_ptr` storage

## Supported property bindings

- `ObjectName.position`
- `ObjectName.quaternion`
- `ObjectName.scale`
- `ObjectName.morphTargetInfluences[0]`
- `ObjectName.morphTargetInfluences[Smile]`

## New example

```bash
xmake f -m debug --use_angle=false --enable_assimp=false
xmake -r
xmake run 39_animation_mixer_parity_lab
```

The example demonstrates repeat, ping-pong, once, fade, cross-fade and quaternion interpolation.

## Remaining gaps

- full additive animation
- full PropertyBinding parser equivalent to three.js
- root motion conventions
- event dispatching (`loop`, `finished`)
- clip optimization utilities
- bone texture fallback for very large skeletons
