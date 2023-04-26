// Fill out your copyright notice in the Description page of Project Settings.


#include "PCGgamemode.h"
#include "Components/StaticMeshComponent.h"
#include "Math/UnrealMathUtility.h"
#include "Components/SceneComponent.h"
#include "UObject/Object.h"
#include "Kismet/KismetMathLibrary.h"


void APCGgamemode::BeginPlay()
{
	if (debug_log)
		GEngine->AddOnScreenDebugMessage(-1, 20.f, FColor::Red, TEXT("Generating map"));
	//Grid allocation
	GenerateGrid();
	GenerateCells();
	//Room alloction
	GenerateRoom();
	buildCaveRooms();
	clearUnseenTiles();
	VoidfillCaveRooms();
	//Connection allocation
	generateCavePath();
	//Build map
	BuildInUseArea();

	for (auto& cave : cave_rooms)
	{
		//GEngine->AddOnScreenDebugMessage(-1, 20.f, FColor::Red, FString::FromInt(cave.cave_room_id));
	}

	for (auto& room : connection_rooms)
	{
		for (auto& attachments : room.rooms_to_connect)
		{
			int test_int = attachments->cave_room_id;

			//GEngine->AddOnScreenDebugMessage(-1, 20.f, FColor::Red, FString::FromInt(test_int));
		}
	}

}

void APCGgamemode::GenerateGrid()
{
	for (int x = 0; x < max_grid_size; x++)
	{
		map_cells.Add(TArray<TArray<FTileData>>());

		for (int y = 0; y < max_grid_size; y++)
		{
			map_cells[x].Add(TArray<FTileData>());

			for (int z = 0; z < max_grid_size; z++)
			{
				map_cells[x][y].Add(FTileData());
			}
		}
	}

	for (int i = 0; i < cave_room_count; i++)
	{
		cave_rooms.Add(FCaveRoom());
	}
}

void APCGgamemode::GenerateCells()
{
	//constuctor for generating the grid and cells
	FVector spawn_location = { 0,0,0 };
	int currnet_id = 0;

	for(int z = 0; z < max_grid_size; z++)
	{
		spawn_location.X = 0;
		spawn_location.Y = 0;

		for(int y = 0; y < max_grid_size; y++)
		{
			spawn_location.X = 0;
			for(int x = 0; x < max_grid_size; x++)
			{
				//loops for each cell to generate base values/ generate in world

				//set the values of the cell
				map_cells[x][y][z].tile_id = currnet_id;
				map_cells[x][y][z].location = spawn_location;

				map_cells[x][y][z].tile_grid_location.X = spawn_location.X / 100;
				map_cells[x][y][z].tile_grid_location.Y = spawn_location.Y / 100;
				map_cells[x][y][z].tile_grid_location.Z = spawn_location.Z / 100;

				currnet_id += 1;
				spawn_location.X += cell_size; // go across to the x one cell
			}

			spawn_location.Y += cell_size;// go across to the z once
		}
		spawn_location.Z += cell_size; // move down a layer
	}
}



