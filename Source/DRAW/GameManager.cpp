#include "..\CCL.h"
#include "..\GAME\GameComponents.h"
#include "DrawComponents.h"
#include <random>


namespace GAME
{
	using namespace GW::MATH;
	//current issues = bullets are not getting destroyed properly, thus they lose velocity when another bullet is spawned. 
	void UpdateGameManager(entt::registry& registry, entt::entity entity) {

		auto view = registry.view<GameManager>();
		for (auto entity : view) {
			if (registry.any_of<GAMEOVER>(entity)) {

			}
			else {


				static auto start = std::chrono::steady_clock::now();
				double elapsed = std::chrono::duration<double>(
					std::chrono::steady_clock::now() - start).count();
				start = std::chrono::steady_clock::now();

				//collects all the entities that have both a GPUInstance and a Transform component
				auto& view = registry.view<DRAW::MeshCollection, Transform>(entt::exclude<Obstacle>);
				//for each entity in the view, copy the transform from Transform into the GPUInstance's transform.
				for (auto entity : view) {

					auto& mc = registry.get<DRAW::MeshCollection>(entity);
					for (size_t i = 0; i < mc.entities.size(); i++)
					{
						auto& gpuInstance = registry.get<DRAW::GPUInstance>(mc.entities[i]);
						auto& transform = registry.get<Transform>(entity);
						gpuInstance.transform = transform.mat;
					}


					if (registry.all_of<Player>(entity)) {
						//checking if the player is there
					}
					else { //if the entity doesnt contain the player, get velocity and use it to move the entity. 
						auto& velocity = registry.get<Velocity>(entity); //gets the velocity component from the current dynamic entity
						const float trueSpeed = velocity.speed * elapsed;
						GMatrix::TranslateLocalF(registry.get<Transform>(entity).mat,
							GVECTORF{ static_cast<float>(trueSpeed * velocity.vec.x),
							static_cast<float>(trueSpeed * velocity.vec.y), static_cast<float>(trueSpeed * velocity.vec.z) }, registry.get<Transform>(entity).mat);
					}

				}
				registry.patch<GAME::Player>(registry.view<GAME::Player>().front());

				auto& collisions = registry.view<Transform, DRAW::MeshCollection, Collidable>();
				for (auto i = collisions.begin(); i != collisions.end(); i++) {
					auto& transformA = registry.get<Transform>(*i).mat;
					auto colliderA = registry.get<DRAW::MeshCollection>(*i).collider;

					//Transform into world space of object 
					GMatrix::VectorXMatrixF(transformA, colliderA.center, colliderA.center);
					//Scale of extents
					GVECTORF scaleVecA;
					GMatrix::GetScaleF(transformA, scaleVecA);
					colliderA.extent.x *= scaleVecA.x;
					colliderA.extent.y *= scaleVecA.y;
					colliderA.extent.z *= scaleVecA.z;

					//apply rotation
					GQUATERNIONF qA;
					GQuaternion::SetByMatrixF(transformA, qA);
					GQuaternion::MultiplyQuaternionF(colliderA.rotation, qA, colliderA.rotation);

					auto j = i;
					for (j++; j != collisions.end(); j++)
					{
						auto& transformB = registry.get<Transform>(*j).mat;
						auto colliderB = registry.get<DRAW::MeshCollection>(*j).collider;

						//Transform into world space of object 
						GMatrix::VectorXMatrixF(transformB, colliderB.center, colliderB.center);
						//Scale of extents
						GVECTORF scaleVecB;
						GMatrix::GetScaleF(transformB, scaleVecB);
						colliderB.extent.x *= scaleVecB.x;
						colliderB.extent.y *= scaleVecB.y;
						colliderB.extent.z *= scaleVecB.z;

						//apply rotation
						GQUATERNIONF qB;
						GQuaternion::SetByMatrixF(transformB, qB);
						GQuaternion::MultiplyQuaternionF(colliderB.rotation, qB, colliderB.rotation);

						GW::MATH::GCollision::GCollisionCheck result;
						GW::MATH::GCollision::TestOBBToOBBF(colliderA, colliderB, result);
						if (result == GCollision::GCollisionCheck::COLLISION) {
							// if bullets collide with walls
							if (registry.all_of<Obstacle>(*i) && registry.all_of<Bullet>(*j)) {
								registry.destroy(*j);
								//std::cout << " ---------------------------COLLISION----------------------" << std::endl;
							}
							if (registry.all_of<Obstacle>(*j) && registry.all_of<Bullet>(*i)) {
								registry.destroy(*i);
								//std::cout << " ---------------------------COLLISION----------------------" << std::endl;
							}
							//if enemies collide with walls
							if (registry.all_of<Enemy>(*i) && registry.all_of<Obstacle>(*j)) {
								auto& enemyVelocity = registry.get<Velocity>(*i);
								GVECTORF enemyPos = transformA.row4;
								GVECTORF point;
								GCollision::ClosestPointToOBBF(colliderB, enemyPos, point);
								GVECTORF reflection;
								GVector::SubtractVectorF(enemyPos, point, reflection);
								point.y = 0.0f;
								reflection.w = 0.0f;
								GVector::NormalizeF(reflection, reflection);
								GVECTORF newVel;
								float dot;
								GVector::DotF(enemyVelocity.vec, reflection, dot);
								dot = 2.0f * (dot);
								GVector::ScaleF(reflection, dot, reflection);
								GVector::SubtractVectorF(enemyVelocity.vec, reflection, newVel);
								enemyVelocity.vec = newVel;


							}
							if (registry.all_of<Enemy>(*j) && registry.all_of<Obstacle>(*i)) {
								auto& enemyVelocity = registry.get<Velocity>(*i);
								GVECTORF enemyPos = transformB.row4;
								GVECTORF point;
								GCollision::ClosestPointToOBBF(colliderA, enemyPos, point);
								GVECTORF reflection;
								GVector::SubtractVectorF(enemyPos, point, reflection);
								point.y = 0.0f;
								reflection.w = 0.0f;
								GVector::NormalizeF(reflection, reflection);
								GVECTORF newVel;
								float dot;
								GVector::DotF(enemyVelocity.vec, reflection, dot);
								dot = 2.0f * (dot);
								GVector::ScaleF(reflection, dot, reflection);
								GVector::SubtractVectorF(enemyVelocity.vec, reflection, newVel);
								enemyVelocity.vec = newVel;

							}
							//if bullets hit enemy
							if (registry.all_of<Enemy>(*i) && registry.all_of<Bullet>(*j)) {
								auto& eHealth = registry.get<Health>(*i);
								registry.destroy(*j);
								eHealth.health--;
								if (eHealth.health <= 0) {
									std::cout << "-----------------ENEMY DEAD --------------" << std::endl;
								}
							}
							if (registry.all_of<Enemy>(*j) && registry.all_of<Bullet>(*i)) {
								auto& eHealth = registry.get<Health>(*j);
								registry.destroy(*i);
								eHealth.health--;
								if (eHealth.health <= 0) {
									std::cout << "-----------------ENEMY DEAD --------------" << std::endl;
								}
							}
							if (registry.all_of<Enemy>(*i) && registry.all_of<Player>(*j)) {

								if (registry.all_of<invincibility>(*j)) {
									//std::cout << "blocked" <<std::endl;
								}
								else {
									auto& pHealth = registry.get<Health>(*j);
									pHealth.health--;
									registry.emplace<invincibility>(*j);
									if (pHealth.health >= 0) {
										std::cout << "HEALTH =   " << pHealth.health << std::endl;
									}
								}
							}
							if (registry.all_of<Enemy>(*j) && registry.all_of<Player>(*i)) {

								if (registry.all_of<invincibility>(*i)) {
									//std::cout << "blocked" << std::endl;
								}
								else {
									auto& pHealth = registry.get<Health>(*i);
									pHealth.health--;
									registry.emplace<invincibility>(*i);
									if (pHealth.health >= 0) {
										std::cout << "HEALTH =   " << pHealth.health << std::endl;
									}
									
									

								}
							}
							//collision has happened!

						}
					}
				}
				auto& enemies = registry.view<Enemy>();
				for (auto i = enemies.begin(); i != enemies.end(); i++)
				{
					auto& eHealth = registry.get<Health>(*i);

					if (eHealth.health <= 0)
					{
						if (registry.all_of<Shatters>(*i)) {
							if (registry.get<Shatters>(*i).initialShatterCount > 1) {
								for (size_t j = 0; j < 2; j++)
								{
									auto& shatter = registry.get<Shatters>(*i);
									auto enemy = registry.create();
									auto& enemyMC = registry.emplace<DRAW::MeshCollection>(enemy);
									auto& enemyTransform = registry.emplace<Transform>(enemy, registry.get<Transform>(*i));
									auto& enemyShatter = registry.emplace<GAME::Shatters>(enemy, shatter);
									enemyShatter.initialShatterCount--;
									auto& oldMC = registry.get<DRAW::MeshCollection>(*i);
									enemyMC.collider = oldMC.collider;
									auto mesh = registry.create();
									registry.emplace<DRAW::GPUInstance>(mesh, registry.get<DRAW::GPUInstance>(oldMC.entities[0]));
									registry.emplace<DRAW::GeometryData>(mesh, registry.get<DRAW::GeometryData>(oldMC.entities[0]));
									enemyMC.entities.push_back(mesh);

									auto& enemyHealth = registry.emplace<GAME::Health>(enemy);
									auto& enemyVelocity = registry.emplace<Velocity>(enemy, registry.get<Velocity>(*i));
									registry.emplace<GAME::Collidable>(enemy);
									registry.emplace<GAME::Enemy>(enemy);
									GVECTORF scaleVecB;
									GMatrix::GetScaleF(enemyTransform.mat, scaleVecB);
									scaleVecB.x *= shatter.scale;
									scaleVecB.y *= shatter.scale;
									scaleVecB.z *= shatter.scale;
									GMatrix::ScaleGlobalF(enemyTransform.mat, scaleVecB, enemyTransform.mat);
									enemyHealth.health = 3; //only hard coding this because the originals health is now zero and i cant copy it. 

									std::random_device rd;
									std::mt19937 gen(rd());
									std::uniform_real_distribution<> dist(-3, 3);
									enemyVelocity.vec.x = dist(gen);
									std::random_device rd2;
									std::mt19937 gen2(rd2());
									std::uniform_real_distribution<> dist2(-3, 3);
									enemyVelocity.vec.z = dist2(gen2);
									enemyVelocity.speed++;
								}
							}



						}




						registry.destroy(*i);
					}
				}
				// game over logic
				auto& gameManager = registry.view<GameManager>();
				auto& playerList = registry.view<Player>();
				auto& enemyList = registry.view<Enemy>();
				for (auto entity : gameManager)
				{
					for (auto pList : playerList) { //if player health is zero, game over
						auto& pHealth = registry.get<Health>(pList);
						if (pHealth.health < 0) {
							std::cout << "--------------------------------GAME OVER, YOU LOST---------------------------------" << std::endl;
							registry.emplace_or_replace<GAMEOVER>(entity);
						}
					}
					
					if (enemyList.empty()) {
						registry.emplace_or_replace<GAMEOVER>(entity);
						std::cout << "------------------------------------YOU WON! CONGRATZ!-----------------------------------" << std::endl;
					}
				}
				
				





			}
		}
		


		

	}
	

	CONNECT_COMPONENT_LOGIC() {
		// register the Window component's logic
		registry.on_update<GameManager>().connect<UpdateGameManager>();
		
		//when you emplace something to be a cpu level or gpu level, these functions get called 

	}
}