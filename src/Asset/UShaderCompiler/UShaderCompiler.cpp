#include "UShaderCompiler.h"

#include "_deps/UShaderBaseVisitor.h"
#include "_deps/UShaderLexer.h"
#include "_deps/UShaderParser.h"

#include <Utopia/Asset/AssetMngr.h>
#include <Utopia/Render/HLSLFile.h>
#include <Utopia/Render/Texture2D.h>
#include <Utopia/Render/TextureCube.h>

using namespace Ubpa::Utopia;

struct UShaderCompiler::Impl {
	class UShaderCompilerInstance : private details::UShaderBaseVisitor {
	public:
		std::tuple<bool, Shader> Compile(std::string_view ushader) {
			success = true;
			curPass = nullptr;

			antlr4::ANTLRInputStream input(ushader.data());
			details::UShaderLexer lexer(&input);
			antlr4::CommonTokenStream tokens(&lexer);
			details::UShaderParser parser(&tokens);

			Shader shader = visitShader(parser.shader());

			return { success, shader };
		}
	private:
		bool success{ true };
		ShaderPass* curPass{ nullptr };

		virtual antlrcpp::Any visitChildren(antlr4::tree::ParseTree* node) override {
			antlrcpp::Any result = defaultResult();
			size_t n = node->children.size();
			for (size_t i = 0; i < n; i++) {
				if (!shouldVisitNextChild(node, result)) {
					break;
				}

				antlrcpp::Any childResult = node->children[i]->accept(this);
				result = std::move(childResult);// aggregateResult(result, childResult);
			}

			return result;
		}

		static std::string_view StringLiteralContentView(const std::string& str) {
			std::string_view sv{ str };
			sv.remove_prefix(1);
			sv.remove_suffix(1);
			return sv;
		}

		virtual antlrcpp::Any visitShader(details::UShaderParser::ShaderContext* ctx) override {
			const Shader ERROR;

			Shader shader;

			// name
			auto name = ctx->shader_name()->StringLiteral()->getText();
			shader.name = StringLiteralContentView(name);
			if (shader.name.empty()) {
				success = false;
				return ERROR;
			}

			// hlsl
			auto guid_meta_s = ctx->hlsl()->StringLiteral()->getText();
			xg::Guid guid{ xg::Guid{ StringLiteralContentView(guid_meta_s) } };
			if (!guid.isValid()) {
				success = false;
				return ERROR;
			}
			const auto& path = AssetMngr::Instance().GUIDToAssetPath(guid);
			if (path.empty()) {
				success = false;
				return ERROR;
			}
			shader.hlslFile = AssetMngr::Instance().LoadAsset<HLSLFile>(path);
			if (!shader.hlslFile) {
				success = false;
				return ERROR;
			}

			// root parameters
			std::vector<RootParameter> rootParams = visitRoot_signature(ctx->root_signature());
			if (!success || rootParams.empty())
				return ERROR;
			shader.rootParameters = std::move(rootParams);

			// properties
			if (ctx->property_block()) {
				std::map<std::string, ShaderProperty, std::less<>> properties = visitProperty_block(ctx->property_block());
				if (!success)
					return ERROR;
				shader.properties = std::move(properties);
			}

			// passes
			for(auto passCtx : ctx->pass()) {
				ShaderPass pass = visitPass(passCtx);
				if (!success)
					return ERROR;
				shader.passes.push_back(std::move(pass));
			}

			return std::move(shader);
		}

