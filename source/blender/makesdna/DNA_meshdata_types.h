/*
 * ***** BEGIN GPL LICENSE BLOCK *****
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 * The Original Code is Copyright (C) 2001-2002 by NaN Holding BV.
 * All rights reserved.
 *
 * The Original Code is: all of this file.
 *
 * Contributor(s): none yet.
 *
 * ***** END GPL LICENSE BLOCK *****
 */

/** \file DNA_meshdata_types.h
 *  \ingroup DNA
 */

#ifndef __DNA_MESHDATA_TYPES_H__
#define __DNA_MESHDATA_TYPES_H__

#include "DNA_customdata_types.h"
#include "DNA_listBase.h"

struct Bone;
struct Image;

/*tessellation face, see MLoop/MPoly for the real face data*/
typedef struct MFace {
	unsigned int v1, v2, v3, v4;
	short mat_nr;
	char edcode, flag;  /* we keep edcode, for conversion to edges draw flags in old files */
} MFace;

typedef struct MEdge {
	unsigned int v1, v2;
	char crease, bweight;
	short flag;
} MEdge;

typedef struct MDeformWeight {
	int def_nr;
	float weight;
} MDeformWeight;

typedef struct MDeformVert {
	struct MDeformWeight *dw;
	int totweight;
	int flag;  /* flag only in use for weightpaint now */
} MDeformVert;

typedef struct MVert {
	float co[3];
	short no[3];
	char flag, bweight;
} MVert;

/* tessellation vertex color data.
 * at the moment alpha is abused for vertex painting and not used for transparency, note that red and blue are swapped
 */
typedef struct MCol {
	char a, r, g, b;
} MCol;

/* new face structure, replaces MFace, which is now only used for storing tessellations.*/
typedef struct MPoly {
	/* offset into loop array and number of loops in the face */
	int loopstart;
	int totloop;  /* keep signed since we need to subtract when getting the previous loop */
	short mat_nr;
	char flag, pad;
} MPoly;

/* the e here is because we want to move away from relying on edge hashes.*/
typedef struct MLoop {
	unsigned int v;  /* vertex index */
	unsigned int e;  /* edge index */
} MLoop;

typedef struct MTexPoly {
	struct Image *tpage;
	char flag, transp;
	short mode, tile, pad;
} MTexPoly;

/* can copy from/to MTexPoly/MTFace */
#define ME_MTEXFACE_CPY(dst, src)   \
{                                   \
	(dst)->tpage  = (src)->tpage;   \
	(dst)->flag   = (src)->flag;    \
	(dst)->transp = (src)->transp;  \
	(dst)->mode   = (src)->mode;    \
	(dst)->tile   = (src)->tile;    \
} (void)0

typedef struct MLoopUV {
	float uv[2];
	int flag;
} MLoopUV;

/*mloopuv->flag*/
enum {
	MLOOPUV_EDGESEL = (1 << 0),
	MLOOPUV_VERTSEL = (1 << 1),
	MLOOPUV_PINNED  = (1 << 2),
};

/**
 * at the moment alpha is abused for vertex painting,
 * otherwise it should _always_ be initialized to 255
 * Mostly its not used for transparency...
 * (except for blender-internal rendering, see [#34096]).
 *
 * \note red and blue are _not_ swapped, as they are with #MCol
 */
typedef struct MLoopCol {
	char r, g, b, a;
} MLoopCol;

#define MESH_MLOOPCOL_FROM_MCOL(_mloopcol, _mcol) \
{                                                 \
	MLoopCol   *mloopcol__tmp = _mloopcol;        \
	const MCol *mcol__tmp     = _mcol;            \
	mloopcol__tmp->r = mcol__tmp->b;              \
	mloopcol__tmp->g = mcol__tmp->g;              \
	mloopcol__tmp->b = mcol__tmp->r;              \
	mloopcol__tmp->a = mcol__tmp->a;              \
} (void)0


#define MESH_MLOOPCOL_TO_MCOL(_mloopcol, _mcol) \
{                                               \
	const MLoopCol *mloopcol__tmp = _mloopcol;  \
	MCol           *mcol__tmp     = _mcol;      \
	mcol__tmp->b = mloopcol__tmp->r;            \
	mcol__tmp->g = mloopcol__tmp->g;            \
	mcol__tmp->r = mloopcol__tmp->b;            \
	mcol__tmp->a = mloopcol__tmp->a;            \
} (void)0

typedef struct MSelect {
	int index;
	int type;  /* ME_VSEL/ME_ESEL/ME_FSEL */
} MSelect;

/*tessellation uv face data*/
typedef struct MTFace {
	float uv[4][2];
	struct Image *tpage;
	char flag, transp;
	short mode, tile, unwrap;
} MTFace;

