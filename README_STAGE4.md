# Stage 4 patch — EvolBeing_Gen_Alpha

Apply over the uploaded Stage 3 repository. This ZIP contains only changed source files.

## Layout
- Menu bar: File > Load Geometry / Save Geometry; Edit > Scale Geometry / Undo / Redo.
- Mesh toolbar: Generate Mesh (disabled placeholder), Show Mesh toggle, Fit View, Cell, Face, Node.
- Geometry toolbar: Info, Draw Line, Polyline, Draw Edge, Draw Vertex, Select Vertex/Edge/Polyline, Connect Nodes, Snap, Close Polyline, Delete Entity.
- Each toolbar is a separate single-height QToolBar.

## Geometry persistence
Versioned JSON (.eg2d.json), including tombstones/IDs, vertices, edges, polylines, groups, group names, types, active states, colours, thicknesses and per-polyline overrides. A failed validation leaves the current geometry untouched. Load replaces geometry after confirmation; it does not change the Cartesian mesh. Save uses QSaveFile for atomic replacement.

## Geometry drawing and scaling
Connect Nodes: click two geometry or Cartesian nodes in succession. Mesh nodes supply coordinates only; geometry vertices are created/reused independently. Each connection creates a two-node polyline (does not automatically merge into existing polylines). Scale Geometry applies X/Y factors about the active geometry bounding-box centre; does not scale the mesh.

## Known limits / validation
- Qt GUI compilation was not possible in the patch-generation environment (Qt6 development libraries unavailable). Build and manually test on Windows.
- Undo/Redo is a bounded (80 snapshots) geometry history. It covers principal model edits; GUI interaction sequences should be validated, especially incomplete polylines and Connect Nodes.
- No Fit-to-Mesh scaling preset in this patch; use explicit X/Y factors.
- Geometry node-based drawing is implemented via Connect Nodes. Starting the existing freehand polyline tool directly from a mesh node is not yet integrated.
- Generate Mesh is a disabled placeholder, per request; existing bounding-box Apply behaviour is retained.

## Apply (Windows CMD, repository root)
```
tar -xf "%USERPROFILE%\Downloads\EvolBeing_Stage4_patch.zip" -C .
cmake -S . -B build-qt
cmake --build build-qt
ctest --test-dir build-qt --output-on-failure
```

Core Geometry2D tests and new Stage 4 core tests passed with GCC C++20. Test Save > Load with a group and colour/thickness overrides, then inspect the restored Workflow tree and graphics.
