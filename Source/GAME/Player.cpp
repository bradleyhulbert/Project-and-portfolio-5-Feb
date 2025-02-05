#include "..\CCL.h"
#include "GameComponents.h"
#include "..\DRAW\DrawComponents.h"
#include "..\UTIL\Utilities.h"


namespace GAME
{

	void UpdatePlayer(entt::registry& registry, entt::entity entity)
	{   //grabbing default.ini data 
		std::shared_ptr<const GameConfig> config = registry.get<UTIL::Config>(
			registry.view<UTIL::Config>().front()).gameConfig;
		
		auto dTime = registry.get<UTIL::DeltaTime>(registry.view<UTIL::DeltaTime>().front());
		auto& input = registry.get<UTIL::Input>(registry.view<UTIL::Input>().front());
		//getting the default speed of the player from defaults.ini
		std::string playerSpeed = (*config).at("Player").at("speed").as<std::string>();
		int trueSpeed = std::stoi(playerSpeed);
		//grabs the only entity with transform and player components 
		auto& group = registry.group<Transform>(entt::get<Player>);
		auto& view = registry.view<Player, GAME::invincibility>();
		for (auto entity : view) {
			auto& ivcb = registry.get<GAME::invincibility>(entity);
			ivcb.cooldown = ivcb.cooldown - 0.001;
			if (ivcb.cooldown < 0) {
				registry.remove<GAME::invincibility>(entity);
			}
		}
		//just gonna fuckin use some code from the camera back in 3DCC. still not sure how to modify based on speed but fuckin whatever
		//this will change later for the bullet code, but fuck it.
		static auto start = std::chrono::steady_clock::now();
		double elapsed = std::chrono::duration<double>(
			std::chrono::steady_clock::now() - start).count();
		start = std::chrono::steady_clock::now();
		
		float states[8] = { 0, 0, 0, 0};
		input.immediateInput.GetState(G_KEY_W, states[0] = 0);
		input.immediateInput.GetState(G_KEY_S, states[1] = 0);
		input.immediateInput.GetState(G_KEY_D, states[2] = 0);
		input.immediateInput.GetState(G_KEY_A, states[3] = 0);
		const float pfs = trueSpeed * elapsed;
		double Z_Change = states[0] - states[1];
		double X_Change = states[2] - states[3];
		for (auto entity : group){
            auto& playerLocation = group.get<Transform>(entity);
			GW::MATH::GMatrix::TranslateLocalF(playerLocation.mat,
				GW::MATH::GVECTORF{ static_cast<float>(X_Change * pfs),
				0, static_cast<float>(Z_Change * pfs) }, playerLocation.mat);
			input.immediateInput.GetState(G_KEY_UP, states[4] = 0);
			input.immediateInput.GetState(G_KEY_DOWN, states[5] = 0);
			input.immediateInput.GetState(G_KEY_RIGHT, states[6] = 0);
			input.immediateInput.GetState(G_KEY_LEFT, states[7] = 0);
			// if the arrow keys are being pressed
			if (states[4] == 1 || states[5] == 1 || states[6] == 1 || states[7] == 1) {
				
				if (registry.all_of<Firing>(entity)) { // if firing exists
					auto& cooldown = registry.get<Firing>(entity);
					cooldown.Cooldown -= .005; //reduce cooldown
					//std::cout << " COOLDOWN " << std::endl;
					if (registry.get<Firing>(entity).Cooldown < 0) { //if cooldown is below zero, remove it.
						registry.remove<Firing>(entity);
						//std::cout << " RELOAD " << std::endl;
					}
					
				}//MIGHT HAVE TO PLACE THIS OUTSIDE THE BUTTON PRESS IF CHECK AND BEFORE IT. 
				else { // if firing is gone, that means the cooldown is over and we can fire. 
					//std::cout << " BULLET " << std::endl;
					auto bulletEntity = registry.create(); 
					auto& bulletMeshCollection = registry.emplace<DRAW::MeshCollection>(bulletEntity);
					auto& bulletTransform = registry.emplace<Transform>(bulletEntity);
					auto& bulletVelocity = registry.emplace<Velocity>(bulletEntity);
					registry.emplace<Collidable>(bulletEntity);
					registry.emplace<Bullet>(bulletEntity);

					float UpDown = states[4] - states[5];
					float leftRight = states[6] - states[7];
					if (states[4] == 1 && states[5] == 1) {
						UpDown = 1;
					}
					if (states[6] == 1 && states[7] == 1) {
						leftRight = 0;
						if (states[5] == 1 && states[4] == 0) {
							UpDown = -1;
						}
						else {
							UpDown = 1;
						}
					}
					bulletVelocity.vec.z = UpDown * bulletVelocity.speed;
					bulletVelocity.vec.x = leftRight * bulletVelocity.speed;
					
					
					auto& MM = registry.get<DRAW::ModelManager>(registry.view<DRAW::ModelManager>().front());
					std::string	bulletName = (*config).at("Bullet").at("model").as<std::string>();
					auto bulletMC = MM.mcMap[bulletName];

					for (size_t i = 0; i < bulletMC.entities.size(); i++)
					{
						auto& test = registry.get<DRAW::GPUInstance>(bulletMC.entities[i]);
						test.transform.row4.x = 9999;
						bulletMeshCollection.entities.resize(bulletMC.entities.size());
						auto mesh = registry.create();
						auto& bTrans = registry.emplace<DRAW::GPUInstance>(mesh, registry.get<DRAW::GPUInstance>(bulletMC.entities[i]));
						registry.emplace<DRAW::GeometryData>(mesh, registry.get<DRAW::GeometryData>(bulletMC.entities[i]));
						bulletTransform.mat = playerLocation.mat;
						bulletMeshCollection.collider = bulletMC.collider;
						bulletMeshCollection.entities[i] = mesh;
						

						
					}
					registry.emplace<Firing>(entity);
				}
			}
		
		}
		
		//if (states[0] == 1) { std::cout << " W "; }if (states[1] == 1) { std::cout << " S "; }if (states[2] == 1) { std::cout << " D "; }if (states[3] == 1) { std::cout << " A "; }
		
		
		
		
		
	}

	


	CONNECT_COMPONENT_LOGIC() {
		registry.on_update<Player>().connect<UpdatePlayer>();

	}
}