/*Custom Data Properties*/
typedef struct MFloatProperty {
	float f;
} MFloatProperty;
typedef struct MIntProperty {
	int i;
} MIntProperty;
typedef struct MStringProperty {
	char s[255], s_len;
} MStringProperty;

typedef struct OrigSpaceFace {
	float uv[4][2];
} OrigSpaceFace;

typedef struct OrigSpaceLoop {
	float uv[2];
} OrigSpaceLoop;

typedef struct MDisps {
	/* Strange bug in SDNA: if disps pointer comes first, it fails to see totdisp */
	int totdisp;
	int level;
	float (*disps)[3];

	/* Used for hiding parts of a multires mesh. Essentially the multires equivalent of MVert.flag's ME_HIDE bit.
	 * NOTE: This is a bitmap, keep in sync with type used in BLI_bitmap.h
	 */
	unsigned int *hidden;
} MDisps;

/** Multires structs kept for compatibility with old files **/
typedef struct MultiresCol {
	float a, r, g, b;
} MultiresCol;

typedef struct MultiresColFace {
	/* vertex colors */
	MultiresCol col[4];
} MultiresColFace;

typedef struct MultiresFace {
	unsigned int v[4];
	unsigned int mid;
	char flag, mat_nr, pad[2];
} MultiresFace;

typedef struct MultiresEdge {
	unsigned int v[2];
	unsigned int mid;
} MultiresEdge;

struct MultiresMapNode;
typedef struct MultiresLevel {
	struct MultiresLevel *next, *prev;

	MultiresFace *faces;
	MultiresColFace *colfaces;
	MultiresEdge *edges;

	unsigned int totvert, totface, totedge, pad;

	/* Kept for compatibility with even older files */
	MVert *verts;
} MultiresLevel;

typedef struct Multires {
	ListBase levels;
	MVert *verts;

	unsigned char level_count, current, newlvl, edgelvl, pinlvl, renderlvl;
	unsigned char use_col, flag;

	/* Special level 1 data that cannot be modified from other levels */
	CustomData vdata;
	CustomData fdata;
	short *edge_flags;
	char *edge_creases;
} Multires;

/** End Multires **/

typedef struct MRecast {
	int i;
} MRecast;

typedef struct GridPaintMask {
	/* The data array contains gridsize*gridsize elements */
	float *data;

	/* The maximum multires level associated with this grid */
	unsigned int level;

	int pad;
} GridPaintMask;

typedef enum MVertSkinFlag {
	/* Marks a vertex as the edge-graph root, used for calculating rotations for all connected edges (recursively).
	 * Also used to choose a root when generating an armature.
	 */
	MVERT_SKIN_ROOT = 1,

	/* Marks a branch vertex (vertex with more than two connected edges), so that it's neighbors are
	 * directly hulled together, rather than the default of generating intermediate frames.
	 */
	MVERT_SKIN_LOOSE = 2,
} MVertSkinFlag;

typedef struct MVertSkin {
	/* Radii of the skin, define how big the generated frames are. Currently only the first two elements are used. */
	float radius[3];

	/* MVertSkinFlag */
	int flag;
} MVertSkin;

typedef enum MPtexDataType {
	MPTEX_DATA_TYPE_UINT8   = 0,
	/* Reserved for compatibility with Ptex file format
	 * specification */
	/* MPTEX_DATA_TYPE_UINT16  = 1, */
	/* MPTEX_DATA_TYPE_FLOAT16 = 2, */
	MPTEX_DATA_TYPE_FLOAT32 = 3,
} MPtexDataType;

typedef struct MPtexTexelInfo {
	unsigned char num_channels;

	/* enum MPtexDataType */
	unsigned char data_type;

	unsigned char pad[2];
} MPtexTexelInfo;


typedef struct MPtexLogRes {
	unsigned char u;
	unsigned char v;

	unsigned char pad[2];
} MPtexLogRes;

/* Ptex texture data attached to mesh poly loops
 *
 * The texture is rectanglular (doesn't have to be square), and the
 * number of pixels on each side must be a power of two. The number of
 * texels in the texture is (2^logres.u * 2^logres.v).
 *
 * All channels have the same data type and resolution, so the rect
 * array's length in bytes is:
 *     num_texels *
 *     texel_info.num_channels *
 *     data_type_size_in_bytes
 */
typedef struct MLoopPtex {
	/* TODO, shouldn't be in each loop? */
	struct Image *image;

	/* Channel data array */
	void *rect;

	MPtexLogRes logres;

	int pad1;

	/* TODO, shouldn't be in each loop */
	MPtexTexelInfo texel_info;

	int pad2;
} MLoopPtex;

