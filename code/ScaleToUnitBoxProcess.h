/*
 * ScaleToUnitBoxProcess.h
 *
 *  Created on: Jan 20, 2015
 *      Author: Alov Maxim <alovmax@yandex.ru>
 */

/// @file ScaleToUnitBoxProcess.h
/// Defines a post processing step to scale mesh vertices into unit box
#ifndef AI_SCALETOUNITBOXPROCESS_H_INC
#define AI_SCALETOUNITBOXPROCESS_H_INC

#include <vector>
#include "BaseProcess.h"

#include "../include/assimp/mesh.h"
#include "../include/assimp/scene.h"

namespace Assimp
{

class ScaleToUnitBoxProcess : public BaseProcess
{
public:

  ScaleToUnitBoxProcess();
  ~ScaleToUnitBoxProcess();

public:
  /** Returns whether the processing step is present in the given flag.
  * @param pFlags The processing flags the importer was called with. A
  *   bitwise combination of #aiPostProcessSteps.
  * @return true if the process is present in this flag fields,
  *   false if not.
  */
  bool IsActive( unsigned int pFlags) const;

  /** Called prior to ExecuteOnScene().
  * The function is a request to the process to update its configuration
  * basing on the Importer's configuration property list.
  */
  virtual void SetupProperties(const Importer* pImp);

protected:
  /** Executes the post processing step on the given imported data.
  * At the moment a process is not supposed to fail.
  * @param pScene The imported data to work at.
  */
  void Execute( aiScene* pScene);

  /// Scales all vertices of the given mesh to conform unit box.
  /// @param pMesh the Mesh which vertices to be scaled.
  void ScaleMesh( const aiMesh* pMesh) const;
};

} // end of namespace Assimp


#endif // !!AI_SCALETOUNITBOXPROCESS_H_INC
