# v6.0.16 VAO cache / shadow-pass forward draw fix

Fixes a macOS OpenGL Core issue where examples rendered only the blue/background clear color even though RenderList and draw call counters were non-zero.

Root cause: `GLShadowMap` uses raw `glBindVertexArray()` during depth rendering. The forward renderer keeps its own `GLBindingStates` VAO cache. After the shadow pass, OpenGL could have VAO 0 bound while `GLBindingStates` still believed a forward VAO was current, causing `bindVertexArray()` to early-return and subsequent forward `glDrawElements()` to draw nothing.

Fix: reset `GLBindingStates` immediately after `shadowMap.render(...)` so the forward pass always rebinds the real VAO.
