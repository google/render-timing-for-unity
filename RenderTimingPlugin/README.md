Building the native plugin
==========================
The native plugin source exists separately from the Unity project.  Upon
modifying the source the native libraries must be rebuilt and copied into
the Unity project as follows.

Android
-------
```
cd Android
ndk-build
cp libs/armeabi-v7a/libRenderTimingPlugin.so ../../RenderTiming/Assets/RenderTiming/Plugins/Android/
```
