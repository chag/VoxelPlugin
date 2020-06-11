// Copyright 2020 Phyronnaz

#pragma once

#include "CoreMinimal.h"
#include "VoxelValue.h"
#include "VoxelMaterial.h"
#include "VoxelFoliage.h"
#include "VoxelSaveStruct.h"
#include "VoxelSave.generated.h"

DECLARE_VOXEL_MEMORY_STAT(TEXT("Voxel Uncompressed Saves Memory"), STAT_VoxelUncompressedSavesMemory, STATGROUP_VoxelMemory, VOXEL_API);
DECLARE_VOXEL_MEMORY_STAT(TEXT("Voxel Compressed Saves Memory"), STAT_VoxelCompressedSavesMemory, STATGROUP_VoxelMemory, VOXEL_API);

struct VOXEL_API FVoxelUncompressedWorldSaveImpl
{
public:
	FVoxelUncompressedWorldSaveImpl()
	{
	}
	~FVoxelUncompressedWorldSaveImpl()
	{
		DEC_VOXEL_MEMORY_STAT_BY(STAT_VoxelUncompressedSavesMemory, AllocatedSize);
	}

public:
	int32 GetDepth() const
	{
		return Depth;
	}
	FGuid GetGuid() const
	{
		return Guid;
	}

	bool HasValues() const
	{
		return ValueBuffers.Num() > 0;
	}
	bool HasMaterials() const
	{
		return MaterialBuffers.Num() > 0;
	}
	bool HasFoliage() const
	{
		return FoliageBuffers.Num() > 0;
	}

	/**
	 * Use in combination with SetUserFlags to do custom fixes (note that the plugin handles loading values/materials saved with different defines on its own)
	 * 
	 * For example:
	 * When loading:
	 * Save.ApplyCustomFixes([&](uint64 Flags, TArray<FVoxelValue>& Values, TArray<FVoxelMaterial>& Materials)
	 * {
	 *		if (Flags & EMyFlags::InvertR) // Or Flags < EMyVersions::InvertR
	 *		{
	 *			for (auto& Material : Materials)
	 *			{
	 *				Material.SetR(255 - Material.GetR());
	 *			}
	 *		}
	 * });
	 *
	 * When saving:
	 * Save.SetUserFlags(EMyFlags::Current);
	 */
	template<typename T>
	void ApplyCustomFixes(T Lambda)
	{
		Lambda(UserFlags, ValueBuffers, MaterialBuffers);
	}
	void SetUserFlags(uint64 InUserFlags)
	{
		UserFlags = InUserFlags;
	}
	uint64 GetUserFlags() const
	{
		return UserFlags;
	}

public:
	void UpdateAllocatedSize() const;
	bool Serialize(FArchive& Ar);
	TArray<uint8> GetSerializedData() const;

	bool operator==(const FVoxelUncompressedWorldSaveImpl& Other) const
	{
		return Guid == Other.Guid;
	}

private:
	struct FVoxelChunkSave
	{
		FIntVector Position;
		// GSingleValueIndexFlag bit is set if it's an index to the single value buffers
		int32 ValuesIndex = -1;
		int32 MaterialsIndex = -1;
		int32 FoliageIndex = -1;

		friend FArchive& operator<<(FArchive& Ar, FVoxelChunkSave& Save)
		{
			Ar << Save.Position;
			Ar << Save.ValuesIndex;
			Ar << Save.MaterialsIndex;
			Ar << Save.FoliageIndex;
			return Ar;
		}
	};
	
	int32 Version = -1;
	FGuid Guid;
	int32 Depth = -1;
	uint64 UserFlags = 0;
	
	TArray<FVoxelValue> ValueBuffers;
	TArray<FVoxelMaterial> MaterialBuffers;
	TArray<FVoxelFoliage> FoliageBuffers;
	
	TArray<FVoxelValue> SingleValues;
	TArray<FVoxelMaterial> SingleMaterials;
	TArray<FVoxelFoliage> SingleFoliage;
	
	TArray<FVoxelChunkSave> Chunks;
	TArray<uint8> PlaceableItems;

	mutable uint32 AllocatedSize = 0;

	friend class FVoxelSaveBuilder;
	friend class FVoxelSaveLoader;
	friend struct FVoxelChunkSaveWithoutFoliage;
};

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

struct VOXEL_API FVoxelCompressedWorldSaveImpl
{
	FVoxelCompressedWorldSaveImpl() = default;
	~FVoxelCompressedWorldSaveImpl();

	int32 GetDepth() const
	{
		return Depth;
	}

	bool operator==(const FVoxelCompressedWorldSaveImpl& Other) const
	{
		return Guid == Other.Guid;
	}

	bool Serialize(FArchive& Ar);
	void UpdateAllocatedSize() const;
	
private:
	int32 Version;
	FGuid Guid;
	int32 Depth = -1;
	TArray<uint8> CompressedData;

	mutable uint32 AllocatedSize = 0;

	friend class UVoxelSaveUtilities;
};

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

// Blueprint wrapper that's cheap to copy around
USTRUCT(BlueprintType, Category = Voxel)
struct VOXEL_API FVoxelUncompressedWorldSave
#if CPP
	: public TVoxelSaveStruct<FVoxelUncompressedWorldSaveImpl>
#endif
{
	GENERATED_BODY()
};

// Blueprint wrapper that's cheap to copy around
USTRUCT(BlueprintType, Category = Voxel)
struct VOXEL_API FVoxelCompressedWorldSave
#if CPP
	: public TVoxelSaveStruct<FVoxelCompressedWorldSaveImpl>
#endif
{
	GENERATED_BODY()
};

DEFINE_VOXEL_SAVE_STRUCT(FVoxelUncompressedWorldSave);
DEFINE_VOXEL_SAVE_STRUCT(FVoxelCompressedWorldSave);

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

UCLASS(Blueprintable, Category = Voxel)
class VOXEL_API UVoxelWorldSaveObject : public UObject
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Voxel")
	FVoxelCompressedWorldSave Save;

	// Depth of the world
	UPROPERTY(VisibleAnywhere, Category = "Voxel")
	int32 Depth = 0;

	virtual void PostLoad() override;

	void CopyDepthFromSave();
};