void APCGgamemode::GenerateRoom()
{
	FRandomStream Stream(seed);

	int cave_current_id = 1; // 0 = no cave

	for (auto& cave : cave_rooms)
	{
		//set section of cave type e.g. START, MIDDLE, END
		if (cave_current_id == 1)
		{
			cave.cave_type = "START";
		}
		else
		{
			cave.cave_type = "MIDDLE";
		}
			
		if (debug_log)
			GEngine->AddOnScreenDebugMessage(-1, 20.f, FColor::Red, TEXT("generating cave"));

		bool successfull_generation = false;

		for (int i = 0; i < max_attempts; i++)
		{
			//Get start location
			FVector spawn_location;
			successfull_generation = false;

			int min_spawn = 0 + (cave_base_size + 1);
			int max_spawn = max_grid_size-1 - min_spawn;

			spawn_location.X = Stream.FRandRange(min_spawn, max_spawn);
			spawn_location.Y = Stream.FRandRange(min_spawn, max_spawn);
			spawn_location.Z = Stream.FRandRange(min_spawn, max_spawn);

			int cave_size = cave_base_size;
			cave.cave_room_size = cave_size;

			if (checkIfSpawnBlocked(spawn_location.X, spawn_location.Y, spawn_location.Z, cave_size))
			{
				int cave_x = spawn_location.X;
				int cave_y = spawn_location.Y;
				int cave_z = spawn_location.Z;

				successfull_generation = true;

				cave.cave_room_id = cave_current_id;
				cave_current_id += 1;

				cave.generated = true;

				/*
				Loop though room spawn size equation = room size * complexity, Complexity can change depending on load configuration.

				Generation picks a point to start and goes half of cave size into the negatives and half to the posotives.
				*/

				for (int x = cave_x - (cave_size/2); x < cave_x + (cave_size / 2); x++)
				{
					for (int y = cave_y - (cave_size / 2); y < cave_y + (cave_size / 2); y++)
					{
						for (int z = cave_z - (cave_size / 2); z < cave_z + (cave_size / 2); z++)
						{
							//check if already part of cave

							cave.contained_cells.Add(&map_cells[x][y][z]);
							map_cells[x][y][z].in_use = true;
							map_cells[x][y][z].part_of_cave = cave.cave_room_id;

							//map_cells[x][y][z].tile_grid_location.X = x; map_cells[x][y][z].tile_grid_location.Y = y; map_cells[x][y][z].tile_grid_location.Z = z;

						} // z axis
					} // y axis
				} // x axis
			}
			else
			{
				//GEngine->AddOnScreenDebugMessage(-1, 20.f, FColor::Red, TEXT("spawn param out of bounds"));
			}


			if (successfull_generation)
			{
				if (debug_log)
					GEngine->AddOnScreenDebugMessage(-1, 20.f, FColor::Red, TEXT("successfull cave starting point found"));

				break;
			}
			
		}

		if (!successfull_generation)
		{
			if (debug_log)
				GEngine->AddOnScreenDebugMessage(-1, 20.f, FColor::Red, TEXT("unable to find start location"));
		}
	}

	//Grab last index and set type to END

	for (int i = cave_rooms.Num() - 1; i > 0 ; i--)
	{
		if (cave_rooms[i].generated)
		{
			cave_rooms[i].cave_type = "END";
			break;
		}
	}
}

bool APCGgamemode::checkIfSpawnBlocked(int spawn_x, int spawn_y, int spawn_z, int size)
{
	for (int x = spawn_x - (size / 2); x < spawn_x + (size / 2); x++)
	{
		for (int y = spawn_y - (size / 2); y < spawn_y + (size / 2); y++)
		{
			for (int z = spawn_x - (size / 2); z < spawn_z + (size / 2); z++)
			{
				//Check if in bounds
				if (x < max_grid_size-1 && x > 1 &&
					y < max_grid_size-1 && y > 1 &&
					z < max_grid_size-1 && z > 1)
				{
					if(map_cells[x][y][z].in_use)
						return false;
				}
				else
				{
					return false;
				}

			}
		}
	}

	return true;
}

