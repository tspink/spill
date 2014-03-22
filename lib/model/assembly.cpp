#include <spill/model/assembly.h>

using namespace spill::model;

Assembly::Assembly(Image *image_) : image(image_)
{
	assert(image_ != NULL);
}

Assembly::~Assembly()
{
	
}

Method *Assembly::FindEntryPoint()
{
	return NULL;
}
