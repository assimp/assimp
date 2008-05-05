//-------------------------------------------------------------------------------
/**
*	This program is distributed under the terms of the GNU Lesser General
*	Public License (LGPL). 
*
*	ASSIMP Viewer Utility
*
*/
//-------------------------------------------------------------------------------

#if (!defined AV_CAMERA_H_INCLUDED)
#define AV_CAMERA_H_INCLUDED

//-------------------------------------------------------------------------------
/**	\brief Camera class
*/
//-------------------------------------------------------------------------------
class Camera
	{
	public:


		Camera ()
			:

			vPos(0.0f,0.0f,-10.0f),
			vLookAt(0.0f,0.0f,1.0f),
			vUp(0.0f,1.0f,0.0f),
			vRight(0.0f,1.0f,0.0f)
			{

			}
	public:

		// position of the camera
		aiVector3D vPos;

		// up-vector of the camera
		aiVector3D vUp;

		// camera's looking point is vPos + vLookAt
		aiVector3D vLookAt;

		// right vector of the camera
		aiVector3D vRight;


		// Equation
		// (vRight ^ vUp) - vLookAt == 0  
		// needn't apply

	} ;

#endif // !!IG