void APCGgamemode::buildCaveRooms()
{
	/*
	Generate points randomly in each room, then use cellular automata to switch tiles from off and on, finally clear up rooms to be more coherant!
	*/

	FRandomStream Stream(seed);

	FVector start_location;
	// Inital setup, creates random holes in the 3D cube to be used with the algorithm later on
	for (auto& cave : cave_rooms)
	{
		if (cave.generated)
		{
			for (int x = cave.contained_cells[0]->tile_grid_location.X; x < cave.contained_cells[0]->tile_grid_location.X + cave.cave_room_size; x++)
			{
				for (int y = cave.contained_cells[0]->tile_grid_location.Y; y < cave.contained_cells[0]->tile_grid_location.Y + cave.cave_room_size; y++)
				{
					for (int z = cave.contained_cells[0]->tile_grid_location.Z; z < cave.contained_cells[0]->tile_grid_location.Z + cave.cave_room_size; z++)
					{
						//do the magic
						int probabillity = Stream.FRandRange(0, 100);

						if (x < max_grid_size && x >= 0 &&
							y < max_grid_size && y >= 0 &&
							z < max_grid_size && z >= 0)
						{
							if (automata_spawn_prob > probabillity)
							{
								map_cells[x][y][z].is_open = true;
							}

						}
					}
				}
			}


			//Setup complete, use algorithm to generate actual cave.

			int min_spawn = cave.contained_cells[0]->tile_grid_location.X; // start of cave
			int max_spawn = cave.contained_cells[0]->tile_grid_location.X + cave.cave_room_size; // max reach of cave

			for (int i = 0; i < automata_itteration_count; i++)
			{
				//clear single closed cells, normally floating in the middle of rooms
				for (int x = cave.contained_cells[0]->tile_grid_location.X; x < cave.contained_cells[0]->tile_grid_location.X + cave.cave_room_size; x++)
				{
					for (int y = cave.contained_cells[0]->tile_grid_location.Y; y < cave.contained_cells[0]->tile_grid_location.Y + cave.cave_room_size; y++)
					{
						for (int z = cave.contained_cells[0]->tile_grid_location.Z; z < cave.contained_cells[0]->tile_grid_location.Z + cave.cave_room_size; z++)
						{
							// cycle though an itteration count, a random cell is chosen and condition is changed depending on surronding
							int count = 0;

							if (x < max_grid_size - 1 && x >= 1 &&
								y < max_grid_size - 1 && y >= 1 &&
								z < max_grid_size - 1 && z >= 1)
							{
								count = getLiveCount(x, y, z);
								//max count is 26

								if (map_cells[x][y][z].is_open && count < 9)
								{
									map_cells[x][y][z].is_open = false;
								}
								if (!map_cells[x][y][z].is_open && count > 16)
								{
									map_cells[x][y][z].is_open = true;
								}
							}
						}
					}
				}
			}

			/*
			Creates a square of closes cells around the cave to ensure no gaps come from generation to close to the edge.
			*/
			int wall_mod = 8;

			for (int x = cave.contained_cells[0]->tile_grid_location.X; x < cave.contained_cells[0]->tile_grid_location.X + cave.cave_room_size; x++)
			{
				for (int y = cave.contained_cells[0]->tile_grid_location.Y; y < cave.contained_cells[0]->tile_grid_location.Y + cave.cave_room_size; y++)
				{
					for (int z = cave.contained_cells[0]->tile_grid_location.Z; z < cave.contained_cells[0]->tile_grid_location.Z + cave.cave_room_size; z++)
					{
						

						if ((x > cave.contained_cells[0]->tile_grid_location.X + (cave.cave_room_size/ wall_mod) && x < cave.contained_cells[0]->tile_grid_location.X + cave.cave_room_size - (cave.cave_room_size/ wall_mod)) != true)
						{
							map_cells[x][y][z].is_open = false;
						}
						if ((z > cave.contained_cells[0]->tile_grid_location.Z + (cave.cave_room_size / wall_mod) && z < cave.contained_cells[0]->tile_grid_location.Z + cave.cave_room_size - (cave.cave_room_size / wall_mod)) != true)
						{
							map_cells[x][y][z].is_open = false;
						}
						if ((y > cave.contained_cells[0]->tile_grid_location.Y + (cave.cave_room_size / wall_mod) && y < cave.contained_cells[0]->tile_grid_location.Y + cave.cave_room_size - (cave.cave_room_size / wall_mod)) != true)
						{
							map_cells[x][y][z].is_open = false;
						}


					}
				}
			}
			/*
			Clumps cells together based on their neighbours open/closed position
			*/
			for (int i = 0; i < cleanup_itteration_count; i++)
			{
				for (int x = cave.contained_cells[0]->tile_grid_location.X; x < cave.contained_cells[0]->tile_grid_location.X + cave.cave_room_size; x++)
				{
					for (int y = cave.contained_cells[0]->tile_grid_location.Y; y < cave.contained_cells[0]->tile_grid_location.Y + cave.cave_room_size; y++)
					{
						for (int z = cave.contained_cells[0]->tile_grid_location.Z; z < cave.contained_cells[0]->tile_grid_location.Z + cave.cave_room_size; z++)
						{
							int count = getLiveCount(x, y, z);

							if (x < max_grid_size - 1 && x >= 1 &&
								y < max_grid_size - 1 && y >= 1 &&
								z < max_grid_size - 1 && z >= 1)
							{
								if (count >= 13)
								{
									map_cells[x][y][z].is_open = true;
								}
								else
								{
									map_cells[x][y][z].is_open = false;
								}
							}
						}
					}
				}
			}
		}
	}
}

