// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#include "BlastMeshUtilities.h"

#include "Modules/ModuleManager.h"
#include "RawIndexBuffer.h"
#include "Materials/Material.h"
#include "SkeletalMeshTypes.h"
#include "StaticMeshResources.h"
#include "Engine/SkinnedAssetCommon.h"
#include "Engine/StaticMesh.h"
#include "Animation/Skeleton.h"
#include "MeshUtilities.h"
#include "MeshDescription.h"
#include "Components/SkinnedMeshComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "PhysicsEngine/PhysicsAsset.h"
#include "Runtime/Core/Public/Misc/FeedbackContext.h"
#include "RawMesh.h"
#include "Rendering/SkeletalMeshLODModel.h"
#include "Rendering/SkeletalMeshModel.h"
#include "OverlappingCorners.h"
#include "StaticMeshAttributes.h"
#include "StaticMeshOperations.h"
// UE 5.1 compatibility: StaticToSkeletalMeshConverter was added in later UE5 versions
// #include "StaticToSkeletalMeshConverter.h"
#include "Interfaces/ITargetPlatform.h"
#include "Interfaces/ITargetPlatformManagerModule.h"
#include "Misc/CoreMisc.h"
#include "IMeshBuilderModule.h"
#include "ImportUtils/SkeletalMeshImportUtils.h"
#include "Rendering/SkeletalMeshRenderData.h"

#include "BlastFracture.h"
#include "BlastMesh.h"
#include "BlastGlobals.h"

#include "NvBlastExtAuthoringTypes.h"
#include "NvBlastExtAuthoringMesh.h"
#include "blast-sdk/extensions/authoring/NvBlastExtAuthoring.h"
#include "blast-sdk/extensions/authoring/NvBlastExtAuthoringFractureTool.h"

#define LOCTEXT_NAMESPACE "BlastMeshEditor"

Nv::Blast::Mesh* CreateAuthoringMeshFromRawMesh(const FRawMesh& RawMesh, const FTransform3f& UE4ToBlastTransform)
{
	//Raw meshes are unwelded by default so weld them together and generate a real index buffer
	TArray<FStaticMeshBuildVertex> WeldedVerts;
	TArray<TArray<uint32>> PerSectionIndices;
	TArray<int32> WedgeMap;

	//Map them all to section 0
	PerSectionIndices.SetNum(1);
	TMap<uint32, uint32> MaterialToSectionMapping;
	for (int32 Face = 0; Face < RawMesh.FaceMaterialIndices.Num(); Face++)
	{
		MaterialToSectionMapping.Add(RawMesh.FaceMaterialIndices[Face], 0);
	}

	FOverlappingCorners OverlappingCorners;
	IMeshUtilities& MeshUtilities = FModuleManager::Get().LoadModuleChecked<IMeshUtilities>("MeshUtilities");
	MeshUtilities.FindOverlappingCorners(OverlappingCorners, RawMesh.VertexPositions, RawMesh.WedgeIndices,
	                                     THRESH_POINTS_ARE_SAME);
	MeshUtilities.BuildStaticMeshVertexAndIndexBuffers(WeldedVerts, PerSectionIndices, WedgeMap, RawMesh,
	                                                   OverlappingCorners, MaterialToSectionMapping,
	                                                   THRESH_POINTS_ARE_SAME, FVector3f(1.0f),
	                                                   EImportStaticMeshVersion::LastVersion);

	const TArray<int32>* FaceMaterialIndices = &RawMesh.FaceMaterialIndices;
	const TArray<uint32>* FaceSmoothingMasks = &RawMesh.FaceSmoothingMasks;

	TArray<int32> FilteredFaceMaterialIndices;
	TArray<uint32> FilteredFaceSmoothingMasks;

	//If the size doesn't match it removed some degenerate  triangles so we need to update our arrays
	if (PerSectionIndices[0].Num() != (FaceMaterialIndices->Num() * 3))
	{
		check((FaceMaterialIndices->Num() * 3) == WedgeMap.Num());
		FilteredFaceMaterialIndices.Reserve(FaceMaterialIndices->Num());
		FilteredFaceSmoothingMasks.Reserve(FaceSmoothingMasks->Num());

		for (int32 FaceIdx = 0; FaceIdx < FaceMaterialIndices->Num(); FaceIdx++)
		{
			int32 WedgeNewIdxs[3] = {
				WedgeMap[FaceIdx * 3 + 0],
				WedgeMap[FaceIdx * 3 + 1],
				WedgeMap[FaceIdx * 3 + 2]
			};

			if (WedgeNewIdxs[0] != INDEX_NONE && WedgeNewIdxs[1] != INDEX_NONE && WedgeNewIdxs[2] != INDEX_NONE)
			{
				//we kept this face, make sure the ordering matches
				//checkSlow(WedgeNewIdxs[0] == PerSectionIndices[0][FaceIdx * 3 + 0]
				//	&& WedgeNewIdxs[1] == PerSectionIndices[0][FaceIdx * 3 + 1]
				//	&& WedgeNewIdxs[2] == PerSectionIndices[0][FaceIdx * 3 + 2]);
				FilteredFaceMaterialIndices.Add((*FaceMaterialIndices)[FaceIdx]);
				if (FaceSmoothingMasks->IsValidIndex(FaceIdx))
				{
					FilteredFaceSmoothingMasks.Add((*FaceSmoothingMasks)[FaceIdx]);
				}
			}
			else
			{
				//This should be impossible but if the entire triangle was not removed
				checkSlow(WedgeNewIdxs[0] == INDEX_NONE
					&& WedgeNewIdxs[1] == INDEX_NONE
					&& WedgeNewIdxs[2] == INDEX_NONE);
			}
		}

		FaceMaterialIndices = &FilteredFaceMaterialIndices;
		FaceSmoothingMasks = &FilteredFaceSmoothingMasks;
	}

	TArray<FVector3f> Positions;
	TArray<FVector3f> Normals;
	TArray<FVector2f> UVs;

	Positions.SetNumUninitialized(WeldedVerts.Num());
	Normals.SetNumUninitialized(WeldedVerts.Num());
	UVs.SetNumUninitialized(WeldedVerts.Num());

	//FMeshMergeHelpers::TransformRawMeshVertexData flips it if the determinant is < 0 which we don't wnat
	for (int32 V = 0; V < WeldedVerts.Num(); V++)
	{
		const FStaticMeshBuildVertex& SMBV = WeldedVerts[V];
		Positions[V] = UE4ToBlastTransform.TransformPosition(SMBV.Position);
		Normals[V] = UE4ToBlastTransform.TransformVector(SMBV.TangentZ);
		UVs[V] = FVector2f(SMBV.UVs[0].X, 1.0f - SMBV.UVs[0].Y);
	}

	Nv::Blast::Mesh* mesh = NvBlastExtAuthoringCreateMesh((NvcVec3*)Positions.GetData(), (NvcVec3*)Normals.GetData(),
	                                                      (NvcVec2*)UVs.GetData(), WeldedVerts.Num(),
	                                                      PerSectionIndices[0].GetData(), PerSectionIndices[0].Num());
	mesh->setMaterialId(FaceMaterialIndices->GetData());
	if (FaceMaterialIndices->Num() == FaceSmoothingMasks->Num())
	{
		mesh->setSmoothingGroup((int32*)FaceSmoothingMasks->GetData());
	}

	return mesh;
}

