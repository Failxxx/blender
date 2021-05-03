/*
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
 * The Original Code is Copyright (C) 2009 Blender Foundation, Joshua Leung
 * All rights reserved.
 */

#pragma once

/** \file
 * \ingroup bke
 */

#include "DNA_curve_types.h"

#ifdef __cplusplus
extern "C" {
#endif

struct ChannelDriver;
struct FCM_EnvelopeData;
struct FCurve;
struct FModifier;

struct AnimData;
struct AnimationEvalContext;
struct BezTriple;
struct BlendDataReader;
struct BlendExpander;
struct BlendLibReader;
struct BlendWriter;
struct LibraryForeachIDData;
struct PathResolvedRNA;
struct PointerRNA;
struct PropertyRNA;
struct StructRNA;
struct bAction;
struct bContext;

/* ************** Keyframe Tools ***************** */

typedef struct CfraElem {
  struct CfraElem *next, *prev;
  float cfra;
  int sel;
} CfraElem;

/* ************** F-Curve Modifiers *************** */

/* F-Curve Modifier Type-Info (fmi):
 *  This struct provides function pointers for runtime, so that functions can be
 *  written more generally (with fewer/no special exceptions for various modifiers).
 *
 *  Callers of these functions must check that they actually point to something useful,
 *  as some constraints don't define some of these.
 *
 *  Warning: it is not too advisable to reorder order of members of this struct,
 *           as you'll have to edit quite a few ($FMODIFIER_NUM_TYPES) of these
 *           structs.
 */
typedef struct FModifierTypeInfo {
  /* admin/ident */
  /** #FMODIFIER_TYPE_* */
  short type;
  /** size in bytes of the struct. */
  short size;
  /** #eFMI_Action_Types. */
  short acttype;
  /** #eFMI_Requirement_Flags. */
  short requires;
  /** name of modifier in interface. */
  char name[64];
  /** name of struct for SDNA. */
  char structName[64];
  /** Size of buffer that can be reused between time and value evaluation. */
  uint storage_size;

  /* data management function pointers - special handling */
  /** Free any data that is allocated separately (optional). */
  void (*free_data)(struct FModifier *fcm);
  /** Copy any special data that is allocated separately (optional). */
  void (*copy_data)(struct FModifier *fcm, const struct FModifier *src);
  /**
   * Set settings for data that will be used for FCuModifier.data
   * (memory already allocated using #MEM_callocN). */
  void (*new_data)(void *mdata);
  /** Verifies that the modifier settings are valid */
  void (*verify_data)(struct FModifier *fcm);

  /* evaluation */
  /** Evaluate time that the modifier requires the F-Curve to be evaluated at */
  float (*evaluate_modifier_time)(
      struct FCurve *fcu, struct FModifier *fcm, float cvalue, float evaltime, void *storage);
  /** Evaluate the modifier for the given time and 'accumulated' value */
  void (*evaluate_modifier)(
      struct FCurve *fcu, struct FModifier *fcm, float *cvalue, float evaltime, void *storage);
} FModifierTypeInfo;

/* Values which describe the behavior of a FModifier Type */
typedef enum eFMI_Action_Types {
  /* modifier only modifies values outside of data range */
  FMI_TYPE_EXTRAPOLATION = 0,
  /* modifier leaves data-points alone, but adjusts the interpolation between and around them */
  FMI_TYPE_INTERPOLATION,
  /* modifier only modifies the values of points (but times stay the same) */
  FMI_TYPE_REPLACE_VALUES,
  /* modifier generates a curve regardless of what came before */
  FMI_TYPE_GENERATE_CURVE,
} eFMI_Action_Types;

/* Flags for the requirements of a FModifier Type */
typedef enum eFMI_Requirement_Flags {
  /* modifier requires original data-points (kindof beats the purpose of a modifier stack?) */
  FMI_REQUIRES_ORIGINAL_DATA = (1 << 0),
  /* modifier doesn't require on any preceding data (i.e. it will generate a curve).
   * Use in conjunction with FMI_TYPE_GENRATE_CURVE
   */
  FMI_REQUIRES_NOTHING = (1 << 1),
  /* refer to modifier instance */
  FMI_REQUIRES_RUNTIME_CHECK = (1 << 2),
} eFMI_Requirement_Flags;

/* Function Prototypes for FModifierTypeInfo's */

/**
 * This function should always be used to get the appropriate type-info,
 * as it has checks which prevent segfaults in some weird cases.
 */
const FModifierTypeInfo *fmodifier_get_typeinfo(const struct FModifier *fcm);
/**
 * This function should be used for getting the appropriate type-info when only
 * a F-Curve modifier type is known.
 */
const FModifierTypeInfo *get_fmodifier_typeinfo(const int type);

/* ---------------------- */

/**
 * Add a new F-Curve Modifier to the given F-Curve of a certain type.
 */
struct FModifier *add_fmodifier(ListBase *modifiers, int type, struct FCurve *owner_fcu);
/**
 * Make a copy of the specified F-Modifier.
 */
struct FModifier *copy_fmodifier(const struct FModifier *src);
/**
 * Duplicate all of the F-Modifiers in the Modifier stacks.
 */
void copy_fmodifiers(ListBase *dst, const ListBase *src);
/**
 * Remove and free the given F-Modifier from the given stack.
 */
bool remove_fmodifier(ListBase *modifiers, struct FModifier *fcm);
/**
 * Remove all of a given F-Curve's modifiers.
 */
void free_fmodifiers(ListBase *modifiers);

/**
 * Find the active F-Modifier.
 */
struct FModifier *find_active_fmodifier(ListBase *modifiers);
/**
 * Set the active F-Modifier.
 */
void set_active_fmodifier(ListBase *modifiers, struct FModifier *fcm);

/**
 * Do we have any modifiers which match certain criteria.
 *
 * \param mtype: Type of modifier (if 0, doesn't matter).
 * \param acttype: Type of action to perform (if -1, doesn't matter).
 */
bool list_has_suitable_fmodifier(ListBase *modifiers, int mtype, short acttype);

typedef struct FModifiersStackStorage {
  uint modifier_count;
  uint size_per_modifier;
  void *buffer;
} FModifiersStackStorage;

uint evaluate_fmodifiers_storage_size_per_modifier(ListBase *modifiers);
/**
 * Evaluate time modifications imposed by some F-Curve Modifiers.
 *
 * - This step acts as an optimization to prevent the F-Curve stack being evaluated
 *   several times by modifiers requesting the time be modified, as the final result
 *   would have required using the modified time
 * - Modifiers only ever receive the unmodified time, as subsequent modifiers should be
 *   working on the 'global' result of the modified curve, not some localized segment,
 *   so \a evaltime gets set to whatever the last time-modifying modifier likes.
 * - We start from the end of the stack, as only the last one matters for now.
 *
 * \param fcu: Can be NULL.
 */
float evaluate_time_fmodifiers(FModifiersStackStorage *storage,
                               ListBase *modifiers,
                               struct FCurve *fcu,
                               float cvalue,
                               float evaltime);
/**
 * Evaluates the given set of F-Curve Modifiers using the given data
 * Should only be called after evaluate_time_fmodifiers() has been called.
 */
void evaluate_value_fmodifiers(FModifiersStackStorage *storage,
                               ListBase *modifiers,
                               struct FCurve *fcu,
                               float *cvalue,
                               float evaltime);

/**
 * Bake modifiers for given F-Curve to curve sample data, in the frame range defined
 * by start and end (inclusive).
 */
void fcurve_bake_modifiers(struct FCurve *fcu, int start, int end);

int BKE_fcm_envelope_find_index(struct FCM_EnvelopeData *array,
                                float frame,
                                int arraylen,
                                bool *r_exists);

/* ************** F-Curves API ******************** */

/* threshold for binary-searching keyframes - threshold here should be good enough for now,
 * but should become userpref */
#define BEZT_BINARYSEARCH_THRESH 0.01f /* was 0.00001, but giving errors */

/* -------- Data Management  -------- */
struct FCurve *BKE_fcurve_create(void);
/**
 * Frees the F-Curve itself too, so make sure #BLI_remlink is called before calling this.
 */
void BKE_fcurve_free(struct FCurve *fcu);
/**
 * Duplicate a F-Curve.
 */
struct FCurve *BKE_fcurve_copy(const struct FCurve *fcu);
/**
 * Frees a list of F-Curves.
 */
void BKE_fcurves_free(ListBase *list);
/**
 * Duplicate a list of F-Curves.
 */
void BKE_fcurves_copy(ListBase *dst, ListBase *src);

/**
 * Callback used by lib_query to walk over all ID usages
 * (mimics `foreach_id` callback of #IDTypeInfo structure).
 */
void BKE_fcurve_foreach_id(struct FCurve *fcu, struct LibraryForeachIDData *data);

/**
 * Find the F-Curve affecting the given RNA-access path + index,
 * in the list of F-Curves provided.
 */
struct FCurve *BKE_fcurve_find(ListBase *list, const char rna_path[], const int array_index);

/**
 * Quick way to loop over all f-curves of a given 'path'.
 */
struct FCurve *BKE_fcurve_iter_step(struct FCurve *fcu_iter, const char rna_path[]);

/**
 * High level function to get an f-curve from C without having the RNA.
 */
struct FCurve *id_data_find_fcurve(
    ID *id, void *data, struct StructRNA *type, const char *prop_name, int index, bool *r_driven);

/**
 * Get list of LinkData's containing pointers to the F-curves
 * which control the types of data indicated.
 * e.g. `numMatches = BKE_fcurves_filter(matches, &act->curves, "pose.bones[", "MyFancyBone");`
 *
 * Lists:
 * \param dst: list of LinkData's matching the criteria returned.
 * List must be freed after use, and is assumed to be empty when passed.
 * \param src: list of F-Curves to search through
 * Filters:
 * \param dataPrefix: i.e. `pose.bones[` or `nodes[`.
 * \param dataName: name of entity within "" immediately following the prefix.
 */
int BKE_fcurves_filter(ListBase *dst, ListBase *src, const char *dataPrefix, const char *dataName);

/**
 * Find an f-curve based on an rna property.
 */
struct FCurve *BKE_fcurve_find_by_rna(struct PointerRNA *ptr,
                                      struct PropertyRNA *prop,
                                      int rnaindex,
                                      struct AnimData **r_adt,
                                      struct bAction **r_action,
                                      bool *r_driven,
                                      bool *r_special);
/**
 * Same as above, but takes a context data,
 * temp hack needed for complex paths like texture ones.
 */
struct FCurve *BKE_fcurve_find_by_rna_context_ui(struct bContext *C,
                                                 struct PointerRNA *ptr,
                                                 struct PropertyRNA *prop,
                                                 int rnaindex,
                                                 struct AnimData **r_animdata,
                                                 struct bAction **r_action,
                                                 bool *r_driven,
                                                 bool *r_special);

/**
 * Binary search algorithm for finding where to 'insert' #BezTriple with given frame number.
 * Returns the index to insert at (data already at that index will be offset if replace is 0)
 */
int BKE_fcurve_bezt_binarysearch_index(const struct BezTriple array[],
                                       const float frame,
                                       const int arraylen,
                                       bool *r_replace);

/* fcurve_cache.c */

/**
 * Cached f-curve look-ups, use when this needs to be done many times.
 */
struct FCurvePathCache;
struct FCurvePathCache *BKE_fcurve_pathcache_create(ListBase *list);
void BKE_fcurve_pathcache_destroy(struct FCurvePathCache *fcache);
struct FCurve *BKE_fcurve_pathcache_find(struct FCurvePathCache *fcache,
                                         const char rna_path[],
                                         const int array_index);
/**
 * Fill in an array of F-Curve, leave NULL when not found.
 *
 * \return The number of F-Curves found.
 */
int BKE_fcurve_pathcache_find_array(struct FCurvePathCache *fcache,
                                    const char *rna_path,
                                    struct FCurve **fcurve_result,
                                    int fcurve_result_len);

/**
 * Calculate the extents of F-Curve's keyframes.
 */
bool BKE_fcurve_calc_range(
    struct FCurve *fcu, float *min, float *max, const bool do_sel_only, const bool do_min_length);

/**
 * Calculate the extents of F-Curve's data.
 */
bool BKE_fcurve_calc_bounds(struct FCurve *fcu,
                            float *xmin,
                            float *xmax,
                            float *ymin,
                            float *ymax,
                            const bool do_sel_only,
                            const bool include_handles);

/**
 * Return an array of keyed frames, rounded to `interval`.
 *
 * \param interval: Set to 1.0 to round to whole keyframes, 0.5 for in-between key-frames, etc.
 *
 * \note An interval of zero could be supported (this implies no rounding at all),
 * however this risks very small differences in float values being treated as separate keyframes.
 */
float *BKE_fcurves_calc_keyed_frames_ex(struct FCurve **fcurve_array,
                                        const int fcurve_array_len,
                                        const float interval,
                                        int *r_frames_len);
float *BKE_fcurves_calc_keyed_frames(struct FCurve **fcurve_array,
                                     const int fcurve_array_len,
                                     int *r_frames_len);

/**
 * Set the index that stores the FCurve's active keyframe, assuming that \a active_bezt
 * is already part of `fcu->bezt`. If NULL, set active keyframe index to "none."
 */
void BKE_fcurve_active_keyframe_set(struct FCurve *fcu, const struct BezTriple *active_bezt);
/**
 * Get the active keyframe index, with sanity checks for point bounds.
 */
int BKE_fcurve_active_keyframe_index(const struct FCurve *fcu);

/**
 * Move the indexed keyframe to the given value,
 * and move the handles with it to ensure the slope remains the same.
 */
void BKE_fcurve_keyframe_move_value_with_handles(struct BezTriple *keyframe, float new_value);

/* .............. */

/**
 * Are keyframes on F-Curve of any use (to final result, and to show in editors)?
 * Usability of keyframes refers to whether they should be displayed,
 * and also whether they will have any influence on the final result.
 */
bool BKE_fcurve_are_keyframes_usable(struct FCurve *fcu);

/**
 * Can keyframes be added to F-Curve?
 * Keyframes can only be added if they are already visible.
 */
bool BKE_fcurve_is_keyframable(struct FCurve *fcu);
bool BKE_fcurve_is_protected(struct FCurve *fcu);

/**
 * Checks if the F-Curve has a Cycles modifier with simple settings
 * that warrant transition smoothing.
 */
bool BKE_fcurve_is_cyclic(struct FCurve *fcu);

/* Type of infinite cycle for a curve. */
typedef enum eFCU_Cycle_Type {
  FCU_CYCLE_NONE = 0,
  /* The cycle repeats identically to the base range. */
  FCU_CYCLE_PERFECT,
  /* The cycle accumulates the change between start and end keys. */
  FCU_CYCLE_OFFSET,
} eFCU_Cycle_Type;

/**
 * Checks if the F-Curve has a Cycles modifier, and returns the type of the cycle behavior.
 */
eFCU_Cycle_Type BKE_fcurve_get_cycle_type(const struct FCurve *fcu);

/**
 * Recompute bezier handles of all three given BezTriples, so that `bezt` can be inserted between
 * `prev` and `next` without changing the resulting curve shape.
 *
 * \param r_pdelta: return Y difference between `bezt` and the original curve value at its X
 * position.
 * \return Whether the split was successful.
 */
bool BKE_fcurve_bezt_subdivide_handles(struct BezTriple *bezt,
                                       struct BezTriple *prev,
                                       struct BezTriple *next,
                                       float *r_pdelta);

/* -------- Curve Sanity -------- */

/**
 * This function recalculates the handles of an F-Curve. Acts based on selection with `SELECT`
 * flag. To use a different flag, use #calchandles_fcurve_ex().
 *
 * If the BezTriples have been rearranged, sort them first before using this.
 */
void calchandles_fcurve(struct FCurve *fcu);
/**
 * Variant of #calchandles_fcurve() that allows calculating based on a different select flag.
 *
 * \param handle_sel_flag: The flag (bezt.f1/2/3) value to use to determine selection.
 * Usually `SELECT`, but may want to use a different one at times
 * (if caller does not operate on selection).
 */
void calchandles_fcurve_ex(struct FCurve *fcu, eBezTriple_Flag handle_sel_flag);
/**
 * Update handles, making sure the handle-types are valid (e.g. correctly deduced from an "Auto"
 * type), and recalculating their position vectors.
 * Use when something has changed handle positions.
 *
 * \param sel_flag: The flag (bezt.f1/2/3) value to use to determine selection. Usually `SELECT`,
 * but may want to use a different one at times (if caller does not operate on selection).
 * \param use_handle: Check selection state of individual handles, otherwise always update both
 * handles if the key is selected.
 */
void testhandles_fcurve(struct FCurve *fcu, eBezTriple_Flag sel_flag, const bool use_handle);
/**
 * This function sorts BezTriples so that they are arranged in chronological order,
 * as tools working on F-Curves expect that the BezTriples are in order.
 */
void sort_time_fcurve(struct FCurve *fcu);
/**
 * This function tests if any BezTriples are out of order, thus requiring a sort.
 */
bool test_time_fcurve(struct FCurve *fcu);

/**
 * The length of each handle is not allowed to be more
 * than the horizontal distance between (v1-v4).
 * This is to prevent curve loops.
 *
 * This function is very similar to BKE_curve_correct_bezpart(), but allows a steeper tangent for
 * more snappy animations. This is not desired for other areas in which curves are used, though.
 */
void BKE_fcurve_correct_bezpart(const float v1[2], float v2[2], float v3[2], const float v4[2]);

/* -------- Evaluation -------- */

/* evaluate fcurve */
float evaluate_fcurve(struct FCurve *fcu, float evaltime);
float evaluate_fcurve_only_curve(struct FCurve *fcu, float evaltime);
float evaluate_fcurve_driver(struct PathResolvedRNA *anim_rna,
                             struct FCurve *fcu,
                             struct ChannelDriver *driver_orig,
                             const struct AnimationEvalContext *anim_eval_context);
/**
 * Checks if the curve has valid keys, drivers or modifiers that produce an actual curve.
 */
bool BKE_fcurve_is_empty(struct FCurve *fcu);
/**
 * Calculate the value of the given F-Curve at the given frame,
 * and store it's value in #FCurve.curval.
 */
float calculate_fcurve(struct PathResolvedRNA *anim_rna,
                       struct FCurve *fcu,
                       const struct AnimationEvalContext *anim_eval_context);

/* ************* F-Curve Samples API ******************** */

/* -------- Defines -------- */

/**
 * Basic signature for F-Curve sample-creation function.
 *
 * \param fcu: the F-Curve being operated on.
 * \param data: pointer to some specific data that may be used by one of the callbacks.
 */
typedef float (*FcuSampleFunc)(struct FCurve *fcu, void *data, float evaltime);

/* ----- Sampling Callbacks ------ */

/**
 * Basic sampling callback which acts as a wrapper for #evaluate_fcurve()
 * 'data' arg here is unneeded here.
 */
float fcurve_samplingcb_evalcurve(struct FCurve *fcu, void *data, float evaltime);

/* -------- Main Methods -------- */

/**
 * Main API function for creating a set of sampled curve data, given some callback function
 * used to retrieve the values to store.
 */
void fcurve_store_samples(
    struct FCurve *fcu, void *data, int start, int end, FcuSampleFunc sample_cb);

/**
 * Convert baked/sampled f-curves into bezt/regular f-curves.
 */
void fcurve_samples_to_keyframes(struct FCurve *fcu, const int start, const int end);

/* ************* F-Curve .blend file API ******************** */

void BKE_fmodifiers_blend_write(struct BlendWriter *writer, struct ListBase *fmodifiers);
void BKE_fmodifiers_blend_read_data(struct BlendDataReader *reader,
                                    ListBase *fmodifiers,
                                    struct FCurve *curve);
void BKE_fmodifiers_blend_read_lib(struct BlendLibReader *reader,
                                   struct ID *id,
                                   struct ListBase *fmodifiers);
void BKE_fmodifiers_blend_read_expand(struct BlendExpander *expander, struct ListBase *fmodifiers);

void BKE_fcurve_blend_write(struct BlendWriter *writer, struct ListBase *fcurves);
void BKE_fcurve_blend_read_data(struct BlendDataReader *reader, struct ListBase *fcurves);
void BKE_fcurve_blend_read_lib(struct BlendLibReader *reader,
                               struct ID *id,
                               struct ListBase *fcurves);
void BKE_fcurve_blend_read_expand(struct BlendExpander *expander, struct ListBase *fcurves);

#ifdef __cplusplus
}
#endif