void APCGgamemode::clearUnseenTiles()
{
	if (debug_log)
		GEngine->AddOnScreenDebugMessage(-1, 20.f, FColor::Red, TEXT("removing hidden cells"));

	TArray<FTileData*> cells_to_open;

	for (int x = 1; x < max_grid_size-1; x++)
	{
		for (int y = 1; y < max_grid_size-1; y++)
		{
			for (int z = 1; z < max_grid_size-1; z++)
			{
				int count = 0;

				if (!map_cells[x][y + 1][z].is_open)
					count += 1;
				if (!map_cells[x][y - 1][z].is_open)
					count += 1;
				if (!map_cells[x + 1][y][z].is_open)
					count += 1;
				if (!map_cells[x - 1][y][z].is_open)
					count += 1;
				if (!map_cells[x][y][z + 1].is_open)
					count += 1;
				if (!map_cells[x][y][z - 1].is_open)
					count += 1;

				if (count == 6)
				{
					cells_to_open.Add(&map_cells[x][y][z]);
				}
			}
		}
	}
	if (debug_log)
		GEngine->AddOnScreenDebugMessage(-1, 20.f, FColor::Red, FString::FromInt(cells_to_open.Num()));

	for (auto& tile : cells_to_open)
	{
		tile->is_open = true;
	}
}

int APCGgamemode::getLiveCount(int x, int y, int z)
{
	int count = 0;
	//check neighbouring cells to get live count

	//check top layer
	if (map_cells[x + 1][y + 1][z + 1].is_open)
		count += 1;
	if (map_cells[x + 1][y + 1][z].is_open)
		count += 1;
	if (map_cells[x + 1][y + 1][z - 1].is_open)
		count += 1;
	if (map_cells[x][y + 1][z + 1].is_open)
		count += 1;
	if (map_cells[x][y + 1][z].is_open)
		count += 1;
	if (map_cells[x][y + 1][z -1].is_open)
		count += 1;
	if (map_cells[x -1][y + 1][z + 1].is_open)
		count += 1;
	if (map_cells[x - 1][y + 1][z].is_open)
		count += 1;
	if (map_cells[x - 1][y + 1][z - 1].is_open)
		count += 1;

	//middle layer
	if (map_cells[x + 1][y][z + 1].is_open)
		count += 1;
	if (map_cells[x + 1][y][z].is_open)
		count += 1;
	if (map_cells[x + 1][y][z - 1].is_open)
		count += 1;
	if (map_cells[x][y][z + 1].is_open)
		count += 1;
	if (map_cells[x][y][z - 1].is_open)
		count += 1;
	if (map_cells[x - 1][y][z + 1].is_open)
		count += 1;
	if (map_cells[x - 1][y][z].is_open)
		count += 1;
	if (map_cells[x - 1][y][z - 1].is_open)
		count += 1;

	//bottom layer
	if (map_cells[x + 1][y - 1][z + 1].is_open)
		count += 1;
	if (map_cells[x + 1][y - 1][z].is_open)
		count += 1;
	if (map_cells[x + 1][y - 1][z - 1].is_open)
		count += 1;
	if (map_cells[x][y - 1][z + 1].is_open)
		count += 1;
	if (map_cells[x][y - 1][z].is_open)
		count += 1;
	if (map_cells[x][y - 1][z - 1].is_open)
		count += 1;
	if (map_cells[x - 1][y - 1][z + 1].is_open)
		count += 1;
	if (map_cells[x - 1][y - 1][z].is_open)
		count += 1;
	if (map_cells[x - 1][y - 1][z - 1].is_open)
		count += 1;

	return count;
}

