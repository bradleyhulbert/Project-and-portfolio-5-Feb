// main entry point for the application
// enables components to define their behaviors locally in an .hpp file
#include "CCL.h"
#include "UTIL/Utilities.h"
// include all components, tags, and systems used by this program
#include "DRAW/DrawComponents.h"
#include "GAME/GameComponents.h"
#include "APP/Window.hpp"
#include <random>



// Local routines for specific application behavior
void GraphicsBehavior(entt::registry& registry);
void GameplayBehavior(entt::registry& registry);
void MainLoopBehavior(entt::registry& registry);

// Architecture is based on components/entities pushing updates to other components/entities (via "patch" function)
int main()
{

	// All components, tags, and systems are stored in a single registry
	entt::registry registry;	

	// initialize the ECS Component Logic
	CCL::InitializeComponentLogic(registry);

	auto gameConfig = registry.create();
	registry.emplace<UTIL::Config>(gameConfig);

	GraphicsBehavior(registry); // create windows, surfaces, and renderers

	GameplayBehavior(registry); // create entities and components for gameplay
	
	MainLoopBehavior(registry); // update windows and input

	// clear all entities and components from the registry
	// invokes on_destroy() for all components that have it
	// registry will still be intact while this is happening
	registry.clear(); 

	return 0; // now destructors will be called for all components
}

// This function will be called by the main loop to update the graphics
// It will be responsible for loading the Level, creating the VulkanRenderer, and all VulkanInstances
void GraphicsBehavior(entt::registry& registry)
{
	std::shared_ptr<const GameConfig> config = registry.get<UTIL::Config>(
		registry.view<UTIL::Config>().front()).gameConfig;

	int windowWidth = (*config).at("Window").at("width").as<int>();
	int windowHeight = (*config).at("Window").at("height").as<int>();
	int startX = (*config).at("Window").at("xstart").as<int>();
	int startY = (*config).at("Window").at("ystart").as<int>();
	std::string title = (*config).at("Window").at("title").as<std::string>();

	// Add an entity to handle all the graphics data
	auto display = registry.create();
	registry.emplace<APP::Window>(display,
		APP::Window{ startX, startY, windowWidth, windowHeight, GW::SYSTEM::GWindowStyle::WINDOWEDBORDERED, title});

	std::string vertShader = (*config).at("Shaders").at("vertex").as<std::string>();
	std::string pixelShader = (*config).at("Shaders").at("pixel").as<std::string>();
	registry.emplace<DRAW::VulkanRendererInitialization>(display,
		DRAW::VulkanRendererInitialization{ 
			vertShader, pixelShader,
			{ {0.2f, 0.2f, 0.25f, 1} } , { 1.0f, 0u }, 75.f, 0.1f, 100.0f });
	registry.emplace<DRAW::VulkanRenderer>(display);
	
	// TODO : Load the Level then update the Vertex and Index Buffers
	//CODECHANGE
	std::string levelPath = (*config).at("Level1").at("levelFile").as<std::string>();
	std::string modelPath = (*config).at("Level1").at("modelPath").as<std::string>();
	registry.emplace<DRAW::CPULevel>(display, DRAW::CPULevel{levelPath, modelPath});
	registry.emplace<DRAW::GPULevel>(display);
	//CODESTOP

	// Register for Vulkan clean up
	GW::CORE::GEventResponder shutdown;
	shutdown.Create([&](const GW::GEvent& e) {
		GW::GRAPHICS::GVulkanSurface::Events event;
		GW::GRAPHICS::GVulkanSurface::EVENT_DATA data;
		if (+e.Read(event, data) && event == GW::GRAPHICS::GVulkanSurface::Events::RELEASE_RESOURCES) {
			registry.clear<DRAW::VulkanRenderer>();
		}
		});
	registry.get<DRAW::VulkanRenderer>(display).vlkSurface.Register(shutdown);
	registry.emplace<GW::CORE::GEventResponder>(display, shutdown.Relinquish());

	// Create a camera entity and emplace it
	auto camera = registry.create();
	GW::MATH::GMATRIXF initialCamera;
	GW::MATH::GVECTORF translate = { 0.0f,  45.0f, -5.0f };
	GW::MATH::GVECTORF lookat = { 0.0f, 0.0f, 0.0f };
	GW::MATH::GVECTORF up = { 0.0f, 1.0f, 0.0f };
	GW::MATH::GMatrix::TranslateGlobalF(initialCamera, translate, initialCamera);
	GW::MATH::GMatrix::LookAtLHF(translate, lookat, up, initialCamera);
	// Inverse to turn it into a camera matrix, not a view matrix. This will let us do
	// camera manipulation in the component easier
	GW::MATH::GMatrix::InverseF(initialCamera, initialCamera);
	registry.emplace<DRAW::Camera>(camera,
		DRAW::Camera{ initialCamera });
}