Nv::Blast::Mesh* CreateAuthoringMeshFromMeshDescription(const FMeshDescription& SourceMeshDescription,
                                                        const TMap<FName, int32>& MaterialMap,
                                                        const FTransform3f& UE4ToBlastTransform)
{
	TArray<Nv::Blast::Vertex> Verts;
	TArray<Nv::Blast::Edge> Edges;
	TArray<Nv::Blast::Facet> Facets;

	//Gather all array data
	FStaticMeshConstAttributes Attributes(SourceMeshDescription);
	TVertexAttributesConstRef<FVector3f> VertexPositions = Attributes.GetVertexPositions();
	TVertexInstanceAttributesConstRef<FVector3f> VertexInstanceNormals = Attributes.GetVertexInstanceNormals();
	TVertexInstanceAttributesConstRef<FVector2f> VertexInstanceUVs = Attributes.GetVertexInstanceUVs();
	TPolygonGroupAttributesConstRef<FName> PolygonGroupMaterialSlotName = Attributes.GetPolygonGroupMaterialSlotNames();

	Verts.AddZeroed(SourceMeshDescription.Vertices().Num());
	TArray<int32> RemapVerts;
	RemapVerts.AddZeroed(SourceMeshDescription.Vertices().GetArraySize());
	int32 VertexIndex = 0;
	for (const FVertexID VertexID : SourceMeshDescription.Vertices().GetElementIDs())
	{
		Verts[VertexIndex].p = ToNvVector(UE4ToBlastTransform.TransformPosition(VertexPositions[VertexID]));
		const int32 NumChannels = FMath::Min(1, VertexInstanceUVs.GetNumChannels());
		for (int32 UVChannel = 0; UVChannel < NumChannels; UVChannel++)
		{
			Verts[VertexIndex].uv[UVChannel] = ToNvVector(VertexInstanceUVs.Get(VertexID, UVChannel));
			Verts[VertexIndex].uv[UVChannel].y = 1.f - Verts[VertexIndex].uv[UVChannel].y; // blast thing
		}
		Verts[VertexIndex].p = ToNvVector(VertexPositions[VertexID]);
		Verts[VertexIndex].n = ToNvVector(UE4ToBlastTransform.TransformVector(VertexInstanceNormals[VertexID]));
		RemapVerts[VertexID.GetValue()] = VertexIndex;
		VertexIndex++;
	}

	const int32 TriangleNumber = SourceMeshDescription.Triangles().Num();
	Facets.AddZeroed(TriangleNumber);
	Edges.Reserve(Facets.Num() * 3);

	TArray<uint32> FaceSmoothingMasks;
	FaceSmoothingMasks.AddZeroed(TriangleNumber);
	//Convert the smoothgroup
	FStaticMeshOperations::ConvertHardEdgesToSmoothGroup(SourceMeshDescription, FaceSmoothingMasks);

	int32 TriangleIdx = 0;
	for (const FTriangleID TriID : SourceMeshDescription.Triangles().GetElementIDs())
	{
		Facets[TriangleIdx].firstEdgeNumber = Edges.Num();
		Facets[TriangleIdx].smoothingGroup = FaceSmoothingMasks[TriangleIdx];

		const FPolygonGroupID PolygonGroupID = SourceMeshDescription.GetTrianglePolygonGroup(TriID);
		const int32* MaterialForFace = MaterialMap.Find(PolygonGroupMaterialSlotName[PolygonGroupID]);
		Facets[TriangleIdx].materialId = MaterialForFace ? *MaterialForFace : PolygonGroupID.GetValue();

		const auto EdgesOfTriangle = SourceMeshDescription.GetTriangleEdges(TriID);
		Facets[TriangleIdx].edgesCount = EdgesOfTriangle.Num();

		auto VertsOfTriangleConst = SourceMeshDescription.GetTriangleVertices(TriID);
		check(VertsOfTriangleConst.Num() == 3);
		
		// Make a mutable copy for potential swapping
		TArray<FVertexID> VertsOfTriangle;
		VertsOfTriangle.Add(VertsOfTriangleConst[0]);
		VertsOfTriangle.Add(VertsOfTriangleConst[1]);
		VertsOfTriangle.Add(VertsOfTriangleConst[2]);

		const FVector3f DeducedNormal = FVector3f::CrossProduct(
			VertexPositions[VertsOfTriangle[1]] - VertexPositions[VertsOfTriangle[0]],
			VertexPositions[VertsOfTriangle[2]] - VertexPositions[VertsOfTriangle[0]]);

		const FVector3f AverageNormal =
			VertexInstanceNormals[VertsOfTriangle[0]] + VertexInstanceNormals[VertsOfTriangle[1]] +
			VertexInstanceNormals[VertsOfTriangle[2]];
		if (FVector3f::DotProduct(AverageNormal, DeducedNormal) < 0.f)
		{
			Swap(VertsOfTriangle[0], VertsOfTriangle[1]);
		}

		Edges.Add({
			(uint32)RemapVerts[VertsOfTriangle[0]],
			(uint32)RemapVerts[VertsOfTriangle[1]]
		});
		Edges.Add({
			(uint32)RemapVerts[VertsOfTriangle[1]],
			(uint32)RemapVerts[VertsOfTriangle[2]]
		});
		Edges.Add({
			(uint32)RemapVerts[VertsOfTriangle[2]],
			(uint32)RemapVerts[VertsOfTriangle[0]]
		});

		TriangleIdx++;
	}

	Nv::Blast::Mesh* mesh = NvBlastExtAuthoringCreateMeshFromFacets(Verts.GetData(), Edges.GetData(),
	                                                                Facets.GetData(), Verts.Num(),
	                                                                Edges.Num(), Facets.Num());

	/*Nv::Blast::MeshCleaner* cleaner = NvBlastExtAuthoringCreateMeshCleaner();
	Nv::Blast::Mesh* cleanedMesh = cleaner->cleanMesh(mesh);
	cleaner->release();
	if (cleanedMesh)
	{
		mesh->release();
		return cleanedMesh;
	}*/

	return mesh;
}

