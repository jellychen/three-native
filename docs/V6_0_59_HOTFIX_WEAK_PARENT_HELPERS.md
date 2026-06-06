# V6.0.59 Hotfix: weak parent helper access

## Fix

`Object3D::parent` is now `std::weak_ptr<Object3D>`, so helper code must not use it like a raw pointer or `shared_ptr`.

`SkeletonHelper::updateFromRoot()` now resolves the parent through:

```cpp
Object3D* parent = o.parentObject();
```

Then it checks `parent->kind` and reads `parent->matrixWorld` from that resolved non-owning pointer.

## Reason

This keeps the public parent relationship non-owning and avoids parent-child reference cycles, while still supporting existing stack-allocated scene roots through the internal fallback used by `Object3D::parentObject()`.

## Affected file

- `src/helpers/Helpers.hpp`
