/** @file Defines a post processing step to compute vertex normals for all loaded vertizes */
#ifndef AI_GENVERTEXNORMALPROCESS_H_INC
#define AI_GENVERTEXNORMALPROCESS_H_INC

#include "BaseProcess.h"
#include "../include/aiMesh.h"
namespace Assimp
	{

	// ---------------------------------------------------------------------------
	/** The GenFaceNormalsProcess computes vertex normals for all vertizes of all meshes
	*/
	class GenVertexNormalsProcess : public BaseProcess
		{
		friend class Importer;

		protected:
			/** Constructor to be privately used by Importer */
			GenVertexNormalsProcess();

			/** Destructor, private as well */
			~GenVertexNormalsProcess();

		public:
			// -------------------------------------------------------------------
			/** Returns whether the processing step is present in the given flag field.
			* @param pFlags The processing flags the importer was called with. A bitwise
			*   combination of #aiPostProcessSteps.
			* @return true if the process is present in this flag fields, false if not.
			*/
			bool IsActive( unsigned int pFlags) const;

			// -------------------------------------------------------------------
			/** Executes the post processing step on the given imported data.
			* At the moment a process is not supposed to fail.
			* @param pScene The imported data to work at.
			*/
			void Execute( aiScene* pScene);


		private:
			void GenMeshVertexNormals (aiMesh* pcMesh);
		};

	} // end of namespace Assimp

#endif // !!AI_GENVERTEXNORMALPROCESS_H_INC