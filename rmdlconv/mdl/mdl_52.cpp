// Copyright (c) 2022, rexx
// See LICENSE.txt for licensing information (GPL v3)

#include "stdafx.h"
#include "mdl/studio.h"
#include "versions.h"


//
// ConvertStudioHdr
// Purpose: converts the mdl v52 (Titanfall 1) studiohdr_t struct to mdl v53 compatible (Titanfall 2)
void ConvertStudioHdrFrom52To53(r2::studiohdr_t* out, r1::studiohdr_t* hdr)
{
	printf("converting header...\n");

	out->id = 'TSDI';
	out->version = 53;

	out->checksum = hdr->checksum;

	// :)
	if ((_time64(NULL) % 69420) == 0)
		out->checksum = 0xDEADBEEF;

	memcpy_s(out->name, 64, hdr->name, 64);

	out->length = 0xbadf00d; // needs to be written later

	out->eyeposition = hdr->eyeposition;
	out->illumposition = hdr->illumposition;
	out->hull_min = hdr->hull_min;
	out->hull_max = hdr->hull_max;
	out->view_bbmin = hdr->view_bbmin;
	out->view_bbmax = hdr->view_bbmax;

	// these will probably have to be modified at some point
	out->flags = hdr->flags;

	//-| begin count vars
	out->numbones = hdr->numbones;
	out->numbonecontrollers = hdr->numbonecontrollers;
	out->numhitboxsets = hdr->numhitboxsets;
	//out->numlocalanim = hdr->numlocalanim;
	//out->numlocalseq = hdr->numlocalseq;
	out->activitylistversion = hdr->activitylistversion;
	out->eventsindexed = hdr->eventsindexed;

	out->numtextures = hdr->numtextures;
	out->numcdtextures = hdr->numcdtextures;
	out->numskinref = hdr->numskinref;
	out->numskinfamilies = hdr->numskinfamilies;
	out->numbodyparts = hdr->numbodyparts;
	out->numlocalattachments = hdr->numlocalattachments;
	out->numlocalnodes = hdr->numlocalnodes;

	// skipping all the deprecated flex vars

	out->numikchains = hdr->numikchains;
	out->numlocalposeparameters = hdr->numlocalposeparameters;
	out->keyvaluesize = hdr->keyvaluesize;
	out->numlocalikautoplaylocks = hdr->numlocalikautoplaylocks; // cut?
	out->numincludemodels = hdr->numincludemodels;
	//-| end count vars

	//-| begin misc vars
	out->mass = hdr->mass;
	out->contents = hdr->contents;

	out->constdirectionallightdot = hdr->constdirectionallightdot;
	out->rootLOD = hdr->rootLOD;
	out->numAllowedRootLODs = hdr->numAllowedRootLODs;
	out->defaultFadeDist = hdr->defaultFadeDist;
	out->flVertAnimFixedPointScale = hdr->flVertAnimFixedPointScale;
	//-| end misc vars

	//-| begin studiohdr2 vars
	out->numsrcbonetransform = hdr->pStudioHdr2()->numsrcbonetransform;
	out->illumpositionattachmentindex = hdr->pStudioHdr2()->illumpositionattachmentindex;
	out->m_nPerTriAABBNodeCount = hdr->pStudioHdr2()->m_nPerTriAABBNodeCount;
	out->m_nPerTriAABBLeafCount = hdr->pStudioHdr2()->m_nPerTriAABBLeafCount;
	out->m_nPerTriAABBVertCount = hdr->pStudioHdr2()->m_nPerTriAABBVertCount;
	//-| end studiohdr2 vars
}