		virtual antlrcpp::Any visitRoot_signature(details::UShaderParser::Root_signatureContext* ctx) override {
			const std::vector<RootParameter> ERROR;
			std::vector<RootParameter> rst;
			for (auto paramCtx : ctx->root_parameter()) {
				auto type_s = paramCtx->RootDescriptorType()->getText();
				RootDescriptorType type;
				if (type_s == "CBV")
					type = RootDescriptorType::CBV;
				else if (type_s == "SRV")
					type = RootDescriptorType::SRV;
				else if (type_s == "UAV")
					type = RootDescriptorType::UAV;
				else {
					success = false;
					return ERROR;
				}
				unsigned int base;
				unsigned int space;

				base = static_cast<unsigned int>(std::stoull(paramCtx->register_index()->shader_register()->getText(), nullptr, 0));
				if (auto spaceCtx = paramCtx->register_index()->register_space())
					space = static_cast<unsigned int>(std::stoull(spaceCtx->getText(), nullptr, 0));
				else
					space = 0;

				if (auto registerNumCtx = paramCtx->register_num()) {
					RootDescriptorTable table;
					DescriptorRange range;
					range.RangeType = type;
					range.RegisterSpace = space;
					range.NumDescriptors = static_cast<unsigned int>(std::stoull(registerNumCtx->getText(), nullptr, 0));
					range.BaseShaderRegister = base;
					table.push_back(range);
					rst.emplace_back(table);
				}
				else {
					RootDescriptor descriptor;
					descriptor.ShaderRegister = base;
					descriptor.RegisterSpace = space;
					descriptor.DescriptorType = type;
					rst.emplace_back(descriptor);
				}
			}
			return rst;
		}

		virtual antlrcpp::Any visitProperty_block(details::UShaderParser::Property_blockContext* ctx) override {
			const std::map<std::string, ShaderProperty, std::less<>> ERROR;

			std::map<std::string, ShaderProperty, std::less<>> rst;
			for (auto propertyCtx : ctx->property()) {
				std::pair<std::string, ShaderProperty> property = visitProperty(propertyCtx);
				if (!success)
					return ERROR;
				rst.insert(property);
			}
			return rst;
		}

		virtual antlrcpp::Any visitVal_bool(details::UShaderParser::Val_boolContext* ctx) override {
			if (ctx->getText() == "true")
				return true;
			else
				return false;
		}

		virtual antlrcpp::Any visitVal_int(details::UShaderParser::Val_intContext* ctx) override {
			int rst = std::stoi(ctx->IntegerLiteral()->getText(), nullptr, 0);
			if (ctx->Sign() && ctx->Sign()->getText() == "-")
				rst = -rst;
			return rst;
		}

		virtual antlrcpp::Any visitVal_uint(details::UShaderParser::Val_uintContext* ctx) override {
			return static_cast<unsigned int>(std::stoull(ctx->getText(), nullptr, 0));
		}

		virtual antlrcpp::Any visitVal_float(details::UShaderParser::Val_floatContext* ctx) override {
			float rst = std::stof(ctx->IntegerLiteral()->getText());
			if (ctx->Sign() && ctx->Sign()->getText() == "-")
				rst = -rst;
			return rst;
		}

		virtual antlrcpp::Any visitVal_double(details::UShaderParser::Val_doubleContext* ctx) override {
			double rst = std::stod(ctx->IntegerLiteral()->getText());
			if (ctx->Sign() && ctx->Sign()->getText() == "-")
				rst = -rst;
			return rst;
		}

		virtual antlrcpp::Any visitProperty_bool(details::UShaderParser::Property_boolContext* ctx) override {
			auto name = ctx->property_name()->getText();
			bool rst = ctx->val_bool()->accept(this);
			return std::pair{ name, ShaderProperty{rst} };
		}

		virtual antlrcpp::Any visitProperty_int(details::UShaderParser::Property_intContext* ctx) override {
			auto name = ctx->property_name()->getText();
			int rst = ctx->val_int()->accept(this);
			return std::pair{ name, ShaderProperty{rst} };
		}

		virtual antlrcpp::Any visitProperty_uint(details::UShaderParser::Property_uintContext* ctx) override {
			auto name = ctx->property_name()->getText();
			unsigned rst = ctx->val_uint()->accept(this);
			return std::pair{ name, ShaderProperty{rst} };
		}

		virtual antlrcpp::Any visitProperty_float(details::UShaderParser::Property_floatContext* ctx) override {
			auto name = ctx->property_name()->getText();
			float rst = std::stof(ctx->val_float()->getText());
			return std::pair{ name, ShaderProperty{rst} };
		}

