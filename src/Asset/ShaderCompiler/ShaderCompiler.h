#pragma once

#include <Utopia/Render/Shader.h>

namespace Ubpa::Utopia {
    class ShaderCompiler {
    public:
        static ShaderCompiler& Instance() noexcept {
            static ShaderCompiler instance;
            return instance;
        }

        std::tuple<bool, Shader> Compile(std::string_view ushader);

	private:
        ShaderCompiler();
        ~ShaderCompiler();
        
        struct Impl;
        Impl* pImpl;
    };
}
