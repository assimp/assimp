unit aiCamera;

interface

uses aiTypes, aiVector3D;

type TaiCamera = record
   mName: aiString;
   mPosition: TaiVector3D;
   mUp: TaiVector3D;
   mLookAt: TaiVector3D;
   mHorizontalFOV: single;
   mClipPlaneNear: single;
   mClipPlaneFar: single;
   mAspect: single;
   mOrthographicWidth: single;
end;
type PaiCamera = ^TaiCamera;
type PaiCameraArray = array[0..0] of PaiCamera;
type PPaiCameraArray = ^PaiCameraArray;

implementation

end.