		virtual antlrcpp::Any visitProperty_double(details::UShaderParser::Property_doubleContext* ctx) override {
			auto name = ctx->property_name()->getText();
			double rst = ctx->val_double()->accept(this);
			return std::pair{ name, ShaderProperty{rst} };
		}

		virtual antlrcpp::Any visitProperty_bool2(details::UShaderParser::Property_bool2Context* ctx) override {
			auto name = ctx->property_name()->getText();
			val<bool, 2> rst{
				ctx->val_bool2()->val_bool(0)->accept(this).as<bool>(),
				ctx->val_bool2()->val_bool(1)->accept(this).as<bool>(),
			};
			return std::pair{ name, rst };
		}

		virtual antlrcpp::Any visitProperty_bool3(details::UShaderParser::Property_bool3Context* ctx) override {
			auto name = ctx->property_name()->getText();
			val<bool, 3> rst{
				ctx->val_bool3()->val_bool(0)->accept(this).as<bool>(),
				ctx->val_bool3()->val_bool(1)->accept(this).as<bool>(),
				ctx->val_bool3()->val_bool(2)->accept(this).as<bool>(),
			};
			return std::pair{ name, rst };
		}

		virtual antlrcpp::Any visitProperty_bool4(details::UShaderParser::Property_bool4Context* ctx) override {
			auto name = ctx->property_name()->getText();
			val<bool, 4> rst{
				ctx->val_bool4()->val_bool(0)->accept(this).as<bool>(),
				ctx->val_bool4()->val_bool(1)->accept(this).as<bool>(),
				ctx->val_bool4()->val_bool(2)->accept(this).as<bool>(),
				ctx->val_bool4()->val_bool(3)->accept(this).as<bool>(),
			};
			return std::pair{ name, rst };
		}

		virtual antlrcpp::Any visitProperty_int2(details::UShaderParser::Property_int2Context* ctx) override {
			auto name = ctx->property_name()->getText();
			vali2 rst{
				ctx->val_int2()->val_int(0)->accept(this).as<int>(),
				ctx->val_int2()->val_int(1)->accept(this).as<int>(),
			};
			return std::pair{ name, ShaderProperty{rst} };
		}

		virtual antlrcpp::Any visitProperty_int3(details::UShaderParser::Property_int3Context* ctx) override {
			auto name = ctx->property_name()->getText();
			vali3 rst{
				ctx->val_int3()->val_int(0)->accept(this).as<int>(),
				ctx->val_int3()->val_int(1)->accept(this).as<int>(),
				ctx->val_int3()->val_int(2)->accept(this).as<int>(),
			};
			return std::pair{ name, ShaderProperty{rst} };
		}

		virtual antlrcpp::Any visitProperty_int4(details::UShaderParser::Property_int4Context* ctx) override {
			auto name = ctx->property_name()->getText();
			vali4 rst{
				ctx->val_int4()->val_int(0)->accept(this).as<int>(),
				ctx->val_int4()->val_int(1)->accept(this).as<int>(),
				ctx->val_int4()->val_int(2)->accept(this).as<int>(),
				ctx->val_int4()->val_int(3)->accept(this).as<int>(),
			};
			return std::pair{ name, ShaderProperty{rst} };
		}

		virtual antlrcpp::Any visitProperty_uint2(details::UShaderParser::Property_uint2Context* ctx) override {
			auto name = ctx->property_name()->getText();
			vali2 rst{
				ctx->val_uint2()->val_uint(0)->accept(this).as<unsigned>(),
				ctx->val_uint2()->val_uint(1)->accept(this).as<unsigned>(),
			};
			return std::pair{ name, ShaderProperty{rst} };
		}

		virtual antlrcpp::Any visitProperty_uint3(details::UShaderParser::Property_uint3Context* ctx) override {
			auto name = ctx->property_name()->getText();
			vali3 rst{
				ctx->val_uint3()->val_uint(0)->accept(this).as<unsigned>(),
				ctx->val_uint3()->val_uint(1)->accept(this).as<unsigned>(),
				ctx->val_uint3()->val_uint(2)->accept(this).as<unsigned>(),
			};
			return std::pair{ name, ShaderProperty{rst} };
		}

