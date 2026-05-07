//------------------------------------------------------------------------------------------------
//! Bone-anchored ray and TraceMove helpers for designator ray components, plus gadget-prefab LOS (no skeleton).
class HMD_DesignatorRayTraceUtils
{
	//------------------------------------------------------------------------------------------------
	//! **Local player** active view ray from `ChimeraWorld.GetCurrentCamera` (same basis as LaserFixesAgain handheld designation).
	static bool TryGetLocalPlayerCameraWorldRay(out vector originWorld, out vector dirWorldUnit)
	{
		originWorld = vector.Zero;
		dirWorldUnit = vector.Zero;
		Game g = GetGame();
		if (!g || !g.InPlayMode())
			return false;
		ChimeraWorld world = g.GetWorld();
		if (!world)
			return false;
		vector tm[4];
		world.GetCurrentCamera(tm);
		originWorld = tm[3];
		vector f = tm[2];
		float len = f.Length();
		if (len < 0.001)
			return false;
		dirWorldUnit = f * (1.0 / len);
		return true;
	}

	//------------------------------------------------------------------------------------------------
	//! Handheld designators without bones: start at the gadget prefab **world pivot** (`GetWorldTransform` translation) and shoot along the prefab **+Z** axis in world space. If **+Z** is degenerate, uses **-X** (common item forward); last resort **world +Z**.
	static void GetGadgetPrefabWorldPivotAndLosDir(IEntity gadget, out vector originWorld, out vector dirWorldUnit)
	{
		originWorld = vector.Zero;
		dirWorldUnit = vector.Zero;
		if (!gadget)
			return;
		vector tm[4];
		gadget.GetWorldTransform(tm);
		originWorld = tm[3];
		vector f = tm[2];
		float len = f.Length();
		if (len > 0.001)
		{
			dirWorldUnit = f * (1.0 / len);
			return;
		}
		f = -tm[0];
		len = f.Length();
		if (len > 0.001)
			dirWorldUnit = f * (1.0 / len);
		else
		{
			dirWorldUnit[0] = 0;
			dirWorldUnit[1] = 0;
			dirWorldUnit[2] = 1;
		}
	}

	//------------------------------------------------------------------------------------------------
	//! Resolves bone ids. Prefer **`Animation.GetBoneIndex`** when an `Animation` exists (Workbench: `IEntity.GetBoneIndex` is obsolete). Mesh-only entities fall back to **`IEntity.GetBoneIndex`**.
	static void GetBoneIdsForEntity(IEntity ent, string boneName, out TNodeId entityBoneId, out TNodeId animBoneId)
	{
		entityBoneId = -1;
		animBoneId = -1;
		if (!ent || boneName.IsEmpty())
			return;
		Animation anim = ent.GetAnimation();
		if (anim)
			animBoneId = anim.GetBoneIndex(boneName);
		if (animBoneId != -1)
			entityBoneId = animBoneId;
		else
			entityBoneId = ent.GetBoneIndex(boneName);
	}

	//------------------------------------------------------------------------------------------------
	//! True when this entity or its `Animation` defines `boneName`.
	static bool MeshEntityDefinesBone(IEntity ent, string boneName)
	{
		if (!ent || boneName.IsEmpty())
			return false;
		TNodeId eid;
		TNodeId aid;
		GetBoneIdsForEntity(ent, boneName, eid, aid);
		return eid != -1 || aid != -1;
	}

	//------------------------------------------------------------------------------------------------
	//! Depth-first: first entity under `root` whose mesh skeleton contains `boneName` (inclusive).
	static IEntity FindMeshEntityWithBoneInSubtree(IEntity root, string boneName, int maxDepth)
	{
		if (!root || maxDepth < 0 || boneName.IsEmpty())
			return null;
		if (MeshEntityDefinesBone(root, boneName))
			return root;
		IEntity child = root.GetChildren();
		while (child)
		{
			IEntity found = FindMeshEntityWithBoneInSubtree(child, boneName, maxDepth - 1);
			if (found)
				return found;
			child = child.GetSibling();
		}
		return null;
	}

	//------------------------------------------------------------------------------------------------
	//! Prefer the subtree of `owner` (turret proxy often has no skeleton; child mesh entity does), then parents.
	//! Falls back to `ResolveAnimationHost` when the bone is not found on the chain.
	static IEntity ResolveBoneAnimationHost(IEntity owner, string boneName)
	{
		if (!owner)
			return null;
		if (boneName.IsEmpty())
			return ResolveAnimationHost(owner);
		IEntity fromSubtree = FindMeshEntityWithBoneInSubtree(owner, boneName, 16);
		if (fromSubtree)
			return fromSubtree;
		IEntity walk = owner.GetParent();
		int guard = 0;
		while (walk && guard < 16)
		{
			if (MeshEntityDefinesBone(walk, boneName))
				return walk;
			walk = walk.GetParent();
			guard++;
		}
		return ResolveAnimationHost(owner);
	}

	//------------------------------------------------------------------------------------------------
	//! Gadgets often have no Animation; walk parents for the first entity that does.
	static IEntity ResolveAnimationHost(IEntity owner)
	{
		if (!owner)
			return null;
		if (owner.GetAnimation())
			return owner;
		IEntity walk = owner.GetParent();
		int guard = 0;
		while (walk && guard < 16)
		{
			if (walk.GetAnimation())
				return walk;
			walk = walk.GetParent();
			guard++;
		}
		return owner;
	}