void ConvertBonesFrom52To53(r1::mstudiobone_t* pOldBones, int numBones)
{
	printf("converting %i bones...\n", numBones);
	std::vector<r2::mstudiobone_t*> proceduralBones;

	char* pBoneStart = g_model.pData;
	for (int i = 0; i < numBones; ++i)
	{
		r1::mstudiobone_t* oldBone = &pOldBones[i];

		r2::mstudiobone_t* newBone = reinterpret_cast<r2::mstudiobone_t*>(g_model.pData) + i;

		AddToStringTable((char*)newBone, &newBone->sznameindex, STRING_FROM_IDX(oldBone, oldBone->sznameindex));

		AddToStringTable((char*)newBone, &newBone->surfacepropidx, STRING_FROM_IDX(oldBone, oldBone->surfacepropidx));

		newBone->parent = oldBone->parent;
		memcpy(&newBone->bonecontroller, &oldBone->bonecontroller, sizeof(oldBone->bonecontroller));
		newBone->pos = oldBone->pos;
		newBone->quat = oldBone->quat;
		newBone->rot = oldBone->rot;
		newBone->scale = oldBone->scale;
		newBone->posscale = { 0.0f, 0.0f, 0.0f };
		newBone->rotscale = oldBone->rotscale;
		newBone->scalescale = oldBone->scalescale;
		newBone->poseToBone = oldBone->poseToBone;
		newBone->qAlignment = oldBone->qAlignment;
		newBone->flags = oldBone->flags;
		newBone->proctype = oldBone->proctype;
		newBone->procindex = oldBone->procindex;
		newBone->physicsbone = oldBone->physicsbone;
		newBone->contents = oldBone->contents;
		newBone->surfacepropLookup = oldBone->surfacepropLookup;

		newBone->unkIndex = -1;

		if (oldBone->proctype != 0)
			proceduralBones.push_back(newBone);
	}

	g_model.hdrV53()->boneindex = g_model.pData - g_model.pBase;
	g_model.pData += numBones * sizeof(r2::mstudiobone_t);

	ALIGN4(g_model.pData);

	if (proceduralBones.size() > 0)
		printf("converting %lld procedural bones (jiggle bones)...\n", proceduralBones.size());

	for (auto bone : proceduralBones)
	{
		int boneid = ((char*)bone - pBoneStart) / sizeof(r2::mstudiobone_t);
		r1::mstudiobone_t* oldBone = &pOldBones[boneid];
		mstudiojigglebone_t* oldJBone = PTR_FROM_IDX(mstudiojigglebone_t, oldBone, oldBone->procindex);

		mstudiojigglebone_t* jBone = reinterpret_cast<mstudiojigglebone_t*>(g_model.pData);

		bone->procindex = (char*)jBone - (char*)bone;
		jBone->flags = oldJBone->flags;
		jBone->length = oldJBone->length;
		jBone->tipMass = oldJBone->tipMass;
		jBone->yawStiffness = oldJBone->yawStiffness;
		jBone->yawDamping = oldJBone->yawDamping;
		jBone->pitchStiffness = oldJBone->pitchStiffness;
		jBone->pitchDamping = oldJBone->pitchDamping;
		jBone->alongStiffness = oldJBone->alongStiffness;
		jBone->alongDamping = oldJBone->alongDamping;
		jBone->angleLimit = oldJBone->angleLimit;
		jBone->minYaw = oldJBone->minYaw;
		jBone->maxYaw = oldJBone->maxYaw;
		jBone->yawFriction = oldJBone->yawFriction;
		jBone->yawBounce = oldJBone->yawBounce;
		jBone->baseMass = oldJBone->baseMass;
		jBone->baseStiffness = oldJBone->baseStiffness;
		jBone->baseDamping = oldJBone->baseDamping;
		jBone->baseMinLeft = oldJBone->baseMinLeft;
		jBone->baseMaxLeft = oldJBone->baseMaxLeft;
		jBone->baseLeftFriction = oldJBone->baseLeftFriction;
		jBone->baseMinUp = oldJBone->baseMinUp;
		jBone->baseMaxUp = oldJBone->baseMaxUp;
		jBone->baseUpFriction = oldJBone->baseUpFriction;
		jBone->baseMinForward = oldJBone->baseMinForward;
		jBone->baseMaxForward = oldJBone->baseMaxForward;
		jBone->baseForwardFriction = oldJBone->baseForwardFriction;

		jBone->minPitch = oldJBone->minPitch;
		jBone->maxPitch = oldJBone->maxPitch;
		jBone->pitchFriction = oldJBone->pitchFriction;
		jBone->pitchBounce = oldJBone->pitchBounce;

		g_model.pData += sizeof(mstudiojigglebone_t);
	}

	ALIGN4(g_model.pData);
}

void ConvertHitboxesFromMDLTo53(mstudiohitboxset_t* pOldHitboxSets, int numHitboxSets)
{
	printf("converting %i hitboxsets...\n", numHitboxSets);

	g_model.hdrV53()->hitboxsetindex = g_model.pData - g_model.pBase;

	mstudiohitboxset_t* hboxsetStart = reinterpret_cast<mstudiohitboxset_t*>(g_model.pData);
	for (int i = 0; i < numHitboxSets; ++i)
	{
		mstudiohitboxset_t* oldhboxset = &pOldHitboxSets[i];
		mstudiohitboxset_t* newhboxset = reinterpret_cast<mstudiohitboxset_t*>(g_model.pData);

		memcpy(g_model.pData, oldhboxset, sizeof(mstudiohitboxset_t));

		AddToStringTable((char*)newhboxset, &newhboxset->sznameindex, STRING_FROM_IDX(oldhboxset, oldhboxset->sznameindex));

		g_model.pData += sizeof(mstudiohitboxset_t);
	}

	for (int i = 0; i < numHitboxSets; ++i)
	{
		mstudiohitboxset_t* oldhboxset = &pOldHitboxSets[i];
		mstudiohitboxset_t* newhboxset = hboxsetStart + i;

		newhboxset->hitboxindex = g_model.pData - (char*)newhboxset;

		mstudiobbox_t* oldHitboxes = reinterpret_cast<mstudiobbox_t*>((char*)oldhboxset + oldhboxset->hitboxindex);

		for (int j = 0; j < newhboxset->numhitboxes; ++j)
		{
			mstudiobbox_t* oldHitbox = oldHitboxes + j;
			r2::mstudiobbox_t* newHitbox = reinterpret_cast<r2::mstudiobbox_t*>(g_model.pData);

			memcpy(g_model.pData, oldHitbox, sizeof(r2::mstudiobbox_t));

			AddToStringTable((char*)newHitbox, &newHitbox->szhitboxnameindex, STRING_FROM_IDX(oldHitbox, oldHitbox->szhitboxnameindex));
			AddToStringTable((char*)newHitbox, &newHitbox->keyvalueindex, "");

			g_model.pData += sizeof(r2::mstudiobbox_t);
		}
	}

	ALIGN4(g_model.pData);
}