		virtual antlrcpp::Any visitProperty_uint4(details::UShaderParser::Property_uint4Context* ctx) override {
			auto name = ctx->property_name()->getText();
			vali4 rst{
				ctx->val_uint4()->val_uint(0)->accept(this).as<unsigned>(),
				ctx->val_uint4()->val_uint(1)->accept(this).as<unsigned>(),
				ctx->val_uint4()->val_uint(2)->accept(this).as<unsigned>(),
				ctx->val_uint4()->val_uint(3)->accept(this).as<unsigned>(),
			};
			return std::pair{ name, ShaderProperty{rst} };
		}

		virtual antlrcpp::Any visitProperty_float2(details::UShaderParser::Property_float2Context* ctx) override {
			auto name = ctx->property_name()->getText();
			valf2 rst{
				std::stof(ctx->val_float2()->val_float(0)->getText()),
				std::stof(ctx->val_float2()->val_float(1)->getText()),
			};
			return std::pair{ name, ShaderProperty{rst} };
		}

		virtual antlrcpp::Any visitProperty_float3(details::UShaderParser::Property_float3Context* ctx) override {
			auto name = ctx->property_name()->getText();
			valf3 rst{
				std::stof(ctx->val_float3()->val_float(0)->getText()),
				std::stof(ctx->val_float3()->val_float(1)->getText()),
				std::stof(ctx->val_float3()->val_float(2)->getText()),
			};
			return std::pair{ name, ShaderProperty{rst} };
		}

		virtual antlrcpp::Any visitProperty_float4(details::UShaderParser::Property_float4Context* ctx) override {
			auto name = ctx->property_name()->getText();
			valf4 rst{
				std::stof(ctx->val_float4()->val_float(0)->getText()),
				std::stof(ctx->val_float4()->val_float(1)->getText()),
				std::stof(ctx->val_float4()->val_float(2)->getText()),
				std::stof(ctx->val_float4()->val_float(3)->getText()),
			};
			return std::pair{ name, ShaderProperty{rst} };
		}

		virtual antlrcpp::Any visitProperty_double2(details::UShaderParser::Property_double2Context* ctx) override {
			auto name = ctx->property_name()->getText();
			valf2 rst{
				std::stod(ctx->val_double2()->val_double(0)->getText()),
				std::stod(ctx->val_double2()->val_double(1)->getText()),
			};
			return std::pair{ name, ShaderProperty{rst} };
		}

		virtual antlrcpp::Any visitProperty_double3(details::UShaderParser::Property_double3Context* ctx) override {
			auto name = ctx->property_name()->getText();
			valf3 rst{
				std::stod(ctx->val_double3()->val_double(0)->getText()),
				std::stod(ctx->val_double3()->val_double(1)->getText()),
				std::stod(ctx->val_double3()->val_double(2)->getText()),
			};
			return std::pair{ name, ShaderProperty{rst} };
		}

		virtual antlrcpp::Any visitProperty_double4(details::UShaderParser::Property_double4Context* ctx) override {
			auto name = ctx->property_name()->getText();
			valf4 rst{
				std::stod(ctx->val_double4()->val_double(0)->getText()),
				std::stod(ctx->val_double4()->val_double(1)->getText()),
				std::stod(ctx->val_double4()->val_double(2)->getText()),
				std::stod(ctx->val_double4()->val_double(3)->getText()),
			};
			return std::pair{ name, ShaderProperty{rst} };
		}

		virtual antlrcpp::Any visitProperty_2D(details::UShaderParser::Property_2DContext* ctx) override {
			std::shared_ptr<const Texture2D> tex2d = ctx->val_tex2d()->accept(this);
			if (!success)
				return std::pair{ std::string{}, (const Texture2D*)nullptr };
			auto name = ctx->property_name()->getText();
			return std::pair{ name, ShaderProperty{tex2d} };
		}

