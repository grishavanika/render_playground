### Run

```
# Convert raw .obj into internal format.
lr_export.exe assets\_package assets\skull\skull.obj
# Load assets\_package\skull.lr.bin model.
```

### Sample

![](sample.png)

### Build

WARNING: currently path to Vulkan SDK is hardcoded to be in
C:/Programs/VulkanSDK/1.2.170.0 folder (third_party/vulkan_integration.cmake file).
Sorry.

#### Build and configure as fast as possible

```
cmake -G "Visual Studio 16 2019" ^
    -DCMAKE_TOOLCHAIN_FILE=C:\libs\vcpkg\scripts\buildsystems\vcpkg.cmake ^
    -A x64 ..
```

This will pull dependencies from vcpkg installed in C:/libs/vcpkg.  
Otherwise, 

```
cmake -G "Visual Studio 16 2019" -A x64 ..
```

This will pull dependencies from github (if not yet downloaded with get submodules)
and build them as part of building process.

#### Clang on Windows and MinGW.

Clang:
```
cmake -G "Visual Studio 16 2019" -A x64 -T LLVM ..
```

MinGW:
```
cmake -G "MinGW Makefiles" ..
```
