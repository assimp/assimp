/** @file Definitions for import post processing steps */
#ifndef AI_POSTPROCESS_H_INC
#define AI_POSTPROCESS_H_INC

#ifdef __cplusplus
extern "C" {
#endif

/** Defines the flags for all possible post processing steps. */
enum aiPostProcessSteps
{
	/** Calculates the binormals and tangents for the imported meshes. Does nothing
	 * if a mesh does not have normals. You might want this post processing step to be
	 * executed if you plan to use tangent space calculations such as normal mapping 
	 * applied to the meshes.
	 */
	aiProcess_CalcTangentSpace = 1,

	/** Identifies and joins identical vertex data sets within all imported meshes. 
	 * After this step is run each mesh does contain only unique vertices anymore,
	 * so a vertex is possibly used by multiple faces. You propably always want
	 * to use this post processing step.*/
	aiProcess_JoinIdenticalVertices = 2,

	/** Converts all the imported data to a left-handed coordinate space such as 
	 * the DirectX coordinate system. By default the data is returned in a right-handed
	 * coordinate space which for example OpenGL preferres. In this space, +X points to the
	 * right, +Y points upwards and +Z points to the viewer. In the DirectX coordinate space
	 * +X points to the right, +Y points upwards and +Z points away from the viewer 
	 * into the screen.
	 */
	aiProcess_ConvertToLeftHanded = 4,

	/** Triangulates all faces of all meshes. By default the imported mesh data might 
	 * contain faces with more than 3 indices. For rendering a mesh you usually need
	 * all faces to be triangles. This post processing step splits up all higher faces
	 * to triangles.
	 */
	aiProcess_Triangulate = 8,


	/** Omits all normals found in the file. This can be used together
	 * with either the aiProcess_GenNormals or the aiProcess_GenSmoothNormals
	 * flag to force the recomputation of the normals.
	 */
	aiProcess_KillNormals = 0x10,


	/** Generates normals for all faces of all meshes. The normals are shared
	* between the three vertices of a face. This is ignored
	* if normals are already existing. This flag may not be specified together
	* with aiProcess_GenSmoothNormals
	*/
	aiProcess_GenNormals = 0x20,


	/** Generates smooth normals for all vertices in the mesh. This is ignored
	* if normals are already existing. This flag may not be specified together
	* with aiProcess_GenNormals
	*/
	aiProcess_GenSmoothNormals = 0x40,


	/** Splits large meshes into submeshes
	* This is quite useful for realtime rendering where the number of vertices
	* is usually limited by the video driver.
	*
	* A mesh is split if it consists of more than 1 * 10^6 vertices. This is defined
	* in the internal SplitLargeMeshes.h header as AI_SLM_MAX_VERTICES.
	*/
	aiProcess_SplitLargeMeshes = 0x80
};

#ifdef __cplusplus
} // end of extern "C"
#endif

#endif // AI_POSTPROCESS_H_INC