Nv::Blast::Mesh* CreateAuthoringMeshFromRenderData(const FStaticMeshRenderData& RenderData,
                                                   const TMap<FName, int32>& MaterialMap,
                                                   const FTransform3f& UE4ToBlastTransform)
{
	if (RenderData.LODResources.IsEmpty())
	{
		return nullptr;
	}

	TArray<Nv::Blast::Vertex> Verts;
	Verts.AddUninitialized(RenderData.LODResources[0].GetNumVertices());

	for (int32 VertIdx = 0; VertIdx < Verts.Num(); VertIdx++)
	{
		Verts[VertIdx].p = ToNvVector(UE4ToBlastTransform.TransformPosition(
			RenderData.LODResources[0].VertexBuffers.PositionVertexBuffer.VertexPosition(VertIdx)));
		Verts[VertIdx].n = ToNvVector(UE4ToBlastTransform.TransformVectorNoScale(
			RenderData.LODResources[0].VertexBuffers.StaticMeshVertexBuffer.VertexTangentZ(VertIdx)));
		Verts[VertIdx].uv[0] = ToNvVector(
			RenderData.LODResources[0].VertexBuffers.StaticMeshVertexBuffer.GetVertexUV(VertIdx, 0));
		Verts[VertIdx].uv[0].y = 1.f - Verts[VertIdx].uv[0].y; // blast thing
	}

	TArray<uint32> Indices;
	RenderData.LODResources[0].IndexBuffer.GetCopy(Indices);

	Nv::Blast::Mesh* mesh = NvBlastExtAuthoringCreateMeshOnlyTriangles(Verts.GetData(), Verts.Num(),
	                                                                   Indices.GetData(), Indices.Num(),
	                                                                   nullptr, 0);

	/*Nv::Blast::MeshCleaner* cleaner = NvBlastExtAuthoringCreateMeshCleaner();
	Nv::Blast::Mesh* cleanedMesh = cleaner->cleanMesh(mesh);
	cleaner->release();
	if (cleanedMesh)
	{
		mesh->release();
		return cleanedMesh;
	}*/

	return mesh;
}

float SignedVolumeOfTriangle(const Nv::Blast::Triangle& t) 
{
	const FVector3f& p1 = reinterpret_cast<const FVector3f&>(t.a.p);
	const FVector3f& p2 = reinterpret_cast<const FVector3f&>(t.b.p);
	const FVector3f& p3 = reinterpret_cast<const FVector3f&>(t.c.p);
	return FVector3f::DotProduct(p1, FVector3f::CrossProduct(p2, p3)) / 6.0f;
}