void APCGgamemode::generateTileActor(FTileData* cell_pointer, AActor* actor_to_add)
{
	FActorSpawnParameters SpawnParams;

	if (!IsValid(cell_pointer->tile_actor))
	{

		if (generate_mesh_per_cell)
		{
			cell_pointer->tile_actor = GetWorld()->SpawnActor<AActor>(debug_mesh, cell_pointer->location, FRotator(0, 0, 0), SpawnParams);
		}
		else
		{
			if (cubeMesh != NULL)
			{
				if(debug_log)
					GEngine->AddOnScreenDebugMessage(-1, 20.f, FColor::Red, TEXT("generating cell"));

				FName mesh_name = FName("cell" + FString::FromInt(cell_pointer->tile_id));

				UStaticMeshComponent* mesh_comp = NewObject<UStaticMeshComponent>(actor_to_add, UStaticMeshComponent::StaticClass(), mesh_name);

				if (mesh_comp)
				{
					mesh_comp->RegisterComponent();
					mesh_comp->CreationMethod = EComponentCreationMethod::Instance;

					mesh_comp->SetStaticMesh(cubeMesh);
					mesh_comp->SetWorldLocation(cell_pointer->location);
				}
			}
		}

	}
}

void APCGgamemode::BuildInUseArea()
{
	FActorSpawnParameters SpawnParams;

	for (auto& cave : cave_rooms)
	{
		if (cave.actor == NULL)
		{
			cave.actor = GetWorld()->SpawnActor<AActor>(SpawnParams);
		}

		for (auto& cell : cave.contained_cells)
		{
			if (cell->is_open != true)
			{
				generateTileActor(cell, cave.actor);
			}
		}
	}

	for (auto& corridor : connection_rooms)
	{
		if (corridor.actor == NULL)
		{
			corridor.actor = GetWorld()->SpawnActor<AActor>(SpawnParams);
		}

		for (auto& cell : corridor.contained_cells)
		{
			if (cell->is_open != true)
			{
				generateTileActor(cell, corridor.actor);
			}
		}

	}
}

void APCGgamemode::generateConnectionsArray()
{
	for (int i = 0; i < cave_room_count; i++)
	{
		connection_rooms.Add(FConectionRoom());
	}
}

void APCGgamemode::generateCavePath()
{
	/*
	Function to generate the connections between caves
	In order: checks distance between connecting caves (id:1 to id:2).
	checks obstruction, is so create diversion around.
	create solid line of open cells (the path) then generate noise around path with a bias the further out
	cleanup connections*/

	generateConnectionsArray();

	int connection_id = 1; // 0 means no connection
	bool conc = false;

	for (int i = 0; i < cave_rooms.Num()-1; i++)
	{
		conc = false;
		if (cave_rooms[i].generated)
		{
			if (cave_rooms[i].cave_type == "START" || cave_rooms[i].cave_type == "MIDDLE")
			{
				//section is start link to first id thats useable

				for (int x = 0; x < cave_rooms.Num(); x++)
				{

					if (cave_rooms[x].generated && cave_rooms[x].cave_room_id > cave_rooms[i].cave_room_id)
					{

						for (auto& connection : connection_rooms)
						{
							if (!connection.connected && !conc)
							{
								if(debug_log)
									GEngine->AddOnScreenDebugMessage(-1, 20.f, FColor::Red, TEXT("generating connections"));
								
								connection.cave_room_id = connection_id;
								connection_id += 1;
								connection.connected = true;

								connection.rooms_to_connect.Add(&cave_rooms[i]);
								connection.rooms_to_connect.Add(&cave_rooms[x]);

								cave_rooms[x].connections_attached.Add(&connection);
								cave_rooms[i].connections_attached.Add(&connection);

								conc = true;
							}
						}
					}
				}
			}
		}

		generateNoiseToConnection();

	}
}