		virtual antlrcpp::Any visitVal_tex2d(details::UShaderParser::Val_tex2dContext* ctx) override {
			static const std::shared_ptr<const Texture2D> ERROR;
			xg::Guid guid;
			if (ctx->default_texture_2d()) {
				auto name = ctx->default_texture_2d()->getText();
				if (name == "White")
					guid = xg::Guid{ "1936ed7e-6896-4ace-abd9-5b084fcfb891" };
				else if (name == "Black")
					guid = xg::Guid{ "ece48884-cc0d-4288-be5e-c58a6d2ea187" };
				else if (name == "Bump")
					guid = xg::Guid{ "b5e7fb39-fedd-4371-a00a-552a86307db7" };
				else {
					assert(false);
					success = false;
					return ERROR;
				}
				
			}
			else {
				auto guid_s = ctx->StringLiteral()->getText();
				guid = xg::Guid{ StringLiteralContentView(guid_s) };
			}

			if (!guid.isValid()) {
				success = false;
				return ERROR;
			}
			const auto& path = AssetMngr::Instance().GUIDToAssetPath(guid);
			if (path.empty()) {
				success = false;
				return ERROR;
			}
			auto tex2d = AssetMngr::Instance().LoadAsset<Texture2D>(path);
			if (!tex2d) {
				success = false;
				return ERROR;
			}
			return std::const_pointer_cast<const Texture2D>(tex2d);
		}

		virtual antlrcpp::Any visitProperty_cube(details::UShaderParser::Property_cubeContext* ctx) override {
			std::shared_ptr<const TextureCube> texcube = ctx->val_texcube()->accept(this);
			if (!success)
				return std::pair{ std::string{}, (const TextureCube*)nullptr };
			auto name = ctx->property_name()->getText();
			return std::pair{ name, ShaderProperty{texcube} };
		}

		virtual antlrcpp::Any visitVal_texcube(details::UShaderParser::Val_texcubeContext* ctx) override {
			static const std::shared_ptr<const TextureCube> ERROR = nullptr;
			xg::Guid guid;
			if (ctx->default_texture_cube()) {
				auto name = ctx->default_texture_cube()->getText();
				if (name == "White")
					guid = xg::Guid{ "ca4f09fc-b1fb-4a45-99c1-c2d7bfe828c2" };
				else if (name == "Balck")
					guid = xg::Guid{ "4fcdaad5-f960-4d8f-aab8-29b771636256" };
				else {
					assert(false);
					success = false;
					return ERROR;
				}

			}
			else {
				auto guid_s = ctx->StringLiteral()->getText();
				guid = xg::Guid{ StringLiteralContentView(guid_s) };
			}

			if (!guid.isValid()) {
				success = false;
				return ERROR;
			}
			const auto& path = AssetMngr::Instance().GUIDToAssetPath(guid);
			if (path.empty()) {
				success = false;
				return ERROR;
			}
			auto texcube = AssetMngr::Instance().LoadAsset<TextureCube>(path);
			if (!texcube) {
				success = false;
				return ERROR;
			}
			return std::const_pointer_cast<const TextureCube>(texcube);
		}

		virtual antlrcpp::Any visitProperty_rgb(details::UShaderParser::Property_rgbContext* ctx) override {
			auto name = ctx->property_name()->getText();
			valf3 f3 = ctx->val_float3()->accept(this);
			return std::pair{ name, ShaderProperty{f3.as<rgbf>()} };
		}

		virtual antlrcpp::Any visitProperty_rgba(details::UShaderParser::Property_rgbaContext* ctx) override {
			auto name = ctx->property_name()->getText();
			valf4 f4 = ctx->val_float4()->accept(this);
			return std::pair{ name, ShaderProperty{f4.cast_to<rgbaf>()} };
		}

		virtual antlrcpp::Any visitVal_float3(details::UShaderParser::Val_float3Context* ctx) override {
			valf3 rst{
				ctx->val_float(0)->accept(this).as<float>(),
				ctx->val_float(1)->accept(this).as<float>(),
				ctx->val_float(2)->accept(this).as<float>(),
			};
			return rst;
		}

