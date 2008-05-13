//-------------------------------------------------------------------------------
/**
*	This program is distributed under the terms of the GNU Lesser General
*	Public License (LGPL). 
*
*	ASSIMP Viewer Utility
*
*/
//-------------------------------------------------------------------------------

#if (!defined AV_SHADERS_H_INCLUDED)
#define AV_SHADERS_H_INCLUDED

// Shader used for rendering a skybox background
extern std::string  g_szSkyboxShader;

// Shader used for visualizing normal vectors
extern std::string  g_szNormalsShader;

// Default shader
extern std::string  g_szDefaultShader;

// Material shader
extern std::string  g_szMaterialShader;

// Shader used to draw the yellow circle on top of everything
extern std::string  g_szPassThroughShader;

// Shader used to draw the checker pattern background for the texture view
extern std::string  g_szCheckerBackgroundShader;

#endif // !! AV_SHADERS_H_INCLUDED
