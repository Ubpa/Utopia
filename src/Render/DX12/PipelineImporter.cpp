#include <Utopia/Render/DX12/PipelineImporter.h>

using namespace Ubpa::Utopia;

void PipelineImporter::RegisterToUDRefl() {
	RegisterToUDReflHelper();
}

std::vector<std::string> PipelineImporterCreator::SupportedExtentions() const {
	return { ".pipeline" };
}
