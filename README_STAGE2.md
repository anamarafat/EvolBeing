# Stage 2 integrated editor patch (experimental)

Based on uploaded `EvolBeing-main (1).zip` (Stage 1 model).

## Included
- Info action for selected mesh cell/face/node and selected geometry vertex/edge/polyline.
- Separate selection modes for geometry vertices, edges and polylines; viewport hit testing and highlighting.
- Drawing toolbar actions for polyline, two-click edge and standalone vertex; existing Draw Line retained.
- Snap split-button: clicking Snap moves the selected geometry vertex, edge endpoints, or polyline vertices to their nearest **actual** Cartesian grid nodes. Snap All checkbox changes scope to all active geometry vertices. No pixel tolerance. Rejects moves that collapse an active edge. No automatic snapping while drawing.
- Close Polyline split-button with New Edge checkbox: when unchecked, finish as open; when checked, add closing edge.
- Delete Entity action calls Stage 1 model deletion methods and redraws geometry.
- Geometry redraw from model after edits; mesh regeneration now retains existing geometry.
- New `Geometry2D::moveVertex` method.

## Not yet included
- Persistent mesh-node references and geometry group property inheritance.
- Automatic geometry vertex/edge snapping *during drawing*.
- Geometry group property editor.
- Comprehensive undo/redo, automated GUI interaction tests, spatial acceleration.
- A truly closed polyline without a new edge: this mode deliberately finishes open.

## Validation
- Core Geometry2D C++20 test program compiled and passed on Linux with g++.
- Qt6 GUI could not be built in this environment (Qt6 dev package unavailable). **Build and GUI-test locally before committing.**

## Apply in Windows CMD
1. Save/commit/stash local changes first.
2. From repo root: `tar -xf "%USERPROFILE%\Downloads\EvolBeing_Stage2_integrated_patch.zip" -C .`
3. `git diff --check` and `git diff`.
4. `cmake --build build-qt`
5. `ctest --test-dir build-qt --output-on-failure`
6. Launch GUI, exercise mesh selections, geometry selection, snapping, deletion, closure, and mesh regeneration.

If compilation fails, share the full first compiler error and surrounding lines before changing additional files.
