#include <spill/context.h>
#include <spill/model/image.h>
#include <spill/model/assembly.h>

using namespace spill;

Spill::Spill(SpillRuntimeOptions& opts_) 
	: opts(opts_)
{
}

Spill::~Spill()
{
}

model::Assembly *Spill::LoadAssembly(std::string assembly_file)
{
	model::Image *img = new model::Image();
	if (!img->LoadFile(assembly_file)) {
		LOG(ERROR) << "unable to load assembly image.";
		delete img;
		return NULL;
	}
	
	model::Assembly *assembly = new model::Assembly(img);
	
	return assembly;
}