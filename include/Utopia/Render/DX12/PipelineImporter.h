#pragma once

#include "../../Core/AssetImporter.h"

namespace Ubpa::Utopia {
	class PipelineImporter final : public TAssetImporter<PipelineImporter> {
	public:
		using TAssetImporter<PipelineImporter>::TAssetImporter;
		static void RegisterToUDRefl();
	};

	class PipelineImporterCreator final : public TAssetImporterCreator<PipelineImporter> {
	public:
		virtual std::vector<std::string> SupportedExtentions() const override;
	};
}
