#pragma once

#include "../Core/AssetImporter.h"

namespace Ubpa::Utopia {
	class MeshImporter final : public TAssetImporter<MeshImporter> {
	public:
		using TAssetImporter<MeshImporter>::TAssetImporter;
		
		virtual AssetImportContext ImportAsset() const override;
		static void RegisterToUDRefl();
	};

	class MeshImporterCreator final : public TAssetImporterCreator<MeshImporter> {
	public:
		virtual std::vector<std::string> SupportedExtentions() const override;
	};
}