		virtual antlrcpp::Any visitVal_float4(details::UShaderParser::Val_float4Context* ctx) override {
			valf4 rst{
				ctx->val_float(0)->accept(this).as<float>(),
				ctx->val_float(1)->accept(this).as<float>(),
				ctx->val_float(2)->accept(this).as<float>(),
				ctx->val_float(3)->accept(this).as<float>(),
			};
			return rst;
		}

		virtual antlrcpp::Any visitPass(details::UShaderParser::PassContext* ctx) override {
			const ShaderPass ERROR;
			ShaderPass pass;
			pass.vertexName = ctx->vs()->getText();
			pass.fragmentName = ctx->ps()->getText();

			curPass = &pass;
			visitChildren(ctx);
			curPass = nullptr;

			return pass;
		}

		virtual antlrcpp::Any visitTags(details::UShaderParser::TagsContext* ctx) override {
			for (auto tagCtx : ctx->tag()) {
				auto key_s = tagCtx->StringLiteral(0)->getText();
				auto mapped_s = tagCtx->StringLiteral(1)->getText();
				curPass->tags.emplace(
					std::string{ StringLiteralContentView(key_s) },
					std::string{ StringLiteralContentView(mapped_s) }
				);
			}
			return {};
		}

		virtual antlrcpp::Any visitCull(details::UShaderParser::CullContext* ctx) override {
			auto mode = ctx->CullMode()->getText();
			if (mode == "Front")
				curPass->renderState.cullMode = CullMode::FRONT;
			else if (mode == "Back")
				curPass->renderState.cullMode = CullMode::BACK;
			else if (mode == "Off")
				curPass->renderState.cullMode = CullMode::NONE;
			else {
				assert(false);
				success = false;
			}
			return {};
		}

		virtual antlrcpp::Any visitFill(details::UShaderParser::FillContext* ctx) override {
			auto mode = ctx->FillMode()->getText();
			if (mode == "Wireframe")
				curPass->renderState.fillMode = FillMode::WIREFRAME;
			else if (mode == "Solid")
				curPass->renderState.fillMode = FillMode::SOLID;
			else {
				assert(false);
				success = false;
			}
			return {};
		}

		static CompareFunc DecodeComparatorStr(const std::string& str) {
			if (str == "Less")
				return CompareFunc::LESS;
			else if (str == "Greater")
				return CompareFunc::GREATER;
			else if (str == "LEqual")
				return CompareFunc::LESS_EQUAL;
			else if (str == "GEqaul")
				return CompareFunc::GREATER_EQUAL;
			else if (str == "Equal")
				return CompareFunc::EQUAL;
			else if (str == "NotEqual")
				return CompareFunc::NOT_EQUAL;
			else if (str == "Always")
				return CompareFunc::NEVER;
			else {
				assert(str == "Never");
				return CompareFunc::NEVER;
			}
		}

		virtual antlrcpp::Any visitZtest(details::UShaderParser::ZtestContext* ctx) override {
			curPass->renderState.zTest = DecodeComparatorStr(ctx->Comparator()->getText());
			return {};
		}

		virtual antlrcpp::Any visitZwrite_off(details::UShaderParser::Zwrite_offContext* ctx) override {
			curPass->renderState.zWrite = false;
			return {};
		}

		static Blend DecodeBlendStr(const std::string& str) {
			if (str == "Zero")
				return Blend::ZERO;
			else if (str == "One")
				return Blend::ONE;
			else if (str == "SrcAlpha")
				return Blend::SRC_ALPHA;
			else if (str == "SrcColor")
				return Blend::SRC_COLOR;
			else if (str == "DstAlpha")
				return Blend::DEST_ALPHA;
			else if (str == "DstColor")
				return Blend::DEST_COLOR;
			else if (str == "OneMinusSrcAlpha")
				return Blend::INV_SRC_ALPHA;
			else if (str == "OneMinusSrcColor")
				return Blend::INV_SRC_COLOR;
			else if (str == "OneMinusDstAlpha")
				return Blend::INV_DEST_ALPHA;
			else if (str == "OneMinusDstColor")
				return Blend::INV_DEST_COLOR;
			else {
				assert(false);
				return Blend::ZERO;
			}
		}