TArray<SkeletalMeshImportData::FBone> GetBinaryBonesFromFractureData(const FTransform& RootTransform, TSharedPtr<FFractureSession> FractureSession)
{
	TArray<SkeletalMeshImportData::FBone> Bones;
	Bones.AddDefaulted(FractureSession->ChunkToBoneIndex.Num());
	for (auto& Bone : Bones)
	{
		Bone.Flags = 0;
		Bone.NumChildren = 0;
	}
	
	Bones[0].Name = TEXT("root");
	Bones[0].ParentIndex = INDEX_NONE;
	Bones[0].BonePos.Transform = FTransform3f(RootTransform);
	
	for (const auto& [ChunkIdx, BoneIdx] : FractureSession->ChunkToBoneIndex)
	{
		if (ChunkIdx < 0)
		{
			continue;
		}

		const int32 chunkID = FractureSession->FractureData->assetToFractureChunkIdMap[ChunkIdx];
		const int32 chunkInfoIdx = FractureSession->FractureTool->getChunkInfoIndex(chunkID);

		const Nv::Blast::ChunkInfo& ChunkInfo = FractureSession->FractureTool->getChunkInfo(chunkInfoIdx);
		int32 ParentBoneIdx = 0;
		for (uint32 Idx = 0; Idx < FractureSession->FractureData->chunkCount; Idx++)
		{
			if (ChunkInfo.parentChunkId == FractureSession->FractureData->assetToFractureChunkIdMap[Idx])
			{
				ParentBoneIdx = FractureSession->ChunkToBoneIndex[Idx];
				break;
			}
		}

		Bones[BoneIdx].Name = UBlastMesh::GetDefaultChunkBoneNameFromIndex(ChunkIdx).ToString();
		Bones[BoneIdx].BonePos.Transform = FTransform3f::Identity;
		Bones[BoneIdx].ParentIndex = ParentBoneIdx;
		Bones[ParentBoneIdx].NumChildren++;
	}

	return Bones;
}

