# Stage 3 Geometry Properties patch

Apply on top of the previously validated Stage 2 Correction patch. Archive paths are relative to repository root.

## Included
- Right-click group/polyline in Workflow tree > Properties...
- Editable name, group type, active metadata and group colour
- Colour chooser via QColorDialog, group colour inheritance and optional polyline colour override
- Geometry redraw uses effective per-polyline edge colour
- Right-click Geometry root > Validate Geometry prints existing validation results
- Core model tests for colour inheritance and overrides

## Apply (Windows CMD, from repo root)
```
tar -xf "%USERPROFILE%\Downloads\EvolBeing_Stage3_Properties_patch.zip" -C .
cmake --build build-qt
```

## Run new model test (optional)
Register a standalone target in CMake or compile tests/Geometry2DPropertiesTests.cpp with src/Geometry2D.cpp and include/.

## Important limitations
- Group active/type properties are metadata; they do not yet affect physics or hide geometry.
- Colour applies to polyline edges. Shared geometry vertices retain the existing vertex marker style.
- The existing geometry validation routine is reused; it is not a new exhaustive validator.
- No Save/Load serialization in this stage.
- Full Qt6 GUI build and interactive tests must be run on your Windows environment.