void ConvertBodyPartsFrom52To53(mstudiobodyparts_t* pOldBodyParts, int numBodyParts)
{
	printf("converting %i bodyparts...\n", numBodyParts);

	g_model.hdrV53()->bodypartindex = g_model.pData - g_model.pBase;

	mstudiobodyparts_t* bodypartStart = reinterpret_cast<mstudiobodyparts_t*>(g_model.pData);
	for (int i = 0; i < numBodyParts; ++i)
	{
		mstudiobodyparts_t* oldbodypart = &pOldBodyParts[i];
		mstudiobodyparts_t* newbodypart = reinterpret_cast<mstudiobodyparts_t*>(g_model.pData);

		memcpy(g_model.pData, oldbodypart, sizeof(mstudiobodyparts_t));

		AddToStringTable((char*)newbodypart, &newbodypart->sznameindex, STRING_FROM_IDX(oldbodypart, oldbodypart->sznameindex));

		g_model.pData += sizeof(mstudiobodyparts_t);
	}

	for (int i = 0; i < numBodyParts; ++i)
	{
		mstudiobodyparts_t* oldbodypart = &pOldBodyParts[i];
		mstudiobodyparts_t* newbodypart = bodypartStart + i;

		newbodypart->modelindex = g_model.pData - (char*)newbodypart;

		// pointer to old models (in .mdl)
		r1::mstudiomodel_t* oldModels = reinterpret_cast<r1::mstudiomodel_t*>((char*)oldbodypart + oldbodypart->modelindex);

		// pointer to start of new model data (in .rmdl)
		r1::mstudiomodel_t* newModels = reinterpret_cast<r1::mstudiomodel_t*>(g_model.pData);
		for (int j = 0; j < newbodypart->nummodels; ++j)
		{
			r1::mstudiomodel_t* oldModel = oldModels + j;
			r1::mstudiomodel_t* newModel = reinterpret_cast<r1::mstudiomodel_t*>(g_model.pData);

			memcpy(&newModel->name, &oldModel->name, sizeof(newModel->name));
			newModel->type = oldModel->type;
			newModel->boundingradius = oldModel->boundingradius;
			newModel->nummeshes = oldModel->nummeshes;
			newModel->numvertices = oldModel->numvertices;
			newModel->vertexindex = oldModel->vertexindex;
			newModel->tangentsindex = oldModel->tangentsindex;
			newModel->numattachments = oldModel->numattachments;
			newModel->attachmentindex = oldModel->attachmentindex;
			newModel->deprecated_numeyeballs = oldModel->deprecated_numeyeballs;
			newModel->deprecated_eyeballindex = oldModel->deprecated_eyeballindex;
			newModel->colorindex = oldModel->colorindex;
			newModel->uv2index = oldModel->uv2index;

			g_model.pData += sizeof(r1::mstudiomodel_t);
		}

		for (int j = 0; j < newbodypart->nummodels; ++j)
		{
			r1::mstudiomodel_t* oldModel = oldModels + j;
			r1::mstudiomodel_t* newModel = newModels + j;

			newModel->meshindex = g_model.pData - (char*)newModel;

			// pointer to old meshes for this model (in .mdl)
			r1::mstudiomesh_t* oldMeshes = reinterpret_cast<r1::mstudiomesh_t*>((char*)oldModel + oldModel->meshindex);

			// pointer to new meshes for this model (in .rmdl)
			r2::mstudiomesh_t* newMeshes = reinterpret_cast<r2::mstudiomesh_t*>(g_model.pData);

			for (int k = 0; k < newModel->nummeshes; ++k)
			{
				r1::mstudiomesh_t* oldMesh = oldMeshes + k;
				r2::mstudiomesh_t* newMesh = newMeshes + k;

				memcpy(newMesh, oldMesh, sizeof(r2::mstudiomesh_t));

				newMesh->modelindex = (char*)newModel - (char*)newMesh;

				g_model.pData += sizeof(r2::mstudiomesh_t);
			}
		}
	}

	ALIGN4(g_model.pData);
}

void ConvertIkChainsFromMDLTo53(mstudioikchain_t* pOldIkChains, int numIkChains)
{
	g_model.hdrV53()->ikchainindex = g_model.pData - g_model.pBase;

	printf("converting %i ikchains...\n", numIkChains);

	int currentLinkCount = 0;

	for (int i = 0; i < numIkChains; i++)
	{
		mstudioikchain_t* oldChain = &pOldIkChains[i];
		r2::mstudioikchain_t* newChain = reinterpret_cast<r2::mstudioikchain_t*>(g_model.pData);

		AddToStringTable((char*)newChain, &newChain->sznameindex, STRING_FROM_IDX(oldChain, oldChain->sznameindex));

		newChain->linktype = oldChain->linktype;
		newChain->numlinks = oldChain->numlinks;
		newChain->linkindex = (sizeof(mstudioiklink_t) * currentLinkCount) + (sizeof(r2::mstudioikchain_t) * (numIkChains - i));
		//newChain->unk = oldChain->unk;

		g_model.pData += sizeof(r2::mstudioikchain_t);

		currentLinkCount += oldChain->numlinks;
	}

	for (int i = 0; i < numIkChains; i++)
	{
		mstudioikchain_t* oldChain = &pOldIkChains[i];

		for (int linkIdx = 0; linkIdx < oldChain->numlinks; linkIdx++)
		{
			mstudioiklink_t* oldLink = PTR_FROM_IDX(mstudioiklink_t, oldChain, oldChain->linkindex + (sizeof(mstudioiklink_t) * linkIdx));
			mstudioiklink_t* newLink = reinterpret_cast<mstudioiklink_t*>(g_model.pData);

			newLink->bone = oldLink->bone;
			newLink->kneeDir = oldLink->kneeDir;

			g_model.pData += sizeof(mstudioiklink_t);
		}
	}

	ALIGN4(g_model.pData);
}

struct phyheader_t
{
	int size; // Size of this header section (generally 16), this is also version.
	int id; // Often zero, unknown purpose.
	int solidCount; // Number of solids in file
	int checkSum; // checksum of source .mdl file (4-bytes)
};

struct compactsurfaceheader_t
{
	int size;
	int id;
	short version;

