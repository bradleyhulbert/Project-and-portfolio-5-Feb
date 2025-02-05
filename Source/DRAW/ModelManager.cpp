#include "..\CCL.h"
#include "DrawComponents.h"
#include "..\GAME\GameComponents.h"

namespace DRAW
{


	void ConstructModelManager(entt::registry& registry, entt::entity entity) 
	{

	}
	 
	void DestroyModelManager(entt::registry& registry, entt::entity entity)
	{
		//TODO, figure out how to handle the destruction of MeshCollection here, because it will affect a later assignment. 
		// ASKTEACH
		//auto& MM = registry.get<ModelManager>(entity);
		/*registry.destroy<MeshCollection>(entity);*/
		/*registry.remove<MeshCollection>(entity);
		registry.remove<ModelManager>(entity);*/
	}
	void DestroyMeshCollection(entt::registry& registry, entt::entity entity)
	{
		if (registry.any_of<MeshCollection>(entity)) {
			auto& meshCollection = registry.get<MeshCollection>(entity);
			for (auto& meshEntity : meshCollection.entities) {
				registry.destroy(meshEntity);
			}
		}
	}
	

	CONNECT_COMPONENT_LOGIC() {
		registry.on_destroy<MeshCollection>().connect<DestroyMeshCollection>();
		registry.on_construct<ModelManager>().connect<ConstructModelManager>();
		registry.on_destroy<ModelManager>().connect<DestroyModelManager>();
		
	}

}