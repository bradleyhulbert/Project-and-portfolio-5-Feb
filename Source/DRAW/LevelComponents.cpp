#include "..\CCL.h"
#include "DrawComponents.h"
#include "..\GAME\GameComponents.h"

namespace DRAW
{
	void ConstructCPULevel(entt::registry& registry, entt::entity entity)
	{
		GW::SYSTEM::GLog Glog;
		GW::SYSTEM::GFile Gfile;
		Glog.Create(Gfile);
		Glog.EnableConsoleLogging(true);
		auto& cpuLevel = registry.get<CPULevel>(entity);
		//professor gave a fix in discord for the json race condition. fix it when possible. 
		bool checkLevelLoad;
		checkLevelLoad = cpuLevel.levelData.LoadLevel(cpuLevel.levelPath.c_str(), cpuLevel.modelPath.c_str(), Glog);
		if (!checkLevelLoad) {
			std::cout << ".......LEVEL FAILED TO LOAD........ check LevelComponents.cpp" << std::endl;
		}
		else {
			std::cout << "--------------------LEVEL LOADED SUCCESFULY--------------------" << std::endl;
		}
	}
	void ConstructGPULevel(entt::registry& registry, entt::entity entity)
	{   // gets the current CpuLevel to access and alter its data
		auto levelData = registry.try_get<CPULevel>(entity);
		if (levelData == nullptr) {
			std::cout << ".......ConstructGPULevel failed, check LevelComponents.cpp" << std::endl; //check if failure
		}
	
		//emplaces new vertex and index buffers to the entity
		auto& vBufferData = registry.emplace<VulkanVertexBuffer>(entity);
		auto& iBufferData = registry.emplace<VulkanIndexBuffer>(entity); 
		//emplaces the data for those buffers onto the entity
		registry.emplace<std::vector<H2B::VERTEX>>(entity, levelData->levelData.levelVertices);
		registry.emplace<std::vector<unsigned>>(entity, levelData->levelData.levelIndices);
		//patches the buffers to add the data above
		registry.patch<VulkanVertexBuffer>(entity);
		registry.patch<VulkanIndexBuffer>(entity);
		//creates and emplaces an entity with a ModelManager component. then retrieves it.
		entt::entity mdlManager = registry.create();
		registry.emplace<ModelManager>(mdlManager);
		auto& MM = registry.get<ModelManager>(mdlManager);

		// for each blender object
		for (size_t i = 0; i < levelData->levelData.blenderObjects.size(); i++)
		{  // for each mesh in that blender object (used for objects with multiple meshes)
			
			//THIS MIGHT NEED TO CHANGE, I MIGHT NEED TO DO SOMETHING DIFFERENT WITH THE ENTITY IMPLEMANTATION HERE.
			entt::entity mcEntity = registry.create();
			auto& meshC = registry.emplace<MeshCollection>(mcEntity);
			

			for (size_t x = 0; x < levelData->levelData.levelModels[levelData->levelData.blenderObjects[i].modelIndex].meshCount; x++)
			{

				//creating a new entity for each mesh, then emplacing geometrydata and gpuinstance on each one
				entt::entity newEntity = registry.create();
				auto& geoData = registry.emplace<GeometryData>(newEntity);
				auto& gpuInstance = registry.emplace<GPUInstance>(newEntity);
				//for each mesh's model that has dymanic models enabled, emplace DoNotRender onto their entity
				if (levelData->levelData.levelModels[levelData->levelData.blenderObjects[i].modelIndex].isDynamic == true) {
					registry.emplace<DoNotRender>(newEntity);
				}
				//properly filling those with the level data from levelData
				geoData.indexCount = levelData->levelData.levelModels[levelData->levelData.blenderObjects[i].modelIndex].indexCount;
				geoData.indexStart = levelData->levelData.levelModels[levelData->levelData.blenderObjects[i].modelIndex].indexStart;
				geoData.vertexStart = levelData->levelData.levelModels[levelData->levelData.blenderObjects[i].modelIndex].vertexStart;

				gpuInstance.transform = levelData->levelData.levelTransforms[i];
				gpuInstance.matData = levelData->levelData.levelMaterials[levelData->levelData.blenderObjects[i].modelIndex].attrib;
				// accesses the models collider using the index data inside levelmodels
				meshC.collider = levelData->levelData.levelColliders
					[levelData->levelData.levelModels[levelData->levelData.blenderObjects[i].modelIndex].colliderIndex];
				//MIGHT NEED TO CHANGE THIS LATER IF THE DOC LITERALLY MEANS MESH OBJECTS OR JUST THE TRANSFORMS.
				
				if (levelData->levelData.levelModels[levelData->levelData.blenderObjects[i].modelIndex].isDynamic == true) {
					meshC.entities.resize(levelData->levelData.levelModels[levelData->levelData.blenderObjects[i].modelIndex].meshCount);
					meshC.entities[x] = registry.create();
					//emplaces a blank matrix to the meshC to be added to model manager
					auto& mat = registry.emplace<GW::MATH::GMATRIXF>(meshC.entities[x]);
					//changes the matrix to match the proper one from blenderobjects
					mat = levelData->levelData.levelTransforms[levelData->levelData.blenderObjects[i].modelIndex];
					registry.emplace<GeometryData>(meshC.entities[x], geoData);
					registry.emplace<GPUInstance>(meshC.entities[x], gpuInstance);
					
					
				}
				//collidable shit
				if (levelData->levelData.levelModels[levelData->levelData.blenderObjects[i].modelIndex].isCollidable == true) {
					auto isCollide = registry.create();
					registry.emplace<GAME::Collidable>(isCollide);
					registry.emplace<GAME::Obstacle>(isCollide);
					auto& collideMC = registry.emplace<MeshCollection>(isCollide, meshC);
					auto& collideTransform = registry.emplace<GAME::Transform>(isCollide);
					collideTransform.mat = gpuInstance.transform;
				}
			
			}
			MM.mcMap[levelData->levelData.blenderObjects[i].blendername] = meshC;
		}
		
		
	}
	// Use this MACRO to connect the EnTT Component Logic
	CONNECT_COMPONENT_LOGIC() {
		// register the Window component's logic
		registry.on_construct<CPULevel>().connect<ConstructCPULevel>();
		registry.on_construct<GPULevel>().connect<ConstructGPULevel>();
		//when you emplace something to be a cpu level or gpu level, these functions get called 
	
	}
}