	short modeltype;

	int surfacesize;

	Vector dragaxisareas;

	int axismaparea;
};

struct compactledge_t
{
	int c_point_offset; // byte offset from 'this' to (ledge) point array
	int offsets;
	int packed;
	short n_triangles;
	short for_future_use;
};

struct swapcompactsurfaceheader_t
{
	int		size; // size of the content after this byte
	int		vphysicsID;
	short	version;
	short	modelType;
	int		surfaceSize;
	Vector	dragAxisAreas;
	int		axisMapSize;
};

struct legacysurfaceheader_t
{
	Vector mass_center;
	Vector rotation_inertia;

	float upper_limit_radius;

	// big if true
	int	max_deviation : 8;
	int	byte_size : 24;
	int	offset_ledgetree_root;

	int dummy[3]; // dummy[2] is id
};


struct compactedge_t
{
	unsigned int	start_point_index : 16; // point index
	int				opposite_index : 15; // rel to this // maybe extra array, 3 bits more than tri_index/pierce_index
	unsigned int	is_virtual : 1;
};
static_assert(sizeof(compactedge_t) == 4);
struct compacttriangle_t
{
	unsigned int tri_index : 12; // used for upward navigation
	unsigned int pierce_index : 12;
	unsigned int material_index : 7;
	unsigned int is_virtual : 1;

	// three edges
	compactedge_t c_three_edges[3];
};


struct physection_t
{
	swapcompactsurfaceheader_t surfaceheader;
	legacysurfaceheader_t surfaceheader2;
	compactledge_t ledge;
	compacttriangle_t tri[1];
};

struct phyvertex_t
{
	Vector3 pos; // relative to bone
	char pad[4]; // align to 16 bytes
};

struct edge_t {
	int verts[2];
	int faces[2];
};

struct mapCollHeader_t {
	int faceCount;
	int unkCount;
	int edgeCount;
	int vertCount;
	int dataOffset;
};

struct mapCollEdge_t {
	Vector origin;
	Vector delta;
	uint8_t faceIndices[2];
	uint8_t vertIndices[2];
};


inline __m128 magnitude_ps(__m128 vec) {
	__m128 sqr = _mm_mul_ps(vec,vec);
	__m128 magnitude = _mm_add_ps(sqr,_mm_shuffle_ps(sqr,sqr,_MM_SHUFFLE(2,3,0,1)));
	return _mm_sqrt_ps(_mm_add_ps(magnitude,_mm_shuffle_ps(magnitude,magnitude,_MM_SHUFFLE(0,1,2,3))));
}

inline __m128 dotProduct_ps(__m128 a, __m128 b) {
	__m128 c = _mm_mul_ps(a,b);
	c = _mm_add_ps(c,_mm_shuffle_ps(c,c,_MM_SHUFFLE(2,3,0,1)));
	return _mm_add_ps(c,_mm_shuffle_ps(c,c,_MM_SHUFFLE(0,1,2,3)));
}

inline __m128 normalize_ps(__m128 a) {
	return _mm_div_ps(a,magnitude_ps(a));
}

inline __m128 crossProduct_ps(__m128 u, __m128 v) {
	return _mm_sub_ps(
		_mm_mul_ps(_mm_shuffle_ps(u, u, _MM_SHUFFLE(3, 0, 2, 1)), _mm_shuffle_ps(v, v, _MM_SHUFFLE(3, 1, 0, 2))),
		_mm_mul_ps(_mm_shuffle_ps(u, u, _MM_SHUFFLE(3, 1, 0, 2)), _mm_shuffle_ps(v, v, _MM_SHUFFLE(3, 0, 2, 1))));
}

inline void storeXMMasVec3(Vector3* dest, __m128 src) {
	_mm_maskstore_ps(reinterpret_cast<float*>(dest), _mm_set_epi32(0, ~0, ~0, ~0),src);
}

