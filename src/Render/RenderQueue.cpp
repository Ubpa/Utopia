#include <Utopia/Render/RenderQueue.h>
#include <Utopia/Render/Material.h>
#include <Utopia/Render/Shader.h>
#include <Utopia/Render/Mesh.h>

#include <algorithm>

using namespace Ubpa::Utopia;

void RenderQueue::Add(RenderObject object) {
	if (object.material->shader->passes[object.passIdx].queue < (size_t)ShaderPass::Queue::Transparent)
		opaques.push_back(object);
	else
		transparents.push_back(object);
}

void RenderQueue::Sort(pointf3 cameraPos) {
	auto opaqueLess = [=](const RenderObject& lhs, const RenderObject& rhs) {
		auto lqueue = lhs.material->shader->passes[lhs.passIdx].queue;
		auto rqueue = rhs.material->shader->passes[rhs.passIdx].queue;
		if (lqueue < rqueue)
			return true;
		if (lqueue > rqueue)
			return false;

		auto lID = lhs.material->shader->GetInstanceID();
		auto rID = rhs.material->shader->GetInstanceID();
		if (lID < rID)
			return true;
		if (lID > rID)
			return false;

		if (lhs.material < rhs.material)
			return true;
		if (lhs.material > rhs.material)
			return false;

		auto lpos = lhs.mesh->GetSubMeshes().at(lhs.submeshIdx).bounds.center() + lhs.translation;
		auto rpos = rhs.mesh->GetSubMeshes().at(rhs.submeshIdx).bounds.center() + rhs.translation;

		return pointf3::distance(lpos, cameraPos) < pointf3::distance(rpos, cameraPos);
	};

	auto transparentLess = [=](const RenderObject& lhs, const RenderObject& rhs) {
		auto lqueue = lhs.material->shader->passes[lhs.passIdx].queue;
		auto rqueue = rhs.material->shader->passes[rhs.passIdx].queue;
		if (lqueue < rqueue)
			return true;
		if (lqueue > rqueue)
			return false;

		auto lpos = lhs.mesh->GetSubMeshes().at(lhs.submeshIdx).bounds.center() + lhs.translation;
		auto rpos = rhs.mesh->GetSubMeshes().at(rhs.submeshIdx).bounds.center() + rhs.translation;

		return pointf3::distance(lpos, cameraPos) > pointf3::distance(rpos, cameraPos);
	};

	std::sort(opaques.begin(), opaques.end(), opaqueLess);
	std::sort(transparents.begin(), transparents.end(), transparentLess);
}

void RenderQueue::Clear() {
	opaques.clear();
	transparents.clear();
}
