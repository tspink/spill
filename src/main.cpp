#include <iostream>
#include <spill/context.h>
#include <spill/model/assembly.h>
#include <spill/model/method.h>

using namespace spill;

int main(int argc, char **argv)
{
	SpillRuntimeOptions rto;
	rto.debug = true;
	
	Spill spill(rto);

	if (argc < 2) {
		std::cerr << "usage: " << argv[0] << " [options] <program> [program options]" << std::endl;
		return -1;
	}
	
	auto program = spill.LoadAssembly(std::string(argv[1]));
	if (program == NULL) {
		std::cerr << "error: unable to load main assembly" << std::endl;
		return -1;	
	}
	
	auto entry_point = program->FindEntryPoint();
	if (entry_point == NULL) {
		std::cerr << "error: unable to locate entry point" << std::endl;
		return -1;	
	}
	
	entry_point->Invoke();
	return 0;
}
