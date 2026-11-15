// ============================================================================
// TerrainRender.inc
//
// Terrain-rendering features that are kept outside the official Orbiter
// sources. The official files only call into this file at a few hook points,
// so that the official sources stay (almost) untouched and all terrain related
// features can be extended here in one place.
//
// Current features:
//  - Base building placement on the actual (high-resolution) terrain surface:
//    rest each building on the lowest terrain point below its footprint.
// ============================================================================
#ifndef ORBITER_TERRAINRENDER_INC
#define ORBITER_TERRAINRENDER_INC

// Returns true if the object is a base building whose model was designed for a
// flat ground plane (and therefore should rest on the terrain footprint).
static bool TerrainRender_IsBuilding (BaseObject *obj)
{
	return (dynamic_cast<Block*>(obj) != NULL)
		|| (dynamic_cast<Hangar*>(obj) != NULL)
		|| (dynamic_cast<Hangar2*>(obj) != NULL)
		|| (dynamic_cast<Hangar3*>(obj) != NULL)
		|| (dynamic_cast<Tank*>(obj) != NULL);
}

// Returns the lowest terrain elevation below the object's footprint.
static double TerrainRender_FootprintMinElevation (BaseObject *obj, const Base *base, const Vector &relpos,
                                                   const Vector &scale, double rot, double elev)
{
	double hx, hz;
	if (dynamic_cast<Tank*>(obj)) {
		// tank footprint: ellipse with half axes scale.x / scale.z
		hx = scale.x;
		hz = scale.z;
	}
	else {
		hx = 0.5 * scale.x;
		hz = 0.5 * scale.z;
	}
	if (hx < 0.5 && hz < 0.5) return elev;	// point-like object: nothing to sample
	double srot = sin (rot), crot = cos (rot);
	// footprint corners (same rotation convention as the geometry generators)
	double dx[4] = {  hx*crot + hz*srot, -hx*crot + hz*srot, -hx*crot - hz*srot,  hx*crot - hz*srot };
	double dz[4] = {  hx*srot - hz*crot, -hx*srot - hz*crot, -hx*srot + hz*crot,  hx*srot + hz*crot };
	// larger footprints get a denser sample grid so that local dips are not missed
	int nsub = ((hx > hz ? hx : hz) > 20.0 ? 4 : 2);	// 4 -> 5x5 samples, 2 -> 3x3 samples
	double cx = 0.25*(dx[0]+dx[1]+dx[2]+dx[3]);	// footprint centre
	double cz = 0.25*(dz[0]+dz[1]+dz[2]+dz[3]);
	double ux = 0.25*(dx[0]+dx[3]-dx[1]-dx[2]);	// half axis along local x
	double uz = 0.25*(dz[0]+dz[3]-dz[1]-dz[2]);
	double vx = 0.25*(dx[0]+dx[1]-dx[2]-dx[3]);	// half axis along local z
	double vz = 0.25*(dz[0]+dz[1]-dz[2]-dz[3]);
	const Planet *planet = base->RefPlanet();
	double lo = elev;
	for (int iy = 0; iy <= nsub; iy++) {
		double fy = (double)iy / nsub * 2.0 - 1.0;	// -1 .. +1
		for (int ix = 0; ix <= nsub; ix++) {
			double fx = (double)ix / nsub * 2.0 - 1.0;
			double px = cx + ux*fx + vx*fy;
			double pz = cz + uz*fx + vz*fy;
			double lng, lat;
			base->Rel_EquPos (Vector (relpos.x + px, 0.0, relpos.z + pz), lng, lat);
			double e = planet->Elevation (lng, lat);
			if (e < lo) lo = e;
		}
	}
	return lo;
}

// Fix the base object placement: rest buildings on the lowest terrain point below
// their footprint. Base models were built for a flat ground plane; with a
// high-resolution (non-flat) terrain surface most buildings would float above
// the terrain on their downhill side. The uphill side may then be partially
// buried, which is far less noticeable than floating structures.
static inline double TerrainRender_FixBaseElevation (BaseObject *obj, const Base *base, const Vector &relpos,
                                                     const Vector &scale, double rot, double elev)
{
	if (!TerrainRender_IsBuilding (obj)) return elev;
	double elevMin = TerrainRender_FootprintMinElevation (obj, base, relpos, scale, rot, elev);
	return (elevMin < elev ? elevMin : elev);
}

#endif // ORBITER_TERRAINRENDER_INC