FSkeletalMeshImportData CreateSkeletalMeshImportDataFromFractureData(TSharedPtr<FFractureSession> FractureSession,
                    const TArray<FSkeletalMaterial>& ExistingMaterials,
                    int32 ChunkIndex = INDEX_NONE)
{
	FSkeletalMeshImportData OutData;
	TMap<int32, int32> InteriorMaterialsToSlots;
	USkeletalMesh* SkeletalMesh = FractureSession->BlastMesh->Mesh;
	TSharedPtr<Nv::Blast::AuthoringResult> FractureData = FractureSession->FractureData;
	check(ChunkIndex < (int32)FractureData->chunkCount);
	FTransform3f Converter = UBlastMesh::GetTransformBlastToUECoordinateSystem();

	TArray<FSkeletalMaterial>& NewMaterials = SkeletalMesh->GetMaterials();

	int32 FirstChunk = ChunkIndex < 0 ? 0 : ChunkIndex;
	int32 LastChunk = ChunkIndex < 0 ? FractureData->chunkCount : ChunkIndex + 1;
	if (FractureSession->BlastMesh->ChunkMeshVolumes.Num() < LastChunk)
	{
		FractureSession->BlastMesh->ChunkMeshVolumes.AddZeroed(LastChunk - FractureSession->BlastMesh->ChunkMeshVolumes.Num());
	}

	uint32 TriangleCount = FractureData->geometryOffset[LastChunk] - FractureData->geometryOffset[FirstChunk];
	OutData.Points.AddUninitialized(TriangleCount * 3);
	OutData.Wedges.AddUninitialized(TriangleCount * 3);
	OutData.Faces.SetNum(TriangleCount);
	OutData.Influences.AddUninitialized(TriangleCount * 3);
	OutData.PointToRawMap.AddUninitialized(TriangleCount * 3);
	OutData.RefBonesBinary.AddDefaulted(LastChunk - FirstChunk);
	OutData.NumTexCoords = 1;
	OutData.bHasTangents = false;
	OutData.bHasNormals = true;
	OutData.bHasVertexColors = false;
	uint32 VertexIndex = 0;
	int32 FaceIndex = 0;
	
	for (int32 ci = FirstChunk; ci < LastChunk; ci++)
	{
		float MeshVolume = 0.f;
		for (uint32 fi = FractureData->geometryOffset[ci]; fi < FractureData->geometryOffset[ci + 1]; fi++, FaceIndex++)
		{
			Nv::Blast::Triangle& tr = FractureData->geometry[fi];
			MeshVolume += SignedVolumeOfTriangle(tr);
			//No need to pass normals, it is computed in mesh builder anyway
			for (uint32 vi = 0; vi < 3; vi++, VertexIndex++)
			{
				const Nv::Blast::Vertex& v = (&tr.a)[vi];
				OutData.Points[VertexIndex] = Converter.TransformPosition(FVector3f(v.p.x, v.p.y, v.p.z));
				OutData.PointToRawMap[VertexIndex] = VertexIndex;
				OutData.Wedges[VertexIndex].Color = FColor::White;
				for (uint32 uvi = 0; uvi < OutData.NumTexCoords; uvi++)
				{
					OutData.Wedges[VertexIndex].UVs[uvi] = FVector2f(0.f, 0.f);
					if (uvi == 0)
					{
						OutData.Wedges[VertexIndex].UVs[uvi] = FVector2f(v.uv[uvi].x, -v.uv[uvi].y + 1.f);
					}
				}
				OutData.Wedges[VertexIndex].VertexIndex = VertexIndex;
				OutData.Faces[FaceIndex].WedgeIndex[vi] = VertexIndex;
				OutData.Faces[FaceIndex].TangentZ[vi] = Converter.TransformVector(FVector3f(v.n.x, v.n.y, v.n.z));
				OutData.Influences[VertexIndex].BoneIndex = FractureSession->ChunkToBoneIndex[ci];
				OutData.Influences[VertexIndex].VertexIndex = VertexIndex;
				OutData.Influences[VertexIndex].Weight = 1.f;
			}

			//the interior material ids don't follow after the previously existing materials, there could be a gap, while UE4 needs them tightly packed
			int32 FinalMatSlot = tr.materialId;

			if (!ExistingMaterials.IsValidIndex(tr.materialId))
			{
				const int32* MatSlot = InteriorMaterialsToSlots.Find(tr.materialId);
				if (MatSlot)
				{
					FinalMatSlot = *MatSlot;
				}
				else
				{
					// Try find material by name.
					FName matname = FName(FBlastFracture::InteriorMaterialID, tr.materialId);
					int32 rslot = -1;
					for (int32 mid = 0; mid < ExistingMaterials.Num(); ++mid)
					{
						if (ExistingMaterials[mid].ImportedMaterialSlotName == matname)
						{
							rslot = mid;
							break;
						}
					}


					if (rslot == -1)
					{
						FinalMatSlot = NewMaterials.Num();
						InteriorMaterialsToSlots.Add(tr.materialId, FinalMatSlot);
						//Update the internal representation with what our final materialID is
						FractureSession->FractureTool->replaceMaterialId(tr.materialId, FinalMatSlot);
						FSkeletalMaterial NewMat(UMaterial::GetDefaultMaterial(MD_Surface));
						NewMat.ImportedMaterialSlotName = matname;
						NewMaterials.Add(NewMat);
					}
					else
					{
						FinalMatSlot = rslot;
						InteriorMaterialsToSlots.Add(tr.materialId, FinalMatSlot);
						FractureSession->FractureTool->replaceMaterialId(tr.materialId, FinalMatSlot);
					}
				}
			}
			OutData.Faces[FaceIndex].MatIndex = FinalMatSlot;
			tr.materialId = FinalMatSlot;
			//tr.smoothingGroup >= 0  is only valid if non-negative
			OutData.Faces[FaceIndex].SmoothingGroups = FMath::Max(tr.smoothingGroup, 0);
		}

		FractureSession->BlastMesh->ChunkMeshVolumes[ci] = FMath::Abs(MeshVolume);
	}

	FTransform RootTransform(FTransform::Identity);
	if (SkeletalMesh->GetRefSkeleton().GetRefBonePose().Num())
	{
		RootTransform = SkeletalMesh->GetRefSkeleton().GetRefBonePose()[0];
	}

	OutData.RefBonesBinary = GetBinaryBonesFromFractureData(RootTransform, FractureSession);

	OutData.MaxMaterialIndex = NewMaterials.Num() - 1;
	for (const auto& Material : NewMaterials)
	{
		auto& OutMaterial = OutData.Materials.AddDefaulted_GetRef();
		OutMaterial.MaterialImportName = Material.ImportedMaterialSlotName.ToString();
		OutMaterial.Material = Material.MaterialInterface;
	}
	
	return OutData;
}