		virtual antlrcpp::Any visitBlend(details::UShaderParser::BlendContext* ctx) override {
			size_t index;
			if (auto indexCtx = ctx->index()) {
				index = std::stoull(indexCtx->getText(), nullptr, 0);
				if (index >= 8) {
					success = false;
					return {};
				}
			}
			else
				index = 0;

			curPass->renderState.blendStates[index].enable = true;

			if (auto expr = ctx->blend_expr()) {
				if (auto srcCtx = expr->blend_src_factor_color())
					curPass->renderState.blendStates[index].src = DecodeBlendStr(srcCtx->getText());
				if (auto destCtx = expr->blend_dst_factor_color())
					curPass->renderState.blendStates[index].dest = DecodeBlendStr(destCtx->getText());
				if (auto srcACtx = expr->blend_src_factor_alpha())
					curPass->renderState.blendStates[index].srcAlpha = DecodeBlendStr(srcACtx->getText());
				if (auto dstACtx = expr->blend_dst_factor_alpha())
					curPass->renderState.blendStates[index].destAlpha = DecodeBlendStr(dstACtx->getText());
			}

			return {};
		}

		static BlendOp DecodeBlendOpStr(const std::string& str) {
			if (str == "Add")
				return BlendOp::ADD;
			else if (str == "Sub")
				return BlendOp::SUBTRACT;
			else if (str == "RevSub")
				return BlendOp::REV_SUBTRACT;
			else if (str == "Min")
				return BlendOp::MIN;
			else if (str == "Max")
				return BlendOp::MAX;
			else {
				assert(false);
				return BlendOp::ADD;
			}
		}

		virtual antlrcpp::Any visitBlend_op(details::UShaderParser::Blend_opContext* ctx) override {
			size_t index;
			if (auto indexCtx = ctx->index()) {
				index = std::stoull(indexCtx->getText(), nullptr, 0);
				if (index >= 8) {
					success = false;
					return {};
				}
			}
			else
				index = 0;
			curPass->renderState.blendStates[index].enable = true;
			curPass->renderState.blendStates[index].op = DecodeBlendOpStr(ctx->blend_op_color()->getText());
			if (auto opACtx = ctx->blend_op_alpha())
				curPass->renderState.blendStates[index].opAlpha = DecodeBlendOpStr(opACtx->getText());

			return {};
		}

		virtual antlrcpp::Any visitColor_mask(details::UShaderParser::Color_maskContext* ctx) override {
			size_t index;
			if (auto indexCtx = ctx->index()) {
				index = std::stoull(indexCtx->getText(), nullptr, 0);
				if (index >= 8) {
					success = false;
					return {};
				}
			}
			else
				index = 0;

			uint8_t mask = 0;
			auto mask_s = ctx->color_mask_value()->getText();
			if (auto integerCtx = ctx->color_mask_value()->IntegerLiteral())
				mask = 0b1111 & static_cast<uint8_t>(std::stoull(mask_s, nullptr, 0));
			else {
				if (mask_s.find('R') != std::string::npos)
					mask |= 0b1;
				if (mask_s.find('G') != std::string::npos)
					mask |= 0b10;
				if (mask_s.find('B') != std::string::npos)
					mask |= 0b100;
				if (mask_s.find('A') != std::string::npos)
					mask |= 0b1000;
			}
			curPass->renderState.colorMask[index] = mask;

			return {};
		}

		virtual antlrcpp::Any visitStencil(details::UShaderParser::StencilContext* ctx) override {
			curPass->renderState.stencilState.enable = true;
			return visitChildren(ctx);
		}

		virtual antlrcpp::Any visitStencil_ref(details::UShaderParser::Stencil_refContext* ctx) override {
			curPass->renderState.stencilState.ref = static_cast<uint8_t>(std::stoull(ctx->IntegerLiteral()->getText(), nullptr, 0));
			return {};
		}

