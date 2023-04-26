// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "Containers/Array.h"
#include "PCGgamemode.generated.h"


/**
 * 
 */
UCLASS()
class PCGPROJECT_API APCGgamemode : public AGameModeBase
{
	GENERATED_BODY()
	
public:

	struct FTileData
	{
		//Individual cell data
	public:
		UPROPERTY(VisibleAnywhere, Category = "Tile Data")
			AActor* tile_actor;
		UPROPERTY(VisibleAnywhere, Category = "Tile Data")
			FVector location;
		UPROPERTY(VisibleAnywhere, Category = "Tile Data")
			bool has_enemy;
		UPROPERTY(VisibleAnywhere, Category = "Tile Data")
			bool tile_spawned = false;
		UPROPERTY(VisibleAnywhere, Category = "Tile Data")
			bool starting_tile = false;
		UPROPERTY(VisibleAnywhere, Category = "Tile Data")
			float tile_size;
		UPROPERTY(VisibleAnywhere, Category = "Tile Data")
			float tile_id;
		UPROPERTY(VisibleAnywhere, Category = "Tile Data")
			int part_of_cave = 0;
		UPROPERTY(VisibleAnywhere, Category = "Tile Data")
			bool in_use = false;
		UPROPERTY(VisibleAnywhere, Category = "Tile Data")
			bool is_open = false;
		UPROPERTY(VisibleAnywhere, Category = "Tile Data")
			FVector tile_grid_location;
		UPROPERTY(VisibleAnywhere, Category = "Tile Data")
			bool is_path = false;
	};

	struct FConectionRoom;

	struct FCaveRoom
	{
		//struct for pointers to contained cells and data for generation
	public:

		UPROPERTY(VisibleAnywhere, Category = "Generation information")
			TArray<FTileData*> contained_cells;
		UPROPERTY(VisibleAnywhere, Category = "Generation information")
			int cave_room_id;
		UPROPERTY(VisibleAnywhere, Category = "Generation information")
			FString cave_type;
		UPROPERTY(VisibleAnywhere, Category = "Generation information")
			int cave_room_size;
		UPROPERTY(VisibleAnywhere, Category = "Generation information")
			int start_location[3];
		UPROPERTY(VisibleAnywhere, Category = "Generation information")
			bool generated = false;
		UPROPERTY(VisibleAnywhere, Category = "Generation information")
			TArray<FConectionRoom*> connections_attached;
		UPROPERTY(VisibleAnywhere, Category = "Generation actor")
			AActor* actor;
	};

	struct FConectionRoom
	{
		//struct for pointers to contained cells and data for generation
	public:

		UPROPERTY(VisibleAnywhere, Category = "Generation information")
			TArray<FTileData*> contained_cells;
		UPROPERTY(VisibleAnywhere, Category = "Generation information")
			TArray<FTileData*> path;
		UPROPERTY(VisibleAnywhere, Category = "Generation information")
			int cave_room_id;
		UPROPERTY(VisibleAnywhere, Category = "Generation information")
			FString cave_type;
		UPROPERTY(VisibleAnywhere, Category = "Generation information")
			int tunne_size;
		UPROPERTY(VisibleAnywhere, Category = "Generation information")
			bool generated = false;
		UPROPERTY(VisibleAnywhere, Category = "Generation information")
			bool connected = false;
		UPROPERTY(VisibleAnywhere, Category = "Generation information")
			TArray<FCaveRoom*> rooms_to_connect;
		UPROPERTY(VisibleAnywhere, Category = "Generation information")
			TArray<FVector> connection_location_array;
		UPROPERTY(VisibleAnywhere, Category = "Generation actor")
			AActor* actor;

	};

	//General map properites
	UPROPERTY(EditAnywhere, Category = "Generation settings")
		int seed = 0;
	UPROPERTY(EditAnywhere, Category = "Automata settings")
		int automata_spawn_prob = 50;
	UPROPERTY(EditAnywhere, Category = "Automata settings")
		int automata_itteration_count = 3;
	UPROPERTY(EditAnywhere, Category = "Automata settings")
		int cleanup_itteration_count = 2;

	UPROPERTY(EditAnywhere, Category = "Generation settings")
		int max_grid_size = 25;

	UPROPERTY(EditAnywhere, Category = "Generation settings")
		float cell_size = 50;

	UPROPERTY(EditAnywhere, Category = "Generation settings")
		float cave_base_size = 4;

	UPROPERTY(EditAnywhere, Category = "Generation settings")
		float tunnel_width = 8;

	UPROPERTY(EditAnywhere, Category = "Generation settings")
		int cave_room_count;

	UPROPERTY(EditAnywhere, Category = "Generation settings")
		int max_attempts = 5;

	UPROPERTY(EditAnywhere, Category = "Generation settings")
		int corridor_heigh_allowance;

	//debug properties
	UPROPERTY(EditAnywhere, Category = "debug settings")
		bool generate_mesh_per_cell;

	UPROPERTY(EditAnywhere, Category = "debug settings")
		bool debug_log;

	UPROPERTY(EditAnywhere, Category = "debug settings")
		TSubclassOf<AActor> debug_mesh;

	UPROPERTY(EditAnywhere, Category = "debug settings")
		UStaticMesh* cubeMesh;


protected:
	virtual void BeginPlay() override;

private:
	TArray<TArray<TArray<FTileData>>> map_cells; // 3D array to hold all the map cells in, to be used with pointers to allocate rooms
	TArray<FCaveRoom> cave_rooms;
	TArray<FConectionRoom> connection_rooms;


	//Grid allocation
	void GenerateGrid();
	void GenerateCells();

	//Cave allocation/generation
	void GenerateRoom();
	void buildCaveRooms();

	//connection generation/allocation functions
	void generateCavePath();
	void generateConnectionsArray();
	void generateNoiseToConnection();
	void VoidfillCaveRooms();

	//Build map
	void generateTileActor(FTileData* tile_to_generate, AActor* actor_to_add);
	void clearUnseenTiles();
	void BuildInUseArea();
	void cleanupCells();

	//other
	int getLiveCount(int x, int y, int z);
	bool checkIfSpawnBlocked(int spawn_x, int spawn_y, int spawn_z, int size);





public:
};