// UE 5.1 compatibility: Create skeletal mesh import data from static mesh
// This replaces FStaticToSkeletalMeshConverter::InitializeSkeletalMeshFromStaticMesh which was added in UE 5.2+
FSkeletalMeshImportData CreateSkeletalMeshImportDataFromStaticMesh(const UStaticMesh& InSourceStaticMesh, const TArray<SkeletalMeshImportData::FBone>& Bones)
{
	FSkeletalMeshImportData OutData;
	
	// Get the render data from LOD 0
	const FStaticMeshRenderData* RenderData = InSourceStaticMesh.GetRenderData();
	if (!RenderData || RenderData->LODResources.Num() == 0)
	{
		UE_LOG(LogSkeletalMesh, Error, TEXT("[BLAST] Static mesh has no render data"));
		return OutData;
	}
	
	const FStaticMeshLODResources& LODResource = RenderData->LODResources[0];
	const FPositionVertexBuffer& PositionBuffer = LODResource.VertexBuffers.PositionVertexBuffer;
	const FStaticMeshVertexBuffer& VertexBuffer = LODResource.VertexBuffers.StaticMeshVertexBuffer;
	
	// Get indices
	TArray<uint32> Indices;
	LODResource.IndexBuffer.GetCopy(Indices);
	
	const uint32 NumVertices = PositionBuffer.GetNumVertices();
	const uint32 NumTriangles = Indices.Num() / 3;
	const uint32 NumUVChannels = VertexBuffer.GetNumTexCoords();
	
	// Setup import data arrays
	OutData.Points.AddUninitialized(NumVertices);
	OutData.PointToRawMap.AddUninitialized(NumVertices);
	OutData.Wedges.AddUninitialized(Indices.Num());
	OutData.Faces.SetNum(NumTriangles);
	OutData.Influences.AddUninitialized(NumVertices);
	OutData.RefBonesBinary = Bones;
	OutData.NumTexCoords = FMath::Max<uint32>(1, NumUVChannels);
	OutData.bHasTangents = true;
	OutData.bHasNormals = true;
	OutData.bHasVertexColors = LODResource.VertexBuffers.ColorVertexBuffer.GetNumVertices() > 0;
	
	// Copy vertex positions
	for (uint32 VertIdx = 0; VertIdx < NumVertices; VertIdx++)
	{
		OutData.Points[VertIdx] = PositionBuffer.VertexPosition(VertIdx);
		OutData.PointToRawMap[VertIdx] = VertIdx;
		
		// All vertices bound to root bone (index 0) with full weight
		OutData.Influences[VertIdx].BoneIndex = 0;
		OutData.Influences[VertIdx].VertexIndex = VertIdx;
		OutData.Influences[VertIdx].Weight = 1.0f;
	}
	
	// Process triangles and wedges
	for (uint32 TriIdx = 0; TriIdx < NumTriangles; TriIdx++)
	{
		for (uint32 CornerIdx = 0; CornerIdx < 3; CornerIdx++)
		{
			const uint32 WedgeIdx = TriIdx * 3 + CornerIdx;
			const uint32 VertIdx = Indices[WedgeIdx];
			
			OutData.Wedges[WedgeIdx].VertexIndex = VertIdx;
			OutData.Wedges[WedgeIdx].Color = OutData.bHasVertexColors 
				? LODResource.VertexBuffers.ColorVertexBuffer.VertexColor(VertIdx) 
				: FColor::White;
			
			// Copy UVs
			for (uint32 UVIdx = 0; UVIdx < OutData.NumTexCoords; UVIdx++)
			{
				if (UVIdx < NumUVChannels)
				{
					OutData.Wedges[WedgeIdx].UVs[UVIdx] = VertexBuffer.GetVertexUV(VertIdx, UVIdx);
				}
				else
				{
					OutData.Wedges[WedgeIdx].UVs[UVIdx] = FVector2f::ZeroVector;
				}
			}
			
			OutData.Faces[TriIdx].WedgeIndex[CornerIdx] = WedgeIdx;
			OutData.Faces[TriIdx].TangentZ[CornerIdx] = VertexBuffer.VertexTangentZ(VertIdx);
		}
		
		// Determine material index from section
		int32 MaterialIndex = 0;
		for (const FStaticMeshSection& Section : LODResource.Sections)
		{
			const uint32 FirstTriangle = Section.FirstIndex / 3;
			const uint32 LastTriangle = FirstTriangle + Section.NumTriangles;
			if (TriIdx >= FirstTriangle && TriIdx < LastTriangle)
			{
				MaterialIndex = Section.MaterialIndex;
				break;
			}
		}
		OutData.Faces[TriIdx].MatIndex = MaterialIndex;
		OutData.Faces[TriIdx].SmoothingGroups = 1; // Default smoothing group
	}
	
	// Setup materials
	OutData.MaxMaterialIndex = InSourceStaticMesh.GetStaticMaterials().Num() - 1;
	for (const FStaticMaterial& StaticMat : InSourceStaticMesh.GetStaticMaterials())
	{
		auto& OutMaterial = OutData.Materials.AddDefaulted_GetRef();
		OutMaterial.MaterialImportName = StaticMat.ImportedMaterialSlotName.ToString();
		OutMaterial.Material = StaticMat.MaterialInterface;
	}
	
	return OutData;
}

