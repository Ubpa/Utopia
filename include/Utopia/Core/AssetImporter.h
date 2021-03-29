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

	/*
	* [Template]
	* class XXXImporter final : public TAssetImporter<XXXImporter> {
	* public:
	*   using TAssetImporter<XXXImporter>::TAssetImporter;
	*   virtual AssetImportContext ImportAsset() const override {
	*     AssetImportContext ctx;
	*     const auto& path = GetFullPath();
	*     if (path.empty()) return {};
	*     // fill ctx with path ...
	*     return ctx;
	*   }
	*  static void RegisterToUDRefl() {
	*    RegisterToUDReflHelper();
	*    // Register fields
	*    // Register Asset XXX
	*  }
	* 
	*  // fields
	* };
	*/
	class AssetImporter {
	public:
		AssetImporter() = default;
		AssetImporter(xg::Guid guid) : guid{ guid } {}
		virtual ~AssetImporter() = default;

		const xg::Guid& GetGuid() const noexcept { return guid; }
		std::filesystem::path GetFullPath() const;

		virtual UDRefl::ObjectView This() const noexcept = 0;

		// {
		//   "__TypeID":<uint64>,
		//   "__TypeName":<string>,
		//   "__Content":{
		//     "__Guid":<string>
		//     ...
		//   }
		// }
		virtual void Serialize(Serializer::SerializeContext& ctx) const {
			Serializer::SerializeRecursion(This(), ctx);
		}
		virtual std::string ReserializeAsset() const;
		virtual AssetImportContext ImportAsset() const = 0;

		static void RegisterToUDRefl(); // call by AssetMngr
	protected:
		template<typename T>
		static UDRefl::ObjectView TmplThis(const T* ptr) {
			return { Type_of<T>, const_cast<T*>(ptr) };
		}

		xg::Guid guid;
	};

	template<typename Impl>
	class TAssetImporter : public AssetImporter {
	public:
		using AssetImporter::AssetImporter;

		virtual UDRefl::ObjectView This() const noexcept override final {
			return TmplThis(static_cast<const Impl*>(this));
		}

	protected:
		static void RegisterToUDReflHelper() {
			UDRefl::Mngr.RegisterType<Impl>();
			UDRefl::Mngr.AddBases<Impl, AssetImporter>();
		}
	private:
		using AssetImporter::RegisterToUDRefl;
	};

	/*
	* [Template]
	* class XXXImporterCreator final : public TAssetImporterCreator<XXXImporter> {
	*   virtual std::vector<std::string> SupportedExtentions() const override {
	*     return { ".abc", ".def", ... };
	*   }
	* };
	*/
	class AssetImporterCreator {
	public:
		virtual ~AssetImporterCreator() = default;

		// the guid is not registered into asset mngr yet, but we can store it in assetimporter
		virtual std::shared_ptr<AssetImporter> CreateAssetImporter(xg::Guid guid) = 0;

		// reserialize
		virtual std::shared_ptr<AssetImporter> DeserializeAssetImporter(std::string_view json) {
			auto importer_impl = Serializer::Instance().Deserialize(json);
			auto importer_base = importer_impl.StaticCast_DerivedToBase(Type_of<AssetImporter>);
			if (!importer_base)
				return {};
			auto importer = importer_base.AsShared<AssetImporter>();
			return importer;
		}

		virtual std::vector<std::string> SupportedExtentions() const = 0;
	};

	template<typename Importer>
	class TAssetImporterCreator : public AssetImporterCreator {
	public:
		// the guid is not registered into asset mngr yet, but we can store it in assetimporter
		virtual std::shared_ptr<AssetImporter> CreateAssetImporter(xg::Guid guid) override final {
			OnceRegisterAssetImporterToUDRefl();
			return do_CreateAssetImporter(guid);
		}

		// reserialize
		virtual std::shared_ptr<AssetImporter> DeserializeAssetImporter(std::string_view json) override final {
			OnceRegisterAssetImporterToUDRefl();
			return do_DeserializeAssetImporter(json);
		}

	protected:
		virtual std::shared_ptr<AssetImporter> do_CreateAssetImporter(xg::Guid guid) {
			return std::make_shared<Importer>(guid);
		}
		virtual std::shared_ptr<AssetImporter> do_DeserializeAssetImporter(std::string_view json) {
			return AssetImporterCreator::DeserializeAssetImporter(json);
		}

	private:
		void OnceRegisterAssetImporterToUDRefl() {
			static bool init = false;
			if (init)
				return;
			Importer::RegisterToUDRefl();
			init = true;
		}
	};

	struct DefaultAsset {};

	class DefaultAssetImporter final : public TAssetImporter<DefaultAssetImporter> {
	public:
		using TAssetImporter<DefaultAssetImporter>::TAssetImporter;
		virtual AssetImportContext ImportAsset() const override;
		static void RegisterToUDRefl();
	};

	class DefaultAssetImporterCreator final : public TAssetImporterCreator<DefaultAssetImporter> {
		virtual std::vector<std::string> SupportedExtentions() const override { return {}; }
	};
}
