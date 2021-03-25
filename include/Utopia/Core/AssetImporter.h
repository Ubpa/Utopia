#pragma once

#include "Serializer.h"
#include <_deps/crossguid/guid.hpp>
#include <UDRefl/UDRefl.hpp>
#include <map>

namespace Ubpa::Utopia {
	class AssetImportContext {
	public:
		void AddObject(std::string id, UDRefl::SharedObject obj) {
			assets.insert_or_assign(std::move(id), obj);
		}
		void SetMainObjectID(std::string id) { mainObjID = std::move(id); }
		const std::map<std::string, UDRefl::SharedObject>& GetAssets() const noexcept { return assets; }

		const std::string& GetMainObjectID() const noexcept { return mainObjID; }
		UDRefl::SharedObject GetMainObject() const noexcept {
			auto target = assets.find(mainObjID);
			if (target == assets.end())
				return {};
			else
				return target->second;
		}
	private:
		std::string mainObjID;
		std::map<std::string, UDRefl::SharedObject> assets;
	};

	class AssetImporter {
	public:
		AssetImporter(xg::Guid guid) : guid{ guid } {}
		virtual ~AssetImporter() = default;

		const xg::Guid& GetGuid() const noexcept { return guid; }

		// {
		//   "__TypeID":<uint64>,
		//   "__TypeName":<string>,
		//   "__Content":{
		//     "__Guid":<string>
		//     ...
		//   }
		// }
		virtual void Serialize(Serializer::SerializeContext& ctx) const = 0;
		virtual bool ReserializeAsset() const { return false; }
		virtual AssetImportContext ImportAsset() const = 0;
	private:
		xg::Guid guid;
	};

	class AssetImporterCreator {
	public:
		virtual ~AssetImporterCreator() = default;
		// the guid is not registered into asset mngr yet, but we can store it in assetimporter
		virtual std::shared_ptr<AssetImporter> CreateAssetImporter(xg::Guid guid) = 0;
	};

	class DefaultAssetImporter : public AssetImporter {
	public:
		using AssetImporter::AssetImporter;
		virtual void Serialize(Serializer::SerializeContext& ctx) const override;
		virtual AssetImportContext ImportAsset() const override;
	};

	class DefaultAssetImporterCreator : public AssetImporterCreator {
	public:
		virtual std::shared_ptr<AssetImporter> CreateAssetImporter(xg::Guid guid) override;
	};
}
