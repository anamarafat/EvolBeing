# Stage 2 connectivity correction

Apply over the **Stage 2 integrated patch** (not the original main branch).

- Delete a vertex from a closed polyline: one open polyline, original ID retained.
- Delete an interior vertex from an open polyline: up to two valid edge-bearing components, original ID and one new ID.
- Delete second/penultimate vertex: remove isolated endpoint and keep one polyline.
- Delete edge: do not retain singleton/empty polylines.
- Close Polyline / New Edge unchecked: reconnect final edge to first vertex, drop last vertex if unreferenced. Requires at least four vertices before closure.
- Close Polyline works on active or selected polyline.

Run CMake build and CTest, then manually check geometry redraw and tree. The GUI was not compiled here.

Additional regression test file: `tests/Geometry2DCorrectionTests.cpp`.
If the existing CMakeLists.txt only registers `Geometry2DTests`, add:

```cmake
add_executable(Geometry2DCorrectionTests tests/Geometry2DCorrectionTests.cpp)
target_link_libraries(Geometry2DCorrectionTests PRIVATE GeomCore)
add_test(NAME Geometry2DCorrectionTests COMMAND Geometry2DCorrectionTests)
```
