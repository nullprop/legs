# LEGS

Very WIP framework for 3D games.

**legs** help you run. (\*ba-dum-tss\*)

## Running

### Linux

Dependencies: Meson, gcc

```sh
git clone https://github.com/nullprop/legs.git --recursive
cd legs
./scripts/compile_jolt.sh distribution
./scripts/setup_release.sh
ninja -C build
./build/examples/02_systems/02_systems
```

## Third-party code

- [glm](https://github.com/g-truc/glm): MIT / The Happy Bunny License
- [imgui](https://github.com/ocornut/imgui): MIT License
- [JoltPhysics](https://github.com/jrouwe/JoltPhysics): MIT License
- [VulkanMemoryAllocator](https://github.com/GPUOpen-LibrariesAndSDKs/VulkanMemoryAllocator): MIT License
- [SDL](https://github.com/libsdl-org/SDL) (system/wrapdb): Zlib License
- [Vulkan-Headers](https://github.com/KhronosGroup/Vulkan-Headers) (system/wrapdb): Apache-2.0 / MIT License

