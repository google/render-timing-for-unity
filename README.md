RenderTiming plugin for Unity
=============================

This plugin employs the OpenGL timer query extension
(`GL_EXT_disjoint_timer_query`) to provide accurate GPU time measurements
in real-time to apps made with the Unity game engine.  Notably
this works on mobile devices and has virtually no overhead.

Typical use cases include:
   * adding GPU frame time to an app's real-time debug display
   * getting instant, in-app feedback on coding changes which affect rendering,
     as well as visibility into performance regressions and problematic views
   * automated performance testing

Caveats
-------
   * only Android is supported (adding other GL platforms is straightforward)
   * quality varies among mobile implementations of GL_EXT_disjoint_timer_query.
     **Notably, some GPU's (e.g. Mali) don't provide a useful measurement
     at all.**
   * only GLES3 graphics API is supported, not GLES2 or Vulkan
     (adding GLES2 support is straightforward)
   * plugin does not verify GL_EXT_disjoint_timer_query existence yet

Unity versions tested: 5.4 and 5.6

GPU's tested: Snapdragon 821 (Adreno 530), Snapdragon 835 (Adreno 540)

Quick start
-----------
Download the latest [RenderTiming Unity package](releases/latest/) and
import it into your Unity project.

Add the RenderTiming component to an active object in your scene.
Reference `RenderTiming.instance.deltaTime` from your development HUD, etc.

It will measure GPU time from the end of Update until the end of frame
rendering.  (I.e. encompassing both eyes for VR stereo case.)
See [`RenderTiming.cs`](RenderTiming/Assets/RenderTiming/RenderTiming.cs)
source for additional caveats.

By default it will log GPU time to console once per second.  Disable via
component's "Log Timing" field in the inspector.

Directory
---------
   * [RenderTiming/](RenderTiming/) - Unity package source
   * [RenderTimingPlugin/](RenderTimingPlugin/) - native plugin source

Disclaimer
----------
This project is not an official Google project.  It is not supported by
Google and Google specifically disclaims all warranties as to its quality,
merchantability, or fitness for a particular purpose.