void APCGgamemode::VoidfillCaveRooms()
{
	//void fill empty space in the cave room to remove closed empty cells

	for (auto& cave : cave_rooms)
	{
		if(cave.contained_cells.Num() != 0)
		{ 
			//get first cell in x, y, z = 0, 0, 0 | reset the list of checking cells
			int cave_current_id = cave.cave_room_id;

			TArray<FTileData*> tiles_to_check;

			if (cave.contained_cells[0]->is_open == true)
			{
				tiles_to_check.Add(cave.contained_cells[0]);
				if (debug_log)
					GEngine->AddOnScreenDebugMessage(-1, 20.f, FColor::Red, TEXT("Found starting spot!"));
			}

			while (tiles_to_check.Num() > 0)
			{
				//apply flood fill algorithm
				FVector current_grid_tile = tiles_to_check[0]->tile_grid_location;

				if (map_cells[current_grid_tile.X + 1][current_grid_tile.Y][current_grid_tile.Z].is_open == true && map_cells[current_grid_tile.X][current_grid_tile.Y][current_grid_tile.Z].part_of_cave == cave_current_id)
				{
					tiles_to_check.Add(&map_cells[current_grid_tile.X + 1][current_grid_tile.Y][current_grid_tile.Z]);
				}
				if (map_cells[current_grid_tile.X - 1][current_grid_tile.Y][current_grid_tile.Z].is_open == true && map_cells[current_grid_tile.X][current_grid_tile.Y][current_grid_tile.Z].part_of_cave == cave_current_id)
				{
					tiles_to_check.Add(&map_cells[current_grid_tile.X - 1][current_grid_tile.Y][current_grid_tile.Z]);
				}
				if (map_cells[current_grid_tile.X][current_grid_tile.Y + 1][current_grid_tile.Z].is_open == true && map_cells[current_grid_tile.X][current_grid_tile.Y][current_grid_tile.Z].part_of_cave == cave_current_id)
				{
					tiles_to_check.Add(&map_cells[current_grid_tile.X][current_grid_tile.Y + 1][current_grid_tile.Z]);
				}
				if (map_cells[current_grid_tile.X][current_grid_tile.Y - 1][current_grid_tile.Z].is_open == true && map_cells[current_grid_tile.X][current_grid_tile.Y][current_grid_tile.Z].part_of_cave == cave_current_id)
				{
					tiles_to_check.Add(&map_cells[current_grid_tile.X][current_grid_tile.Y - 1][current_grid_tile.Z]);
				}
				if (map_cells[current_grid_tile.X][current_grid_tile.Y][current_grid_tile.Z + 1].is_open == true && map_cells[current_grid_tile.X][current_grid_tile.Y][current_grid_tile.Z].part_of_cave == cave_current_id)
				{
					tiles_to_check.Add(&map_cells[current_grid_tile.X][current_grid_tile.Y][current_grid_tile.Z + 1]);
				}
				if (map_cells[current_grid_tile.X][current_grid_tile.Y][current_grid_tile.Z - 1].is_open == true && map_cells[current_grid_tile.X][current_grid_tile.Y][current_grid_tile.Z].part_of_cave == cave_current_id)
				{
					tiles_to_check.Add(&map_cells[current_grid_tile.X][current_grid_tile.Y][current_grid_tile.Z - 1]);
				}

				tiles_to_check[0]->part_of_cave = 0;
				tiles_to_check[0]->in_use = false;
				tiles_to_check.RemoveAt(0);

		
			}
		}
		
	}
}