void ConvertMapColl(char* phyData) {
	g_model.hdrV53()->unkOffset = g_model.pData - g_model.pBase;
	g_model.hdrV53()->unkCount = 0;
	phyheader_t* phyHeader = reinterpret_cast<phyheader_t*>(phyData);
	if (phyHeader->solidCount != 1)return;
	
	physection_t* section = reinterpret_cast<physection_t*>(phyData + sizeof(phyheader_t));
	phyvertex_t* pysVerts = reinterpret_cast<phyvertex_t*>(reinterpret_cast<char*>(&section->ledge)+section->ledge.c_point_offset);

	//to get vertCount get biggest index than increment by 1
	int vertCount = 0;
	for (int i = 0; i < section->ledge.n_triangles; i++) {
		vertCount = max(vertCount,section->tri[i].c_three_edges[0].start_point_index);
		vertCount = max(vertCount,section->tri[i].c_three_edges[1].start_point_index);
		vertCount = max(vertCount,section->tri[i].c_three_edges[2].start_point_index);
	}
	vertCount++;

	std::vector<__m128> verts;
	//load verts and convert them to source format
	for (int i = 0; i < vertCount; i++) {
		__m128 v = _mm_load_ps(&pysVerts[i].pos.x);
		v = _mm_mul_ps(_mm_shuffle_ps(v,v,_MM_SHUFFLE(3,1,2,0)),_mm_set_ps(0,39.3701,-39.3701,39.3701));
		verts.push_back(v);
	}
	std::vector<__m128> faceNormals;
	for (int i = 0; i < section->ledge.n_triangles; i++) {
		__m128 v0 = verts[section->tri[i].c_three_edges[0].start_point_index];
		__m128 v1 = verts[section->tri[i].c_three_edges[1].start_point_index];
		__m128 v2 = verts[section->tri[i].c_three_edges[2].start_point_index];

		__m128 u = _mm_sub_ps(v1,v0);
		__m128 v = _mm_sub_ps(v2,v0);

		__m128 normal = crossProduct_ps(u,v);
		normal = normalize_ps(normal);



		__m128 distance = dotProduct_ps(normal,v0);
		__m128 swap = _mm_shuffle_ps(distance,normal,_MM_SHUFFLE(2,2,0,0));
		normal = _mm_mul_ps(_mm_shuffle_ps(normal,swap,_MM_SHUFFLE(0,2,1,0)),_mm_set_ps(1,-1,-1,-1));
		faceNormals.push_back(normal);
	}
	std::vector<edge_t> edges;
	//build edges
	for (int i = 0; i < section->ledge.n_triangles; i++) {
		for (int j = 0; j < 3; j++) {
			int edgeP0 = section->tri[i].c_three_edges[j].start_point_index;
			int edgeP1 = section->tri[i].c_three_edges[(j+1)%3].start_point_index;
			bool edgeFound = false;
			for (auto& edge : edges) {
				if ((edge.verts[0] == edgeP0 && edge.verts[1] == edgeP1) || (edge.verts[0] == edgeP1 && edge.verts[1] == edgeP0)) {
					edgeFound = true;
					edge.faces[1] = i;
					break;
				}
			}
			if (!edgeFound) {
				edge_t e;
				e.verts[0] = edgeP0;
				e.verts[1] = edgeP1;
				e.faces[0] = i;
				edges.push_back(e);
			}
		}
	}
	
	
	for (int i = 0;i < edges.size();) {
		//the check needs to be fuzzy since the normals have quite a bit of error
		__m128 f0 = faceNormals[edges[i].faces[0]];
		__m128 f1 = faceNormals[edges[i].faces[1]];
		__m128 difference = _mm_sub_ps(f0,f1);
		__m128 allowedDiff = _mm_set1_ps(0.00001);
		const __m128 absMask = _mm_castsi128_ps(_mm_set1_epi32(0x7FFFFFFF));
		difference = _mm_and_ps(absMask,difference);
		if (_mm_movemask_ps(_mm_cmple_ps(difference,allowedDiff))==0xF) {
			
			//remove face normal and fix face indices
			int removeIndex = max(edges[i].faces[0],edges[i].faces[1]);
			int otherIndex = min(edges[i].faces[0],edges[i].faces[1]);
			for (auto& edge : edges) {
				for (int j = 0; j < 2; j++) {
					if(removeIndex<edge.faces[j])
						edge.faces[j]--;
					else if(removeIndex == edge.faces[j]){
						edge.faces[j] = otherIndex;
					}
				}
			}
			faceNormals.erase(faceNormals.begin()+removeIndex);
			//remove edge
			edges.erase(edges.begin()+i);
			continue;
		}

		i++;
		
	}
	
	std::map<int,std::vector<int>> faces;
	for (int j = 0;j<edges.size();j++) {
		faces[edges[j].faces[0]].push_back(j);
		faces[edges[j].faces[1]].push_back(j);
	}
	/*
	for (auto& v : verts) {
		printf("v %f %f %f\n",v.m128_f32[0],v.m128_f32[1],v.m128_f32[2]);
	}
	for (auto f:faces) {
		printf("g group%d\n", f.first);
		printf("usemtl mtl%d\n",f.first);
		int first = edges[f.second[0]].verts[0];
		printf("f %d",first+1);
		int currentIndex = -1;
		int currentVert = first;
		while(true) {
			int next = -1;
			bool found = false;
			for (auto j : f.second) {
				if(j==currentIndex)continue;
				if (edges[j].verts[0] == currentVert) {
					next = edges[j].verts[1];
					currentIndex = j;
					break;
				}
				if (edges[j].verts[1] == currentVert) {
					next = edges[j].verts[0];
					currentIndex = j;
					break;
				}
			}
			if (next == -1) {
				return;
			}
			if(next==first)
				break;
			printf(" %d",next+1);
			currentVert = next;
		}
		printf("\n");
	
	}
	*/

	mapCollHeader_t header{};
	header.faceCount = faceNormals.size();
	header.unkCount = header.faceCount;
	header.edgeCount = edges.size();
	header.vertCount = verts.size();
	header.dataOffset = 20;

	memcpy(g_model.pData,&header,sizeof(mapCollHeader_t));

	g_model.pData += sizeof(mapCollHeader_t);
	memcpy(g_model.pData,faceNormals.data(), faceNormals.size() * sizeof(__m128));
	g_model.pData += faceNormals.size()*sizeof(__m128);
	for (auto& edge : edges) {
		mapCollEdge_t mapEdge;
		storeXMMasVec3(&mapEdge.origin,verts[edge.verts[0]]);
		storeXMMasVec3(&mapEdge.delta,_mm_sub_ps(verts[edge.verts[1]],verts[edge.verts[0]]));
		mapEdge.vertIndices[0] = edge.verts[0];
		mapEdge.vertIndices[1] = edge.verts[1];
		mapEdge.faceIndices[0] = edge.faces[0];
		mapEdge.faceIndices[1] = edge.faces[1];
		memcpy(g_model.pData,&mapEdge,sizeof(mapCollEdge_t));
		g_model.pData += sizeof(mapCollEdge_t);
	}
	for (auto vert : verts) {
		storeXMMasVec3(reinterpret_cast<Vector3*>(g_model.pData),vert);
		g_model.pData += sizeof(Vector3);
	}
	g_model.hdrV53()->unkCount = 1;
}

