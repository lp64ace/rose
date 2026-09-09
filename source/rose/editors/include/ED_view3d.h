#ifndef ED_VIEW3D_H
#define ED_VIEW3D_H

struct ARegion;

#ifdef __cplusplus
extern "C" {
#endif

#define IS_CLIPPED 12000

/* return values for ED_view3d_project_...() */
typedef enum eV3DProjStatus {
	V3D_PROJ_RET_OK = 0,
	/** can't avoid this when in perspective mode, (can't avoid) */
	V3D_PROJ_RET_CLIP_NEAR = 1,
	/** After clip_end. */
	V3D_PROJ_RET_CLIP_FAR = 2,
	/** so close to zero we can't apply a perspective matrix usefully */
	V3D_PROJ_RET_CLIP_ZERO = 3,
	/** bounding box clip - RV3D_CLIPPING */
	V3D_PROJ_RET_CLIP_BB = 4,
	/** outside window bounds */
	V3D_PROJ_RET_CLIP_WIN = 5,
	/** outside range (mainly for short), (can't avoid) */
	V3D_PROJ_RET_OVERFLOW = 6,
} eV3DProjStatus;

/* some clipping tests are optional */
typedef enum eV3DProjTest {
	V3D_PROJ_TEST_NOP = 0,
	V3D_PROJ_TEST_CLIP_WIN = (1 << 0),
	V3D_PROJ_TEST_CLIP_NEAR = (1 << 1),
	V3D_PROJ_TEST_CLIP_FAR = (1 << 2),
	V3D_PROJ_TEST_CLIP_ZERO = (1 << 3),
	/**
	 * Clip the contents of the data being iterated over.
	 * Currently this is only used to edges when projecting into screen space.
	 *
	 * Clamp the edge within the viewport limits defined by
	 * #V3D_PROJ_TEST_CLIP_WIN, #V3D_PROJ_TEST_CLIP_NEAR & #V3D_PROJ_TEST_CLIP_FAR.
	 * This resolves the problem of a visible edge having one of it's vertices
	 * behind the viewport. See: T32214.
	 *
	 * This is not default behavior as it may be important for the screen-space location
	 * of an edges vertex to represent that vertices location (instead of a location along the edge).
	 *
	 * \note Perspective views should enable #V3D_PROJ_TEST_CLIP_WIN along with
	 * #V3D_PROJ_TEST_CLIP_NEAR as the near-plane-clipped location of a point
	 * may become very large (even infinite) when projected into screen-space.
	 * Unless that point happens to coincide with the camera's point of view.
	 *
	 * Use #V3D_PROJ_TEST_CLIP_CONTENT_DEFAULT instead of #V3D_PROJ_TEST_CLIP_CONTENT,
	 * to avoid accidentally enabling near clipping without clipping by window bounds.
	 */
	V3D_PROJ_TEST_CLIP_CONTENT = (1 << 4),
} eV3DProjTest;

#define V3D_PROJ_TEST_CLIP_DEFAULT (V3D_PROJ_TEST_CLIP_WIN | V3D_PROJ_TEST_CLIP_NEAR)
#define V3D_PROJ_TEST_ALL (V3D_PROJ_TEST_CLIP_DEFAULT | V3D_PROJ_TEST_CLIP_FAR | V3D_PROJ_TEST_CLIP_ZERO | V3D_PROJ_TEST_CLIP_CONTENT)
#define V3D_PROJ_TEST_CLIP_CONTENT_DEFAULT (V3D_PROJ_TEST_CLIP_CONTENT | V3D_PROJ_TEST_CLIP_NEAR | V3D_PROJ_TEST_CLIP_FAR | V3D_PROJ_TEST_CLIP_WIN)

eV3DProjStatus ED_view3d_project_float_global(const struct ARegion *region, const float co[3], float r_co[2], eV3DProjTest flag);

#ifdef __cplusplus
}
#endif

#endif	// !ED_VIEW3D_H