void APCGgamemode::generateNoiseToConnection()
{
	FRandomStream Stream(seed);

	for (auto& connection : connection_rooms)
	{
		//create noise for each connection

		//first get start location for the noise to generate from (the edge of the cell)
		if (!connection.generated && connection.connected)
		{
			connection.generated = true;

			float temp_x = connection.rooms_to_connect[0]->contained_cells[0]->tile_grid_location.X;
			float temp_y = connection.rooms_to_connect[0]->contained_cells[0]->tile_grid_location.Y;
			float temp_z = connection.rooms_to_connect[0]->contained_cells[0]->tile_grid_location.Z;

			int cave_1_spawn_x = temp_x + (connection.rooms_to_connect[0]->cave_room_size / 2);
			int cave_1_spawn_y = temp_y + (connection.rooms_to_connect[0]->cave_room_size / 2);
			int cave_1_spawn_z = temp_z + (connection.rooms_to_connect[0]->cave_room_size / 2);

			temp_x = connection.rooms_to_connect[1]->contained_cells[0]->tile_grid_location.X;
			temp_y = connection.rooms_to_connect[1]->contained_cells[0]->tile_grid_location.Y;
			temp_z = connection.rooms_to_connect[1]->contained_cells[0]->tile_grid_location.Z;

			int cave_2_spawn_x = temp_x + (connection.rooms_to_connect[1]->cave_room_size / 2);
			int cave_2_spawn_y = temp_y + (connection.rooms_to_connect[1]->cave_room_size / 2);
			int cave_2_spawn_z = temp_z + (connection.rooms_to_connect[1]->cave_room_size / 2);

			FVector spawn_location_1 = map_cells[cave_1_spawn_x][cave_1_spawn_y][cave_1_spawn_z].location;
			FVector spawn_location_2 = map_cells[cave_2_spawn_x][cave_2_spawn_y][cave_2_spawn_z].location;

			FVector current_point = spawn_location_1;
			FVector direction = spawn_location_2 - spawn_location_1; direction.Normalize(); direction = direction * 10;

			///*
			//Get cell position of where to spawn connection 1 start point and start point 2
			//*/

			/*
			Locates the nearst spot outside the caves cell to start a connection point

			uses 2 random spots and moves towards the 2nd spot to find the nearest outside spot to start generation

			Generates a path to follow along before generating the tunnel
			*/
			current_point.X = ((current_point.X + cell_size) / cell_size) * cell_size;
			current_point.Y = ((current_point.Y + cell_size) / cell_size) * cell_size;
			current_point.Z = ((current_point.Z + cell_size) / cell_size) * cell_size;
#
			connection.connection_location_array.Add(spawn_location_1);

			current_point = spawn_location_1;

			/*
			check y value of both points A and B, if difference is greater than allowance create midway point in grid to connect to, rinse repeat till under allowance
			*/

			bool has_wind = false;

			if (abs(spawn_location_1.Z / cell_size - spawn_location_2.Z / cell_size) > corridor_heigh_allowance)
			{
				FVector temp;
				//find and add spawn point
				if (spawn_location_1.Z - spawn_location_2.Z > 0)
				{
					//up
					temp = FVector(Stream.FRandRange(tunnel_width * cell_size, (max_grid_size * cell_size) - (tunnel_width * cell_size)),
						Stream.FRandRange(tunnel_width * cell_size, (max_grid_size * cell_size) - (tunnel_width * cell_size)),
						spawn_location_1.Z - (corridor_heigh_allowance * cell_size));
				}
				else
				{
					//down
					temp = FVector(Stream.FRandRange(tunnel_width * cell_size, (max_grid_size * cell_size) - (tunnel_width * cell_size)),
						Stream.FRandRange(tunnel_width * cell_size, (max_grid_size * cell_size) - (tunnel_width * cell_size)),
						spawn_location_1.Z + (corridor_heigh_allowance * cell_size));
				}
				has_wind = true;
				connection.connection_location_array.Add(temp);
			}
			else
			{
				//no winding path neeeded add connection directly

				connection.connection_location_array.Add(spawn_location_2);
			}


			direction = connection.connection_location_array[1] - connection.connection_location_array[0]; direction.Normalize(); direction = direction * 10;
			int current_array_point = 1;
			current_point = spawn_location_1;

			while (connection.connection_location_array.Num() != current_array_point)
			{

				if (current_point.X / cell_size >= 0 && current_point.X / cell_size < max_grid_size - 1 &&
					current_point.Y / cell_size >= 0 && current_point.Y / cell_size < max_grid_size - 1 &&
					current_point.Z / cell_size >= 0 && current_point.Z / cell_size < max_grid_size - 1)
				{
					current_point = current_point + direction;

					if (map_cells[current_point.X / cell_size][current_point.Y / 100][current_point.Z / cell_size].is_path == false)
					{
						connection.path.Add(&map_cells[current_point.X / cell_size][current_point.Y / cell_size][current_point.Z / cell_size]);
						map_cells[current_point.X / cell_size][current_point.Y / cell_size][current_point.Z / cell_size].is_path = true;
					}

					if (abs(FVector::Dist(FVector(current_point.X, current_point.Y, current_point.Z), connection.connection_location_array[current_array_point])) < 200)
					{
						if (current_array_point == connection.connection_location_array.Num() - 1 && has_wind == false)
						{
							break;
						}
						else
						{
							//move to next point

							FVector temp_current = current_point;

							current_point = spawn_location_2;

							connection.connection_location_array.Add(current_point);

							current_point = temp_current;
							current_array_point += 1;
							direction = connection.connection_location_array[current_array_point] - connection.connection_location_array[current_array_point - 1]; direction.Normalize(); direction = direction * 10;

							GEngine->AddOnScreenDebugMessage(-1, 20.f, FColor::Red, TEXT("reached point, moving to next"));
							has_wind = false;
						}
					}

				}
				else
				{
					break;
				}
			}

			/*
			Generates the corridor using the path array
			*/
			for (int cell = 0; cell < connection.path.Num() - 1; cell++)
			{

				for (int x = connection.path[cell]->tile_grid_location.X - (tunnel_width / 2); x < connection.path[cell]->tile_grid_location.X + (tunnel_width / 2); x++)
				{
					for (int y = connection.path[cell]->tile_grid_location.Y - (tunnel_width / 2); y < connection.path[cell]->tile_grid_location.Y + (tunnel_width / 2); y++)
					{
						for (int z = connection.path[cell]->tile_grid_location.Z - (tunnel_width / 2); z < connection.path[cell]->tile_grid_location.Z + (tunnel_width / 2); z++)
						{

							if (x >= 0 && x < max_grid_size &&
								y >= 0 && y < max_grid_size &&
								z >= 0 && z < max_grid_size &&
								(!map_cells[x][y][z].in_use ||
									connection.rooms_to_connect[0]->cave_room_id == map_cells[x][y][z].part_of_cave ||
									connection.rooms_to_connect[1]->cave_room_id == map_cells[x][y][z].part_of_cave))
							{
								//Build corridor pathing line in order to construct the corridor
								if (map_cells[x][y][z].part_of_cave == 0)
								{
									connection.contained_cells.Add(&map_cells[x][y][z]);
									map_cells[x][y][z].is_open = false;
									map_cells[x][y][z].in_use = true;
									map_cells[x][y][z].part_of_cave = 100 + connection.cave_room_id;
								}

							}

						}
					}
				}
			}


			for (auto cell : connection.path)
			{

				for (int x = cell->tile_grid_location.X - (tunnel_width / 4); x < cell->tile_grid_location.X + (tunnel_width / 4); x++)
				{
					for (int y = cell->tile_grid_location.Y - (tunnel_width / 4); y < cell->tile_grid_location.Y + (tunnel_width / 4); y++)
					{
						for (int z = cell->tile_grid_location.Z - (tunnel_width / 4); z < cell->tile_grid_location.Z + (tunnel_width / 4); z++)
						{

							if (x >= 0 && x < max_grid_size &&
								y >= 0 && y < max_grid_size &&
								z >= 0 && z < max_grid_size)
							{
								map_cells[x][y][z].is_open = true;
							}

						}
					}
				}
			}
			
			for (auto cell : connection.contained_cells)
			{
				for (int i = 0; i < cleanup_itteration_count; i++)
				{
					if (cell->tile_grid_location.X < max_grid_size - 1 && cell->tile_grid_location.X >= 1 &&
						cell->tile_grid_location.Y < max_grid_size - 1 && cell->tile_grid_location.Y >= 1 &&
						cell->tile_grid_location.Z < max_grid_size - 1 && cell->tile_grid_location.Z >= 1)
					{
						int count = getLiveCount(cell->tile_grid_location.X, cell->tile_grid_location.Y, cell->tile_grid_location.Z);

						if (count >= 18)
						{
							cell->is_open = true;
						}
						else
						{
							cell->is_open = false;
						}
					}
				}
			}
			


		}
	}
}