void ConvertIncludeModels(mstudiomodelgroup_t* pOldModelGroups, int numModelGroups)
{
	g_model.hdrV53()->includemodelindex = g_model.pData - g_model.pBase;

	printf("converting %i includemodels...\n", numModelGroups);

	for (int i = 0; i < numModelGroups; i++)
	{
		mstudiomodelgroup_t* oldGroup = &pOldModelGroups[i];

		mstudiomodelgroup_t* newGroup = reinterpret_cast<mstudiomodelgroup_t*>(g_model.pData);

		AddToStringTable((char*)newGroup, &newGroup->szlabelindex, STRING_FROM_IDX(oldGroup, oldGroup->szlabelindex));
		AddToStringTable((char*)newGroup, &newGroup->sznameindex, STRING_FROM_IDX(oldGroup, oldGroup->sznameindex));

		g_model.pData += sizeof(mstudiomodelgroup_t);
	}
}

void ConvertTexturesFrom52To53(mstudiotexturedir_t* pCDTextures, int numCDTextures, r1::mstudiotexture_t* pOldTextures, int numTextures, r1::studiohdr_t* pOldHdr)
{
	// TODO[rexx]: maybe add old cdtexture parsing here if available, or give the user the option to manually set the material paths
	printf("converting %i textures...\n", numTextures);

	g_model.hdrV53()->textureindex = g_model.pData - g_model.pBase;
	for (int i = 0; i < numTextures; ++i)
	{
		r1::mstudiotexture_t* oldTexture = &pOldTextures[i];

		r2::mstudiotexture_t* newTexture = reinterpret_cast<r2::mstudiotexture_t*>(g_model.pData);

		const char* textureName = STRING_FROM_IDX(oldTexture, oldTexture->sznameindex);
		AddToStringTable((char*)newTexture, &newTexture->sznameindex, textureName);

		g_model.pData += sizeof(r2::mstudiotexture_t);
	}

	ALIGN4(g_model.pData);

	// Write cdtexture data
	g_model.hdrV53()->cdtextureindex = g_model.pData - g_model.pBase;

	printf("converting %i cdtextures...\n", numCDTextures);

	for (int i = 0; i < numCDTextures; ++i)
	{
		mstudiotexturedir_t* oldDir = &pCDTextures[i];

		mstudiotexturedir_t* newDir = reinterpret_cast<mstudiotexturedir_t*>(g_model.pData);

		const char* dirName = STRING_FROM_IDX(pOldHdr, oldDir->sznameindex);
		//printf(dirName);
		//printf("\n");
		AddToStringTable(g_model.pBase, &newDir->sznameindex, dirName);

		//AddToStringTable(g_model.pBase, &newDir->sznameindex, "");

		g_model.pData += sizeof(mstudiotexturedir_t);
	}

	ALIGN4(g_model.pData);
}

void ConvertSkinsFromMDL(char* pOldSkinData, int numSkinRef, int numSkinFamilies)
{
	// TODO[rexx]: maybe add old cdtexture parsing here if available, or give the user the option to manually set the material paths
	printf("converting %i skins (%i skinrefs)...\n", numSkinFamilies, numSkinRef);

	g_model.hdrV53()->skinindex = g_model.pData - g_model.pBase;

	int skinIndexDataSize = sizeof(__int16) * numSkinRef * numSkinFamilies;
	memcpy(g_model.pData, pOldSkinData, skinIndexDataSize);
	g_model.pData += skinIndexDataSize;

	ALIGN4(g_model.pData);
}

void ConvertPerTriAABBFrom52To53(r1::mstudiopertrihdr_t* pOldPerTri, char* pOldAABBTree, int numNodes, int numLeaves, int numVerts)
{
	g_model.hdrV53()->m_nPerTriAABBIndex = g_model.pData - g_model.pBase;

	printf("converting per triangle aabb with %i nodes, %i leaves, and %i verts...\n", numNodes, numLeaves, numVerts);

	r1::mstudiopertrihdr_t* newPerTri = reinterpret_cast<r1::mstudiopertrihdr_t*>(g_model.pData);
	g_model.pData += sizeof(r1::mstudiopertrihdr_t);

	newPerTri->version = pOldPerTri->version;
	newPerTri->bbmin = pOldPerTri->bbmin;
	newPerTri->bbmax = pOldPerTri->bbmax;

	if (newPerTri->version != 2)
		return;

	int aabbTreeSize = (0x14 * numNodes) + (0x70 * numLeaves) + (sizeof(r1::mstudiopertrivertex_t) * numVerts);

	memcpy(g_model.pData, pOldAABBTree, aabbTreeSize);
	g_model.pData += aabbTreeSize;

	ALIGN4(g_model.pData);
}



#define FILEBUFSIZE (32 * 1024 * 1024)

