/*
MIT License

Copyright (c) 2020 Ubpa

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

#include "imconfig.h"
#include "imgui_internal.h"

ImGui::WrapContextGuard::WrapContextGuard(ImGuiContext* ctx)
{
    prevCtx = GetCurrentContext();
    SetCurrentContext(ctx);
}

ImGui::WrapContextGuard::~WrapContextGuard() { SetCurrentContext(prevCtx); }

ImGuiIO& ImGui::GetIO(ImGuiContext* ctx) { return ctx->IO; }

ImGuiMouseCursor ImGui::GetMouseCursor(ImGuiContext* ctx) { return ctx->MouseCursor; }

bool ImGui::IsAnyMouseDown(ImGuiContext* ctx)
{
    ImGuiContext& g = *ctx;
    for (int n = 0; n < IM_ARRAYSIZE(g.IO.MouseDown); n++)
        if (g.IO.MouseDown[n])
            return true;
    return false;
}

void ImGui::Hack_CorrectFrameCountPlatformEnded() {
    ImGuiContext* ctx = ImGui::GetCurrentContext();
    IM_ASSERT(ctx && ctx->FrameCountPlatformEnded <= ctx->FrameCount);
    if (ctx->FrameCountPlatformEnded == ctx->FrameCount)
        ctx->FrameCountPlatformEnded = ctx->FrameCount - 1;
}