		virtual antlrcpp::Any visitStencil_mask_read(details::UShaderParser::Stencil_mask_readContext* ctx) override {
			curPass->renderState.stencilState.readMask = static_cast<uint8_t>(std::stoull(ctx->IntegerLiteral()->getText(), nullptr, 0));
			return {};
		}

		virtual antlrcpp::Any visitStencil_mask_write(details::UShaderParser::Stencil_mask_writeContext* ctx) override {
			curPass->renderState.stencilState.writeMask = static_cast<uint8_t>(std::stoull(ctx->IntegerLiteral()->getText(), nullptr, 0));
			return {};
		}

		virtual antlrcpp::Any visitStencil_compare(details::UShaderParser::Stencil_compareContext* ctx) override {
			curPass->renderState.stencilState.func = DecodeComparatorStr(ctx->Comparator()->getText());
			return {};
		}

		static StencilOp DecodeStencilOpStr(const std::string& str) {
			if (str == "Keep")
				return StencilOp::KEEP;
			else if (str == "Zero")
				return StencilOp::ZERO;
			else if (str == "Replace")
				return StencilOp::REPLACE;
			else if (str == "IncrSat")
				return StencilOp::INCR_SAT;
			else if (str == "DecrSat")
				return StencilOp::DECR_SAT;
			else if (str == "Invert")
				return StencilOp::INVERT;
			else if (str == "IncrWrap")
				return StencilOp::INCR;
			else if (str == "DecrWrap")
				return StencilOp::DECR;
			else {
				assert(false);
				return StencilOp::KEEP;
			}
		}

		virtual antlrcpp::Any visitStencil_pass(details::UShaderParser::Stencil_passContext* ctx) override {
			curPass->renderState.stencilState.passOp = DecodeStencilOpStr(ctx->stencil_op()->getText());
			return {};
		}

		virtual antlrcpp::Any visitStencil_fail(details::UShaderParser::Stencil_failContext* ctx) override {
			curPass->renderState.stencilState.failOp = DecodeStencilOpStr(ctx->stencil_op()->getText());
			return {};
		}

		virtual antlrcpp::Any visitStencil_zfail(details::UShaderParser::Stencil_zfailContext* ctx) override {
			curPass->renderState.stencilState.depthFailOp = DecodeStencilOpStr(ctx->stencil_op()->getText());
			return {};
		}

		virtual antlrcpp::Any visitQueue(details::UShaderParser::QueueContext* ctx) override {
			size_t base;
			if (auto keyCtx = ctx->val_queue()->queue_key()) {
				if (keyCtx->getText() == "Background")
					base = (size_t)ShaderPass::Queue::Background;
				else if (keyCtx->getText() == "Geometry")
					base = (size_t)ShaderPass::Queue::Geometry;
				else if (keyCtx->getText() == "AlphaTest")
					base = (size_t)ShaderPass::Queue::AlphaTest;
				else if (keyCtx->getText() == "Transparent")
					base = (size_t)ShaderPass::Queue::Transparent;
				else if (keyCtx->getText() == "Overlay")
					base = (size_t)ShaderPass::Queue::Overlay;
				else {
					assert(false);
					base = (size_t)ShaderPass::Queue::Geometry;
				}
			}
			else
				base = 0;
			
			size_t offset;
			if (auto integerCtx = ctx->val_queue()->IntegerLiteral())
				offset = std::stoull(integerCtx->getText(), nullptr, 0);
			else
				offset = 0;

			size_t queue;
			if (auto signCtx = ctx->val_queue()->Sign(); signCtx && signCtx->getText() != "-")
				queue = base + offset;
			else {
				if (base > offset)
					queue = base - offset;
				else
					queue = 0;
			}

			curPass->queue = queue;
			return {};
		}
	};
};

UShaderCompiler::UShaderCompiler()
	:pImpl{ new Impl } {}

UShaderCompiler::~UShaderCompiler() {
	delete pImpl;
}

std::tuple<bool, Shader> UShaderCompiler::Compile(std::string_view ushader) {
	Shader shader;
	Impl::UShaderCompilerInstance compiler;
	return compiler.Compile(ushader);
}
