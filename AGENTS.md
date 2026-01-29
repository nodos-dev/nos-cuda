# AGENTS.md (nos-cuda)

This repo contains CUDA subsystem and plugins under `Module/dev/ai/nos-cuda`.

Key structure
- Subsystem: `nosCUDASubsystem` (public headers in `Include/nosSysCuda`)
- Plugins: `Plugins/nosCUDA`, `Plugins/nosCUDAInterop`
- Dependencies: `nosCUDADeps`

CMake/toolchain conventions
- Plugin SDK v41. Public headers go in `Include/nosSysCuda` only.
- `.nosplugin` files define publish version and dependencies.

Coding conventions
- Use `nos::ObjectRef` / `nos::TypedObjectRef` for object IDs/handles.
- `nosCudaStreamObject` is a `nosObjectId` (object reference semantics apply).

Build/test
- Generate: `./nodos dev gen -p <path to generated project> --plugin-dirs "<path to repo>"`
- Build: `./nodos dev build -p <path to generated project>`
- nvcc PTX steps require `cl.exe` in PATH.

Versioning
- Bump major for breaking API changes in `nosCUDA*.nosplugin` and `nosSysCuda*.nosplugin`.
