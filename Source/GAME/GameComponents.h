#ifndef GAME_COMPONENTS_H_
#define GAME_COMPONENTS_H_

namespace GAME
{
	/// TODO: Create the tags and components for the game
	/// 
	///*** Tags ***///
	

	///*** Components ***///
	
	struct Player
	{
		// A tag put onto the player entity to identify them
	};
	struct Enemy
	{
		// A tag put onto the enemy entity to identify them
	};
	struct Bullet
	{
		// A tag put onto the bullet entity to identify them
	};
	struct Transform
	{
		GW::MATH::GMATRIXF mat;
	};
	struct GameManager
	{
		//needs to be empty, manages the game.
	};
	struct Firing
	{
		//A tag to determine if the player is firing
		float Cooldown = 4.0;
	};
	struct invincibility
	{
		float cooldown = 4.0;
	};
	struct Velocity 
	{
		GW::MATH::GVECTORF vec{ 1,0,1 }; //determines speed and direction of a model
		float speed = 4;
	};
	struct Collidable 
	{

	};
	struct Obstacle
	{
		//A tag for walls
	};
	struct Health
	{
		//Determines the enemies health
		int health;
	};
	struct Shatters
	{
		int initialShatterCount;
		float scale;
	};
	struct GAMEOVER
	{

	};
}// namespace GAME
#endif // !GAME_COMPONENTS_H_