// This function will be called by the main loop to update the gameplay
// It will be responsible for updating the VulkanInstances and any other gameplay components
void GameplayBehavior(entt::registry& registry)
{
	std::shared_ptr<const GameConfig> config = registry.get<UTIL::Config>(
		registry.view<UTIL::Config>().front()).gameConfig;

	// Create the input objects
	auto input = registry.create();
	registry.emplace<UTIL::Input>(input);
	
	// Seed the rand
	unsigned int time = std::chrono::steady_clock::now().time_since_epoch().count();
	srand(time);
	entt::entity player = registry.create();
	entt::entity enemy = registry.create();
	auto& playerMeshCollection = registry.emplace<DRAW::MeshCollection>(player); 
	auto& playerTransform = registry.emplace<GAME::Transform>(player);
	auto& playerHealth = registry.emplace<GAME::Health>(player);
	playerHealth.health = (*config).at("Player").at("hitpoints").as<int>();
	auto& playerInvul = registry.emplace<GAME::invincibility>(player);
	playerInvul.cooldown = (*config).at("Player").at("invulnPeriod").as<float>();
	registry.emplace<GAME::Collidable>(player);
	registry.emplace<GAME::Player>(player); 
	

	auto& enemyMeshCollection = registry.emplace<DRAW::MeshCollection>(enemy); 
	auto& enemyTransform = registry.emplace<GAME::Transform>(enemy);
	auto& enemyVelocity = registry.emplace<GAME::Velocity>(enemy);
	auto& enemyShatter = registry.emplace<GAME::Shatters>(enemy);
	auto& enemyHealth = registry.emplace<GAME::Health>(enemy);  //sets enemy health using defaults file
	enemyHealth.health = (*config).at("Enemy1").at("hitpoints").as<int>();
	enemyShatter.initialShatterCount = (*config).at("Enemy1").at("initialShatterCount").as<int>();
	enemyShatter.scale = (*config).at("Enemy1").at("shatterScale").as<float>();
	registry.emplace<GAME::Collidable>(enemy);
	registry.emplace<GAME::Enemy>(enemy);
	
	//Sets a random speed for the enemy 
	std::random_device rd;
	std::mt19937 gen(rd());
	std::uniform_real_distribution<> dist(-2, 2);
	enemyVelocity.vec.x = dist(gen);
	std::random_device rd2;
	std::mt19937 gen2(rd2());
	std::uniform_real_distribution<> dist2(-2, 2);
	enemyVelocity.vec.z = dist2(gen2);
	//the enemy is meant to bounce around and cant have either speed be too slow to zero because it would just bounce straight up and down or left/right
	if (enemyVelocity.vec.x < 0.1f && enemyVelocity.vec.x > -0.1f) { enemyVelocity.vec.x += 0.5; }
	if (enemyVelocity.vec.z < 0.1f && enemyVelocity.vec.z > -0.1f) { enemyVelocity.vec.z += 0.5; }
		
	
	
	std::string playerName = (*config).at("Player").at("model").as<std::string>();
	std::string enemyName = (*config).at("Enemy1").at("model").as<std::string>();
	//pulling the only model manager, then grabbing the correct mesh collection using the names of player and enemy
	auto& MM = registry.get<DRAW::ModelManager>(registry.view<DRAW::ModelManager>().front());
	auto& playerMC = MM.mcMap[playerName];
	auto& enemyMC = MM.mcMap[enemyName];
	//looping through each entity in the player and enemy's mesh collection, then copying the data to the player entity.
	for (size_t i = 0; i < playerMC.entities.size(); i++)
	{
		playerMeshCollection.entities.resize(playerMC.entities.size());//resize the players MC entity vector
		auto mesh = registry.create();
		//still not sure where its pulling GPUInstance and Geometry from, might have to do some fixing later. could also use tryget
		auto originalTransform = registry.emplace<DRAW::GPUInstance>(mesh, registry.get<DRAW::GPUInstance>(playerMC.entities[i]));
		registry.emplace<DRAW::GeometryData>(mesh, registry.get<DRAW::GeometryData>(playerMC.entities[i]));
		//changes the players transform to the original's transform inside GpuInstance
		playerTransform.mat = originalTransform.transform;
		playerMeshCollection.collider = playerMC.collider;
		//registry.emplace<entt::entity>(playerMeshCollection.entities[i], mesh);
		playerMeshCollection.entities[i] = mesh; //adds this filled out entity to the player's mesh collection
		//might have to change this later! this is just a temporary measure to get the originals offscreen!
		auto& test = registry.get<DRAW::GPUInstance>(playerMC.entities[i]);
		test.transform.row4.x = 100000;
		
	}
	//MM.mcMap[playerName] = playerMeshCollection;
	for (size_t i = 0; i < enemyMC.entities.size(); i++)
	{
		enemyMeshCollection.entities.resize(enemyMC.entities.size()); //resize the enemies MC entity vector
		auto mesh = registry.create();
		auto originalTransform = registry.emplace<DRAW::GPUInstance>(mesh, registry.get<DRAW::GPUInstance>(enemyMC.entities[i]));
		registry.emplace<DRAW::GeometryData>(mesh, registry.get<DRAW::GeometryData>(enemyMC.entities[i]));
		enemyTransform.mat = originalTransform.transform;
		//might have to change this later! this is just a temporary measure to get the originals offscreen!
		auto& test = registry.get<DRAW::GPUInstance>(enemyMC.entities[i]);
		test.transform.row4.x = 10000;
		enemyMeshCollection.collider = enemyMC.collider;
		enemyMeshCollection.entities[i] = mesh;
	}
	//creates an entity with gamemanager so i can use it later
	auto gameManagerEntity = registry.create();
	registry.emplace<GAME::GameManager>(gameManagerEntity);
}

// This function will be called by the main loop to update the main loop
// It will be responsible for updating any created windows and handling any input
void MainLoopBehavior(entt::registry& registry)
{	
	// main loop
	int closedCount; // count of closed windows
	auto winView = registry.view<APP::Window>(); // for updating all windows
	auto& deltaTime = registry.emplace<UTIL::DeltaTime>(registry.create()).dtSec;
	// for updating all windows
	do {
		static auto start = std::chrono::steady_clock::now();
		double elapsed = std::chrono::duration<double>(
			std::chrono::steady_clock::now() - start).count();
		start = std::chrono::steady_clock::now();
		deltaTime = elapsed;

		// TODO : Update Game
		//auto gameManager = registry.get<GAME::GameManager>(registry.view<GAME::GameManager>().front());
		registry.patch<GAME::GameManager>(registry.view<GAME::GameManager>().front());
		
		closedCount = 0;
		// find all Windows that are not closed and call "patch" to update them
		for (auto entity : winView) {
			if (registry.any_of<APP::WindowClosed>(entity))
				++closedCount;
			else
				registry.patch<APP::Window>(entity); // calls on_update()
		}
	} while (winView.size() != closedCount); // exit when all windows are closed
}