	//------------------------------------------------------------------------------------------------
	//! Fills `boneTM` in **entity / model space**. Returns **0** = none, **1** = `GetBoneLocalMatrix` (anim fallback or mesh), **3** = **`Animation.GetBoneMatrix`** (evaluated pose; preferred when `Animation` exists). Prefers **`Animation`** APIs when present.
	static int TryGetBoneTransformInEntitySpace(IEntity boneHost, string boneName, out TNodeId entityBoneId, out TNodeId animBoneId, out vector boneTM[4])
	{
		if (!boneHost || boneName.IsEmpty())
			return 0;
		GetBoneIdsForEntity(boneHost, boneName, entityBoneId, animBoneId);
		Animation anim = boneHost.GetAnimation();
		if (anim && animBoneId != -1)
		{
			//! Evaluated bone pose (`[3]` differs from rest `GetBoneLocalMatrix` on turrets such as `v_gun_01`); map `[3]` with `HmdTransformEntityPointToWorld` like local rest pose.
			if (anim.GetBoneMatrix(animBoneId, boneTM))
				return 3;
			if (anim.GetBoneLocalMatrix(animBoneId, boneTM))
				return 1;
			return 0;
		}
		if (entityBoneId != -1 && boneHost.GetBoneLocalMatrix(entityBoneId, boneTM))
			return 1;
		return 0;
	}

	//------------------------------------------------------------------------------------------------
	//! Maps a point in entity / model space to world using `GetWorldTransform` (rotation + `worldTM[3]` pivot).
	static vector HmdTransformEntityPointToWorld(IEntity ent, vector entityLocalPoint)
	{
		if (!ent)
			return vector.Zero;
		vector worldTM[4];
		ent.GetWorldTransform(worldTM);
		return worldTM[0] * entityLocalPoint[0] + worldTM[1] * entityLocalPoint[1] + worldTM[2] * entityLocalPoint[2] + worldTM[3];
	}

	//------------------------------------------------------------------------------------------------
	//! World position of `boneName` on `boneHost` (mesh or animated entity).
	static vector GetBoneWorldPosition(IEntity boneHost, string boneName)
	{
		if (!boneHost)
			return vector.Zero;
		if (boneName.IsEmpty())
			return boneHost.GetOrigin();
		TNodeId entityBoneId;
		TNodeId animBoneId;
		vector boneTM[4];
		int boneSrc = TryGetBoneTransformInEntitySpace(boneHost, boneName, entityBoneId, animBoneId, boneTM);
		if (boneSrc == 0)
			return boneHost.GetOrigin();
		//! **1** = rest local; **3** = evaluated `GetBoneMatrix` — both supply a point in entity/model space for `GetWorldTransform`. Do **not** use raw `IEntity.GetBoneMatrix(..)[3]` as world without this multiply (often wrong on vehicles).
		if (boneSrc == 1 || boneSrc == 3)
			return HmdTransformEntityPointToWorld(boneHost, boneTM[3]);
		return boneHost.GetOrigin();
	}

	//------------------------------------------------------------------------------------------------
	//! World forward (+Z bone axis mapped through entity orientation) for barrel / LOS.
	static vector GetBoneWorldForward(IEntity boneHost, string boneName)
	{
		if (!boneHost)
		{
			vector def;
			def[0] = 0;
			def[1] = 0;
			def[2] = 1;
			return def;
		}
		if (boneName.IsEmpty())
		{
			vector m[4];
			boneHost.GetWorldTransform(m);
			return -m[0];
		}
		TNodeId entityBoneId;
		TNodeId animBoneId;
		vector boneTM[4];
		if (TryGetBoneTransformInEntitySpace(boneHost, boneName, entityBoneId, animBoneId, boneTM) == 0)
		{
			vector m2[4];
			boneHost.GetWorldTransform(m2);
			return -m2[0];
		}
		vector worldTM[4];
		boneHost.GetWorldTransform(worldTM);
		vector localDir = boneTM[2];
		float locLen = localDir.Length();
		if (locLen < 0.001)
		{
			vector m3[4];
			boneHost.GetWorldTransform(m3);
			return -m3[0];
		}
		localDir = localDir * (1.0 / locLen);
		vector forward = worldTM[0] * localDir[0] + worldTM[1] * localDir[1] + worldTM[2] * localDir[2];
		float len = forward.Length();
		if (len < 0.001)
		{
			vector m4[4];
			boneHost.GetWorldTransform(m4);
			return -m4[0];
		}
		return forward * (1.0 / len);
	}

	//------------------------------------------------------------------------------------------------
	static bool TraceRay(IEntity traceOwner, vector start, vector dir, float maxRange, out vector hitPos, out float hitFraction)
	{
		hitPos = vector.Zero;
		hitFraction = 0.0;
		if (!traceOwner || maxRange <= 0.0)
			return false;
		TraceParam param = new TraceParam();
		param.Flags = TraceFlags.ENTS | TraceFlags.WORLD;
		param.LayerMask = EPhysicsLayerDefs.ViewGeometry | EPhysicsLayerDefs.VehicleSimple | EPhysicsLayerDefs.Terrain;
		param.Start = start;
		param.End = start + (dir * maxRange);
		ref array<IEntity> exclude = {};
		exclude.Insert(traceOwner);
		IEntity parent = traceOwner.GetParent();
		if (parent && parent != traceOwner)
			exclude.Insert(parent);
		IEntity root = traceOwner.GetRootParent();
		if (root && root != traceOwner && root != parent)
			exclude.Insert(root);
		param.ExcludeArray = exclude;
		BaseWorld world = traceOwner.GetWorld();
		if (!world)
			return false;
		float frac = world.TraceMove(param, null);
		if (frac <= 0.0)
			return false;
		hitFraction = frac;
		hitPos = param.Start + (param.End - param.Start) * frac;
		float dist = (start - hitPos).Length();
		return dist <= maxRange + 0.05;
	}
}
