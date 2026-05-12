# Migration Notes

## SDL3 migration

- Migrated the application from SDL2 to SDL3 following the official SDL migration guide:
  https://wiki.libsdl.org/SDL3/README-migration
- Switched the build to `find_package(SDL3 ...)`, updated the Dear ImGui backend to `imgui_impl_sdl3`, and replaced SDL2-specific APIs with their SDL3 equivalents.
- Updated the release workflows and developer setup notes to download, build, or bundle SDL3 instead of SDL2.
