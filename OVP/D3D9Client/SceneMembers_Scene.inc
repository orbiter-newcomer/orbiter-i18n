// ==============================================================
// SceneMembers_Scene.inc - project extension member declarations for class Scene.
// Included inside the class body in the official header: the official
// header only grows by the include line.
// ==============================================================

public:
	// Unified scene shadow system (G1): parameters of the scene shadow group
	// (terrain self shadowing, distant vessel/base shadows). Implementation in
	// TerrainRender.inc (included at the end of Scene.cpp).
	SHADOWMAPPARAM smapT;
	bool smapTValid;


public:
	inline const SHADOWMAPPARAM * GetSMapDataT() const { return &smapT; }
	inline bool IsSceneShadowValid() const { return smapTValid; }

public:
	void RenderMainScene2D();	// dual thread: HUD/2D overlay part of the main scene (main thread)

public:

	// Unified scene shadow system (implementation: TerrainRender.inc)
	void RenderSceneShadow(class vPlanet *vp);
	void InitShadowSystem();
	void ExitShadowSystem();

private:
	// Unified scene shadow system (G1), implementation in TerrainRender.inc
	class ShadowSystem *pShadows;


private:

	// Relativity render cache (implementation in Relativity.inc): the cubemap
	// and the star field composite are only re-rendered when their input
	// parameters (sky brightness, observer velocity, lensing bodies, camera
	// orientation) have changed noticeably. Near a black hole or at high speed
	// these parameters change quickly, so the effect is refreshed every frame
	// there; in normal flight conditions most frames can reuse the previous
	// result.
	struct REL_CACHE {
		bool        valid;
		float       brightness;
		float       beta;			///< observer speed / c
		D3DXVECTOR3 velDir;			///< observer velocity direction
		int         nLens;
		LENSBODY    lens[REL_MAX_LENS];
		D3DXVECTOR3 camX, camY, camZ;	///< camera orientation (composite pass only)
		float       tanW, tanH;		///< camera aperture (composite pass only)
	};
	REL_CACHE relCubeCache;
	REL_CACHE relCompCache;
	bool RelParamsChanged(REL_CACHE &cache, bool bIncludeCamera);
	void RelParamsSave(REL_CACHE &cache, bool bIncludeCamera);

