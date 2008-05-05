#ifndef OBJFILEMTLIMPORTER_H_INC
#define OBJFILEMTLIMPORTER_H_INC

namespace Assimp
{

/**
 *	@class	ObjFileMtlImporter
 *	@brief	Loads the material description from a mtl file.
 */
class ObjFileMtlImporter
{
public:
	//!	\brief	Default constructor
	ObjFileMtlImporter();
	
	//!	\brief	DEstructor
	~ObjFileMtlImporter();

private:
	//!	\brief	Copy constructor, empty.
	ObjFileMtlImporter(const ObjFileMtlImporter &rOther);
	
	//!	\brief	Assignment operator, returns only a reference of this instance.
	ObjFileMtlImporter &operator = (const ObjFileMtlImporter &rOther);

	void getColorRGBA();
	void getIlluminationModel();
	void getFloatValue();
	void createMaterial();
	void getTexture();
};

} // Namespace Assimp

#endif
