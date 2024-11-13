# ASE 3D model reference images

## MotionCaptureROM.ase

### Model is pure node animation of separate rigid meshes, no bones/mesh deformation
Note: this model has no bones/skeleton; the body is formed of separate rigid meshes
for each segment (e.g. upper arm, lower arm, upper leg, lower leg are all separate meshses) 
and animation  is entirely driven by changing node transformations over the animation timeline, 
rather than deforming vertices via bones and associated weights.

### Artifacts
Animation has known artifacts, as stated in [MotionCaptureROM.source.txt](../MotionCaptureROM.source.txt):
```
NOTE: The errors in the middle of the animation are caused by problems during recording,
it's not an importer issue.
```

<img alt="MotionCaptureROM.ase" src="screenshots/MotionCaptureROM_ase.gif" width=320 />
