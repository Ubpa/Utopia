#pragma once

#include <Utopia/Render/Shader.h>

namespace Ubpa::Utopia {
    class UShaderCompiler {
    public:
        static UShaderCompiler& Instance() noexcept {
            static UShaderCompiler instance;
            return instance;
        }

        std::tuple<bool, Shader> Compile(std::string_view ushader);

	private:
        UShaderCompiler();
        ~UShaderCompiler();
        
        struct Impl;
        Impl* pImpl;
    };
}
