### Run

```
# Build "export" project. This gives lr_export.exe.
# Bake .obj file into *.lr.bin (internal format) with:

lr_export.exe assets\skull\skull.obj

# This puts "skull.lr.bin" to folder where lr_export.exe was run.
# Drag&drop "skull.lr.bin" to App window.
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
