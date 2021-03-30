#pragma once

#include "../Core/AssetImporter.h"

namespace Ubpa::Utopia {
	class WorldAsset {
	public:
		// use Serializer
		WorldAsset(const UECS::World* world);
		WorldAsset(const UECS::World* world, std::span<UECS::Entity> entities);

		WorldAsset(std::string data) noexcept : data{ std::move(data) } {}
		const std::string& GetData() const noexcept { return data; }

		// use Serializer
		bool ToWorld(UECS::World* world);

	private:
		std::string data;
	};

	class WorldAssetImporter final : public TAssetImporter<WorldAssetImporter> {
	public:
		using TAssetImporter<WorldAssetImporter>::TAssetImporter;

		virtual std::string ReserializeAsset() const;
		virtual AssetImportContext ImportAsset() const override;
	private:
		friend class TAssetImporterCreator<WorldAssetImporter>;
		static void RegisterToUDRefl();
	};

	class WorldAssetImporterCreator final : public TAssetImporterCreator<WorldAssetImporter> {
	public:
		virtual std::vector<std::string> SupportedExtentions() const override;
	};
}