void ConvertMDL52To53(char* pMDL, const std::string& pathIn, const std::string& pathOut)
{
	std::string rawModelName = std::filesystem::path(pathIn).filename().u8string();

	printf("Converting model '%s' from version 52 to version 53...\n", rawModelName.c_str());

	TIME_SCOPE(__FUNCTION__);

	rmem input(pMDL);

	r1::studiohdr_t* oldHeader = input.get<r1::studiohdr_t>();

	std::string outPath = ChangeExtension(pathOut, "mdl_new");
	std::ofstream out(outPath, std::ios::out | std::ios::binary);

	// allocate temp file buffer
	g_model.pBase = new char[FILEBUFSIZE] {};
	g_model.pData = g_model.pBase;

	// convert mdl hdr
	r2::studiohdr_t* pHdr = reinterpret_cast<r2::studiohdr_t*>(g_model.pData);
	ConvertStudioHdrFrom52To53(pHdr, oldHeader);
	g_model.pHdr = pHdr;
	g_model.pData += sizeof(r2::studiohdr_t);

	// do other file buffers after header so we can save size

	//-| begin phy reading  |-----
	std::unique_ptr<char[]> phyBuf;
	if (FILE_EXISTS(ChangeExtension(pathIn, "phy")))
	{
		printf("model has 'phy' file...\n");

		std::string phyPath = ChangeExtension(pathIn, "phy");

		size_t phySize = GetFileSize(phyPath);
		phyBuf = std::unique_ptr<char[]>(new char[phySize]);

		std::ifstream phyIn(phyPath, std::ios::in | std::ios::binary);
		phyIn.read(phyBuf.get(), phySize);
		phyIn.close();

		g_model.hdrV53()->phySize = phySize;
	}
	//-| end phy reading   |-----

	//-| begin vtx reading  |-----
	std::unique_ptr<char[]> vtxBuf;
	if (FILE_EXISTS(ChangeExtension(pathIn, "dx11.vtx")))
	{
		printf("model has 'vtx' file...\n");

		std::string vtxPath = ChangeExtension(pathIn, "dx11.vtx");

		size_t vtxSize = GetFileSize(vtxPath);
		vtxBuf = std::unique_ptr<char[]>(new char[vtxSize]);

		std::ifstream vtxIn(vtxPath, std::ios::in | std::ios::binary);
		vtxIn.read(vtxBuf.get(), vtxSize);
		vtxIn.close();

		g_model.hdrV53()->vtxSize = vtxSize;
	}
	//-| end vtx reading   |-----

	//-| begin vvd reading |-----
	std::unique_ptr<char[]> vvdBuf;
	if (FILE_EXISTS(ChangeExtension(pathIn, "vvd")))
	{
		printf("model has 'vvd' file...\n");

		std::string vvdPath = ChangeExtension(pathIn, "vvd");

		size_t vvdSize = GetFileSize(vvdPath);
		vvdBuf = std::unique_ptr<char[]>(new char[vvdSize]);

		std::ifstream vvdIn(vvdPath, std::ios::in | std::ios::binary);
		vvdIn.read(vvdBuf.get(), vvdSize);
		vvdIn.close();

		g_model.hdrV53()->vvdSize = vvdSize;
	}
	//-| end vvd reading

	//-| begin vvc reading |-----
	std::unique_ptr<char[]> vvcBuf;
	if (FILE_EXISTS(ChangeExtension(pathIn, "vvc")))
	{
		printf("model has 'vvc' file...\n");

		std::string vvcPath = ChangeExtension(pathIn, "vvc");

		size_t vvcSize = GetFileSize(vvcPath);
		vvcBuf = std::unique_ptr<char[]>(new char[vvcSize]);

		std::ifstream vvcIn(vvcPath, std::ios::in | std::ios::binary);
		vvcIn.read(vvcBuf.get(), vvcSize);
		vvcIn.close();

		g_model.hdrV53()->vvcSize = vvcSize;
	}
	//-| end vvc reading

	// eventually we will need to load ani files

	// init string table so we can use 
	BeginStringTable();

	std::string modelName = oldHeader->pszName();

	printf(modelName.c_str());

	AddToStringTable((char*)pHdr, &pHdr->sznameindex, modelName.c_str());
	AddToStringTable((char*)pHdr, &pHdr->unkStringOffset, oldHeader->pszUnkString()); // "Titan" or empty
	AddToStringTable((char*)pHdr, &pHdr->surfacepropindex, STRING_FROM_IDX(pMDL, oldHeader->surfacepropindex));

	// source file for lulz
	input.seek(oldHeader->sourceFilenameOffset, rseekdir::beg);
	input.read(g_model.pData, oldHeader->boneindex - oldHeader->sourceFilenameOffset);

	pHdr->sourceFilenameOffset = g_model.pData - g_model.pBase;

	g_model.pData += oldHeader->boneindex - oldHeader->sourceFilenameOffset;
	ALIGN4(g_model.pData);

	// convert bones and jigglebones
	input.seek(oldHeader->boneindex, rseekdir::beg);
	ConvertBonesFrom52To53((r1::mstudiobone_t*)input.getPtr(), oldHeader->numbones);

	pHdr->bonecontrollerindex = g_model.pData - g_model.pBase;

	// convert attachments
	input.seek(oldHeader->localattachmentindex, rseekdir::beg);
	g_model.hdrV53()->localattachmentindex = ConvertAttachmentsToMDL((mstudioattachment_t*)input.getPtr(), oldHeader->numlocalattachments);

	// convert hitboxsets and hitboxes
	input.seek(oldHeader->hitboxsetindex, rseekdir::beg);
	ConvertHitboxesFromMDLTo53((mstudiohitboxset_t*)input.getPtr(), oldHeader->numhitboxsets);

	// copy bonebyname table (bone ids sorted alphabetically by name)
	input.seek(oldHeader->bonetablebynameindex, rseekdir::beg);
	input.read(g_model.pData, g_model.hdrV53()->numbones);

	g_model.hdrV53()->bonetablebynameindex = g_model.pData - g_model.pBase;
	g_model.pData += g_model.hdrV53()->numbones;

	ALIGN4(g_model.pData);

	//ConvertAnims_49();

	// convert bodyparts, models, and meshes
	input.seek(oldHeader->bodypartindex, rseekdir::beg);
	ConvertBodyPartsFrom52To53((mstudiobodyparts_t*)input.getPtr(), oldHeader->numbodyparts);

	// set these even though they are unused
	pHdr->deprecated_flexdescindex = g_model.pData - g_model.pBase;
	pHdr->deprecated_flexcontrollerindex = g_model.pData - g_model.pBase;
	pHdr->deprecated_flexruleindex = g_model.pData - g_model.pBase;
	pHdr->deprecated_flexcontrolleruiindex = g_model.pData - g_model.pBase;

	input.seek(oldHeader->ikchainindex, rseekdir::beg);
	ConvertIkChainsFromMDLTo53((mstudioikchain_t*)input.getPtr(), oldHeader->numikchains);

	input.seek(oldHeader->localposeparamindex, rseekdir::beg);
	g_model.hdrV53()->localposeparamindex = ConvertPoseParams((mstudioposeparamdesc_t*)input.getPtr(), oldHeader->numlocalposeparameters, false);

	// should be after meshes
	pHdr->uiPanelOffset = g_model.pData - g_model.pBase;

	input.seek(oldHeader->includemodelindex, rseekdir::beg);
	ConvertIncludeModels((mstudiomodelgroup_t*)input.getPtr(), oldHeader->numincludemodels);

	// get cdtextures pointer for converting textures
	input.seek(oldHeader->cdtextureindex, rseekdir::beg);
	void* pOldCDTextures = input.getPtr();

	// convert textures
	input.seek(oldHeader->textureindex, rseekdir::beg);
	ConvertTexturesFrom52To53((mstudiotexturedir_t*)pOldCDTextures, oldHeader->numcdtextures, (r1::mstudiotexture_t*)input.getPtr(), oldHeader->numtextures, oldHeader);

	// convert skin data
	input.seek(oldHeader->skinindex, rseekdir::beg);
	ConvertSkinsFromMDL((char*)input.getPtr(), oldHeader->numskinref, oldHeader->numskinfamilies);

	// write base keyvalues
	input.seek(oldHeader->keyvalueindex, rseekdir::beg);
	input.read(g_model.pData, oldHeader->keyvaluesize);

	pHdr->keyvalueindex = g_model.pData - g_model.pBase;
	pHdr->keyvaluesize = oldHeader->keyvaluesize;

	g_model.pData += oldHeader->keyvaluesize;
	ALIGN4(g_model.pData);

	// SrcBoneTransforms
	input.seek(oldHeader->pStudioHdr2()->srcbonetransformindex, rseekdir::beg);
	g_model.hdrV53()->srcbonetransformindex = ConvertSrcBoneTransforms((mstudiosrcbonetransform_t*)input.getPtr(), oldHeader->pStudioHdr2()->numsrcbonetransform);

	if (oldHeader->pStudioHdr2()->linearboneindex && oldHeader->numbones > 1)
	{
		mstudiolinearbone_t* pLinearBones = reinterpret_cast<mstudiolinearbone_t*>(oldHeader->pStudioHdr2()->pLinearBones());
		ConvertLinearBoneTableTo53(pLinearBones, (char*)pLinearBones + sizeof(mstudiolinearbone_t));
	}

	r1::mstudiopertrihdr_t* pPerTriAABB = oldHeader->pStudioHdr2()->pPerTriHdr();
	ConvertPerTriAABBFrom52To53(pPerTriAABB, (char*)pPerTriAABB + sizeof(r1::mstudiopertrihdr_t), oldHeader->pStudioHdr2()->m_nPerTriAABBNodeCount, oldHeader->pStudioHdr2()->m_nPerTriAABBLeafCount, oldHeader->pStudioHdr2()->m_nPerTriAABBVertCount); // looooong

	g_model.pData = WriteStringTable(g_model.pData);
	ALIGN4(g_model.pData);

	if (phyBuf)
	{
		printf("inserting phy...\n");

		g_model.hdrV53()->phyOffset = g_model.pData - g_model.pBase;
		memcpy(g_model.pData, phyBuf.get(), g_model.hdrV53()->phySize);

		g_model.pData += g_model.hdrV53()->phySize;
	}

	//g_model.hdrV53()->unkOffset = g_model.pData - g_model.pBase;
	ConvertMapColl(phyBuf.get());
	g_model.hdrV53()->boneFollowerOffset = g_model.pData - g_model.pBase;

	if (vtxBuf)
	{
		printf("inserting vtx...\n");

		g_model.hdrV53()->vtxOffset = g_model.pData - g_model.pBase;
		memcpy(g_model.pData, vtxBuf.get(), g_model.hdrV53()->vtxSize);

		g_model.pData += g_model.hdrV53()->vtxSize;
	}

	if (vvdBuf)
	{
		printf("inserting vvd...\n");

		g_model.hdrV53()->vvdOffset = g_model.pData - g_model.pBase;
		memcpy(g_model.pData, vvdBuf.get(), g_model.hdrV53()->vvdSize);

		g_model.pData += g_model.hdrV53()->vvdSize;
	}

	if (vvcBuf)
	{
		printf("inserting vvc...\n");

		g_model.hdrV53()->vvcOffset = g_model.pData - g_model.pBase;
		memcpy(g_model.pData, vvcBuf.get(), g_model.hdrV53()->vvcSize);

		g_model.pData += g_model.hdrV53()->vvcSize;
	}

	pHdr->length = g_model.pData - g_model.pBase;

	out.write(g_model.pBase, pHdr->length);

	delete[] g_model.pBase;

	g_model.stringTable.clear(); // cleanup string table

	printf("Finished converting model '%s', proceeding...\n\n", rawModelName.c_str());
}