typedef struct MTessFacePtex {
	float uv[4][2];
	int id;
} MTessFacePtex;

typedef struct FreestyleEdge {
	char flag;
	char pad[3];
} FreestyleEdge;

/* FreestyleEdge->flag */
enum {
	FREESTYLE_EDGE_MARK = 1,
};

typedef struct FreestyleFace {
	char flag;
	char pad[3];
} FreestyleFace;

/* FreestyleFace->flag */
enum {
	FREESTYLE_FACE_MARK = 1,
};

/* mvert->flag */
enum {
/*	SELECT              = (1 << 0), */
	ME_VERT_TMP_TAG     = (1 << 2),
	ME_HIDE             = (1 << 4),
	ME_VERT_MERGED      = (1 << 6),
	ME_VERT_PBVH_UPDATE = (1 << 7),
};

/* medge->flag */
enum {
/*	SELECT              = (1 << 0), */
	ME_EDGEDRAW         = (1 << 1),
	ME_SEAM             = (1 << 2),
/*	ME_HIDE             = (1 << 4), */
	ME_EDGERENDER       = (1 << 5),
	ME_LOOSEEDGE        = (1 << 7),
	ME_EDGE_TMP_TAG     = (1 << 8),
	ME_SHARP            = (1 << 9),  /* only reason this flag remains a 'short' */
};

/* puno = vertexnormal (mface) */
enum {
	ME_PROJXY = (1 << 4),
	ME_PROJXZ = (1 << 5),
	ME_PROJYZ = (1 << 6),
};

/* edcode (mface) */
enum {
	ME_V1V2 = (1 << 0),
	ME_V2V3 = (1 << 1),
	ME_V3V1 = (1 << 2),
	ME_V3V4 = ME_V3V1,
	ME_V4V1 = (1 << 3),
};

/* flag (mface) */
enum {
	ME_SMOOTH   = (1 << 0),
	ME_FACE_SEL = (1 << 1),
/*	ME_HIDE     = (1 << 4), */ 
};

#define ME_POLY_LOOP_PREV(mloop, mp, i)  (&(mloop)[(mp)->loopstart + (((i) + (mp)->totloop - 1) % (mp)->totloop)])
#define ME_POLY_LOOP_NEXT(mloop, mp, i)  (&(mloop)[(mp)->loopstart + (((i) + 1) % (mp)->totloop)])

/* mselect->type */
enum {
	ME_VSEL = 0,
	ME_ESEL = 1,
	ME_FSEL = 2,
};

/* mtface->flag */
enum {
	// TF_SELECT = (1 << 0),  /* use MFace hide flag (after 2.43), should be able to reuse after 2.44 */
	// TF_ACTIVE = (1 << 1),  /* deprecated! */
	TF_SEL1   = (1 << 2),
	TF_SEL2   = (1 << 3),
	TF_SEL3   = (1 << 4),
	TF_SEL4   = (1 << 5),
};

/* mtface->mode */
enum {
	TF_DYNAMIC    = (1 << 0),
	TF_ALPHASORT  = (1 << 1),
	TF_TEX        = (1 << 2),
	TF_SHAREDVERT = (1 << 3),
	TF_LIGHT      = (1 << 4),

	TF_CONVERTED  = (1 << 5),  /* tface converted to material */

	TF_SHAREDCOL  = (1 << 6),
	// TF_TILES      = (1 << 7),  /* deprecated */
	TF_BILLBOARD  = (1 << 8),
	TF_TWOSIDE    = (1 << 9),
	TF_INVISIBLE  = (1 << 10),

	TF_OBCOL      = (1 << 11),
	TF_BILLBOARD2 = (1 << 12),  /* with Z axis constraint */
	TF_SHADOW     = (1 << 13),
	TF_BMFONT     = (1 << 14),
};

/* mtface->transp, values 1-4 are used as flags in the GL, WARNING, TF_SUB cant work with this */
enum {
	TF_SOLID = 0,
	TF_ADD   = (1 << 0),
	TF_ALPHA = (1 << 1),
	TF_CLIP  = (1 << 2),  /* clipmap alpha/binary alpha all or nothing! */

	TF_SUB   = 3,  /* sub is not available in the user interface anymore */
};

/* mtface->unwrap */
enum {
	TF_DEPRECATED1 = (1 << 0),
	TF_DEPRECATED2 = (1 << 1),
	TF_DEPRECATED3 = (1 << 2),
	TF_DEPRECATED4 = (1 << 3),
	TF_PIN1        = (1 << 4),
	TF_PIN2        = (1 << 5),
	TF_PIN3        = (1 << 6),
	TF_PIN4        = (1 << 7),
};

#endif  /* __DNA_MESHDATA_TYPES_H__ */