void CreateSkeletalMeshFromAuthoring(TSharedPtr<FFractureSession> FractureSession, const UStaticMesh& InSourceStaticMesh)
{
	UBlastMesh* BlastMesh = FractureSession->BlastMesh;
	BlastMesh->Mesh = nullptr;
	BlastMesh->Skeleton = nullptr;
	BlastMesh->PhysicsAsset = NewObject<UPhysicsAsset>(
		BlastMesh, *InSourceStaticMesh.GetName().Append(TEXT("_PhysicsAsset")), RF_NoFlags);
	if (!BlastMesh->AssetImportData)
	{
		BlastMesh->AssetImportData = NewObject<UBlastAssetImportData>(BlastMesh);
	}
	
	USkeleton* Skeleton = NewObject<USkeleton>(BlastMesh, *InSourceStaticMesh.GetName().Append(TEXT("_Skeleton")));

	USkeletalMesh* SkeletalMesh = NewObject<USkeletalMesh>(
		BlastMesh, FName(*InSourceStaticMesh.GetName().Append(TEXT("_SkelMesh"))), RF_Public);

	// Copy materials from static mesh
	TArray<FSkeletalMaterial>& SkeletalMaterials = SkeletalMesh->GetMaterials();
	for (const FStaticMaterial& StaticMat : InSourceStaticMesh.GetStaticMaterials())
	{
		FSkeletalMaterial SkeletalMat;
		SkeletalMat.MaterialInterface = StaticMat.MaterialInterface;
		SkeletalMat.MaterialSlotName = StaticMat.MaterialSlotName;
		SkeletalMat.ImportedMaterialSlotName = StaticMat.ImportedMaterialSlotName;
		if (SkeletalMat.MaterialInterface)
		{
			SkeletalMat.MaterialInterface->CheckMaterialUsage(MATUSAGE_SkeletalMesh);
		}
		SkeletalMaterials.Add(SkeletalMat);
	}

	// Create bones from fracture data
	TArray<SkeletalMeshImportData::FBone> Bones = GetBinaryBonesFromFractureData(FTransform::Identity, FractureSession);
	
	// Create import data from static mesh
	FSkeletalMeshImportData ImportData = CreateSkeletalMeshImportDataFromStaticMesh(InSourceStaticMesh, Bones);
	
	if (ImportData.Points.Num() == 0)
	{
		UE_LOG(LogSkeletalMesh, Error, TEXT("[BLAST] Failed to create import data from static mesh!"));
		SkeletalMesh->MarkAsGarbage();
		Skeleton->MarkAsGarbage();
		return;
	}

	// Process skeleton
	int32 SkeletalDepth = 0;
	if (!SkeletalMeshImportUtils::ProcessImportMeshSkeleton(Skeleton, SkeletalMesh->GetRefSkeleton(), SkeletalDepth, ImportData))
	{
		UE_LOG(LogSkeletalMesh, Warning, TEXT("[BLAST] Failed importing skeleton data"));
	}

	// Process materials
	SkeletalMeshImportUtils::ProcessImportMeshMaterials(SkeletalMesh->GetMaterials(), ImportData);
	
	// Process bone influences
	SkeletalMeshImportUtils::ProcessImportMeshInfluences(ImportData, SkeletalMesh->GetPathName());

	// Setup LOD
	FSkeletalMeshModel& ImportedResource = *SkeletalMesh->GetImportedModel();
	ImportedResource.LODModels.Empty();
	ImportedResource.LODModels.Add(new FSkeletalMeshLODModel());
	ImportedResource.LODModels[0].NumTexCoords = FMath::Max<uint32>(1, ImportData.NumTexCoords);

	SkeletalMesh->ResetLODInfo();
	FSkeletalMeshLODInfo& NewLODInfo = SkeletalMesh->AddLODInfo();
	NewLODInfo.ReductionSettings.NumOfTrianglesPercentage = 1.0f;
	NewLODInfo.ReductionSettings.NumOfVertPercentage = 1.0f;
	NewLODInfo.ReductionSettings.MaxDeviationPercentage = 0.0f;
	NewLODInfo.LODHysteresis = 0.02f;
	NewLODInfo.BuildSettings.bRecomputeNormals = false;
	NewLODInfo.BuildSettings.bRecomputeTangents = true;
	NewLODInfo.BuildSettings.bUseMikkTSpace = true;

	// Save import data
	PRAGMA_DISABLE_DEPRECATION_WARNINGS
	SkeletalMesh->SaveLODImportedData(0, ImportData);
	PRAGMA_ENABLE_DEPRECATION_WARNINGS

	// Set bounds
	const FBox3f BoundingBox(ImportData.Points.GetData(), ImportData.Points.Num());
	SkeletalMesh->SetImportedBounds(FBoxSphereBounds(FBox(BoundingBox)));
	SkeletalMesh->SetHasVertexColors(ImportData.bHasVertexColors);
	if (ImportData.bHasVertexColors)
	{
		SkeletalMesh->SetVertexColorGuid(FGuid::NewGuid());
	}
	else
	{
		SkeletalMesh->SetVertexColorGuid(FGuid());
	}

	// Build the skeletal mesh
	IMeshBuilderModule& MeshBuilderModule = IMeshBuilderModule::GetForRunningPlatform();
	FSkeletalMeshBuildParameters SkeletalMeshBuildParameters(SkeletalMesh, GetTargetPlatformManagerRef().GetRunningTargetPlatform(), 0, false);
	const bool bBuildSuccess = MeshBuilderModule.BuildSkeletalMesh(SkeletalMeshBuildParameters);
	
	if (!bBuildSuccess)
	{
		UE_LOG(LogSkeletalMesh, Error, TEXT("[BLAST] Failed to build skeletal mesh from static mesh data!"));
		SkeletalMesh->MarkAsGarbage();
		Skeleton->MarkAsGarbage();
		return;
	}

	SkeletalMesh->CalculateInvRefMatrices();

	if (!SkeletalMesh->GetResourceForRendering() || !SkeletalMesh->GetResourceForRendering()->LODRenderData.IsValidIndex(0))
	{
		SkeletalMesh->Build();
	}

	// Update the skeletal mesh and the skeleton so that their ref skeletons are in sync
	SkeletalMesh->SetSkeleton(Skeleton);
	Skeleton->MergeAllBonesToBoneTree(SkeletalMesh);
	Skeleton->RecreateBoneTree(SkeletalMesh);
	if (!Skeleton->GetPreviewMesh())
	{
		Skeleton->SetPreviewMesh(SkeletalMesh);
	}
	
	BlastMesh->Skeleton = Skeleton;
	BlastMesh->Mesh = SkeletalMesh;
}


void CreateSkeletalMeshFromAuthoring(TSharedPtr<FFractureSession> FractureSession, UMaterialInterface* InteriorMaterial)
{
	USkeletalMesh* SkeletalMesh = FractureSession->BlastMesh->Mesh;
	check(SkeletalMesh);

	TSharedPtr<FScopedSkeletalMeshPostEditChange> ScopedChange = MakeShared<FScopedSkeletalMeshPostEditChange>(SkeletalMesh);
	//Dirty the DDC Key for any imported Skeletal Mesh
	SkeletalMesh->InvalidateDeriveDataCacheGUID();

	FSkeletalMeshModel *ImportedResource = SkeletalMesh->GetImportedModel();
	ImportedResource->LODModels.Empty();
	ImportedResource->LODModels.Add(new FSkeletalMeshLODModel());
	
	TArray<FSkeletalMaterial> ExistingMaterials = SkeletalMesh->GetMaterials();
	FSkeletalMeshImportData ImportData = CreateSkeletalMeshImportDataFromFractureData(FractureSession, ExistingMaterials);
	
	// Pass the number of texture coordinate sets to the LODModel.  Ensure there is at least one UV coord
	ImportedResource->LODModels[0].NumTexCoords = FMath::Max<uint32>(1, ImportData.NumTexCoords);
	
	//New slots must be interior materials, they couldn't have come from anywhere else
	for (int32 NewSlot = ExistingMaterials.Num(); NewSlot < SkeletalMesh->GetMaterials().Num(); NewSlot++)
	{
		FSkeletalMaterial& MatSlot = SkeletalMesh->GetMaterials()[NewSlot];
		MatSlot.MaterialInterface = InteriorMaterial;
		if (MatSlot.MaterialInterface)
		{
			MatSlot.MaterialInterface->CheckMaterialUsage(MATUSAGE_SkeletalMesh);
		}
	}
	
	// process materials from import data
	SkeletalMeshImportUtils::ProcessImportMeshMaterials(SkeletalMesh->GetMaterials(), ImportData);

	// process reference skeleton from import data
	int32 SkeletalDepth = 0;
	if (!SkeletalMeshImportUtils::ProcessImportMeshSkeleton(SkeletalMesh->GetSkeleton(), SkeletalMesh->GetRefSkeleton(), SkeletalDepth, ImportData))
	{
		UE_LOG(LogSkeletalMesh, Warning, TEXT("[BLAST] Failed importing skeleton data"));
	}

	// process bone influences from import data
	SkeletalMeshImportUtils::ProcessImportMeshInfluences(ImportData, SkeletalMesh->GetPathName());

	SkeletalMesh->ResetLODInfo();
	FSkeletalMeshLODInfo& NewLODInfo = SkeletalMesh->AddLODInfo();
	NewLODInfo.ReductionSettings.NumOfTrianglesPercentage = 1.0f;
	NewLODInfo.ReductionSettings.NumOfVertPercentage = 1.0f;
	NewLODInfo.ReductionSettings.MaxDeviationPercentage = 0.0f;
	NewLODInfo.LODHysteresis = 0.02f;
	NewLODInfo.BuildSettings.bRecomputeNormals = false;
	NewLODInfo.BuildSettings.bRecomputeTangents = true;
	NewLODInfo.BuildSettings.bUseMikkTSpace = true;
	
	PRAGMA_DISABLE_DEPRECATION_WARNINGS
	SkeletalMesh->SaveLODImportedData(0, ImportData);
	PRAGMA_ENABLE_DEPRECATION_WARNINGS
	
	const FBox3f BoundingBox(ImportData.Points.GetData(), ImportData.Points.Num());
	SkeletalMesh->SetImportedBounds(FBoxSphereBounds(FBox(BoundingBox)));
	SkeletalMesh->SetHasVertexColors(false);
	SkeletalMesh->SetVertexColorGuid(FGuid());
	
	//The imported LOD is always 0 here, the LOD custom import will import the LOD alone(in a temporary skeletalmesh) and add it to the base skeletal mesh later
	check(SkeletalMesh->GetLODInfo(0) != nullptr);
	//New MeshDescription build process
	IMeshBuilderModule& MeshBuilderModule = IMeshBuilderModule::GetForRunningPlatform();
	//We must build the LODModel so we can restore properly the mesh, but we do not have to regenerate LODs
	FSkeletalMeshBuildParameters SkeletalMeshBuildParameters(SkeletalMesh, GetTargetPlatformManagerRef().GetRunningTargetPlatform(), 0, false);
	const bool bBuildSuccess = MeshBuilderModule.BuildSkeletalMesh(SkeletalMeshBuildParameters);
	if (!bBuildSuccess)
	{
		SkeletalMesh->MarkAsGarbage();
		return;
	}

	SkeletalMesh->CalculateInvRefMatrices();

	if (!SkeletalMesh->GetResourceForRendering() || !SkeletalMesh->GetResourceForRendering()->LODRenderData.IsValidIndex(0))
	{
		//We need to have a valid render data to create physic asset
		SkeletalMesh->Build();
	}
	SkeletalMesh->GetSkeleton()->RecreateBoneTree(SkeletalMesh);
	ScopedChange.Reset();
	
	SkeletalMesh->MarkPackageDirty();

	FractureSession->IsMeshCreatedFromFractureData = true;
}

#undef LOCTEXT_NAMESPACE
