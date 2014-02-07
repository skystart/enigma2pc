/*
 * Copyright (C) 2000-2013 the xine project
 *
 * This file is part of xine, a free video player.
 *
 * xine is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * xine is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110, USA
 *
 *
 * xine version of video_out.h
 *
 * vo_frame    : frame containing yuv data and timing info,
 *               transferred between video_decoder and video_output
 *
 * vo_driver   : lowlevel, platform-specific video output code
 *
 * vo_port     : generic frame_handling code, uses
 *               a vo_driver for output
 */

#ifndef HAVE_VIDEO_OUT_H
#define HAVE_VIDEO_OUT_H

#ifdef __cplusplus
extern "C" {
#endif

#include <pthread.h>

#include <xine.h>
#include <xine/buffer.h>

#ifdef XINE_COMPILE
#  include <xine/plugin_catalog.h>
#endif

typedef struct vo_frame_s vo_frame_t;
typedef struct vo_driver_s vo_driver_t;
typedef struct video_driver_class_s video_driver_class_t;
typedef struct vo_overlay_s vo_overlay_t;
typedef struct video_overlay_manager_s video_overlay_manager_t;

/* public part, video drivers may add private fields
 *
 * Remember that adding new functions to this structure requires
 * adaption of the post plugin decoration layer. Be sure to look into
 * src/xine-engine/post.[ch].
 */
struct vo_frame_s {
  /*
   * member functions
   */

  /* Provide a copy of the frame's image in an image format already known to xine. data's member */
  /* have already been intialized to frame's content on entry, so it's usually only necessary to */
  /* change format and img_size. In case img is set, it will point to a memory block of suitable */
  /* size (size has been determined by a previous call with img == NULL). img content and img_size */
  /* must adhere to the specification of _x_get_current_frame_data(). */
  /* Currently this is needed for all image formats except XINE_IMGFMT_YV12 and XINE_IMGFMT_YUY2. */
  void (*proc_provide_standard_frame_data) (vo_frame_t *vo_img, xine_current_frame_data_t *data);

  /* Duplicate picture data and acceleration specific data of a frame. */
  /* if the image format isn't already known by Xine. Currently this is needed */
  /* For all image formats except XINE_IMGFMT_YV12 and XINE_IMGFMT_YUY2 */
  void (*proc_duplicate_frame_data) (vo_frame_t *vo_img, vo_frame_t *src);

  /* tell video driver to copy/convert the whole of this frame, may be NULL */
  /* at least one of proc_frame() and proc_slice() MUST set the variable proc_called to 1 */
  void (*proc_frame) (vo_frame_t *vo_img);

  /* tell video driver to copy/convert a slice of this frame, may be NULL */
  /* at least one of proc_frame() and proc_slice() MUST set the variable proc_called to 1 */
  void (*proc_slice) (vo_frame_t *vo_img, uint8_t **src);

  /* tell video driver that the decoder starts a new field */
  void (*field) (vo_frame_t *vo_img, int which_field);

  /* append this frame to the display queue,
     returns number of frames to skip if decoder is late */
  /* when the frame does not originate from a stream, it is legal to pass an anonymous stream */
  int (*draw) (vo_frame_t *vo_img, xine_stream_t *stream);

  /* lock frame as reference, must be paired with free.
   * most decoders/drivers do not need to call this function since
   * newly allocated frames are already locked once.
   */
  void (*lock) (vo_frame_t *vo_img);

  /* this frame is no longer used by the decoder, video driver, etc */
  void (*free) (vo_frame_t *vo_img);

  /* free memory/resources for this frame */
  void (*dispose) (vo_frame_t *vo_img);

  /*
   * public variables to decoders and vo drivers
   * changing anything here will require recompiling them both
   */
  int64_t                    pts;           /* presentation time stamp (1/90000 sec)        */
  int64_t                    vpts;          /* virtual pts, generated by metronom           */
  int                        bad_frame;     /* e.g. frame skipped or based on skipped frame */
  int                        duration;      /* frame length in time, in 1/90000 sec         */

  /* yv12 (planar)       base[0]: y,       base[1]: u,  base[2]: v  */
  /* yuy2 (interleaved)  base[0]: yuyv..., base[1]: --, base[2]: -- */
  uint8_t                   *base[3];
  int                        pitches[3];

  /* info that can be used for interlaced output (e.g. tv-out)      */
  int                        top_field_first;
  int                        repeat_first_field;
  /* note: progressive_frame is set wrong on many mpeg2 streams. for
   * that reason, this flag should be interpreted as a "hint".
   */
  int                        progressive_frame;
  int                        picture_coding_type;

  /* cropping to be done */
  int                        crop_left, crop_right, crop_top, crop_bottom;

  int                        lock_counter;
  pthread_mutex_t            mutex; /* protect access to lock_count */

  /* extra info coming from input or demuxers */
  extra_info_t              *extra_info;

  /* additional information to be able to duplicate frames:         */
  int                        width, height;
  double                     ratio;         /* aspect ratio  */
  int                        format;        /* IMGFMT_YV12 or IMGFMT_YUY2                     */

  int                        drawn;         /* used by decoder, frame has already been drawn */
  int                        flags;         /* remember the frame flags */
  int                        proc_called;   /* track use of proc_*() methods */

  /* Used to carry private data for accelerated plugins.*/
  void                      *accel_data;

  /* "backward" references to where this frame originates from */
  xine_video_port_t         *port;
  vo_driver_t               *driver;
  xine_stream_t             *stream;

  /* displacement for overlays */
  int                       overlay_offset_x, overlay_offset_y;

  /* pointer to the next frame in display order, used by some vo deint */
  struct vo_frame_s         *future_frame;

  /*
   * that part is used only by video_out.c for frame management
   * obs: changing anything here will require recompiling vo drivers
   */
  struct vo_frame_s         *next;

  int                        id; /* debugging - track this frame */
  int                        is_first;
};


/*
 * Remember that adding new functions to this structure requires
 * adaption of the post plugin decoration layer. Be sure to look into
 * src/xine-engine/post.[ch].
 */
struct xine_video_port_s {

  uint32_t (*get_capabilities) (xine_video_port_t *self); /* for constants see below */

  /* open display driver for video output */
  /* when you are not a full-blown stream, but still need to open the port
   * (e.g. you are a post plugin) it is legal to pass an anonymous stream */
  void (*open) (xine_video_port_t *self, xine_stream_t *stream);

  /*
   * get_frame - allocate an image buffer from display driver
   *
   * params : width      == width of video to display.
   *          height     == height of video to display.
   *          ratio      == aspect ration information
   *          format     == FOURCC descriptor of image format
   *          flags      == field/prediction flags
   */
  vo_frame_t* (*get_frame) (xine_video_port_t *self, uint32_t width,
			    uint32_t height, double ratio,
			    int format, int flags);

  /* create a new grab video frame */
  xine_grab_video_frame_t* (*new_grab_video_frame) (xine_video_port_t *self);

  /* retrieves the last displayed frame (useful for taking snapshots) */
  vo_frame_t* (*get_last_frame) (xine_video_port_t *self);

  /* overlay stuff */
  void (*enable_ovl) (xine_video_port_t *self, int ovl_enable);

  /* get overlay manager */
  video_overlay_manager_t* (*get_overlay_manager) (xine_video_port_t *self);

  /* flush video_out fifo */
  void (*flush) (xine_video_port_t *self);

  /* trigger immediate drawing */
  void (*trigger_drawing) (xine_video_port_t *self);

  /* Get/Set video property
   *
   * See VO_PROP_* bellow
   */
  int (*get_property) (xine_video_port_t *self, int property);
  int (*set_property) (xine_video_port_t *self, int property, int value);

  /* return true if port is opened for this stream, stream can be anonymous */
  int (*status) (xine_video_port_t *self, xine_stream_t *stream,
                 int *width, int *height, int64_t *img_duration);

  /* video driver is no longer used by decoder => close */
  /* when you are not a full-blown stream, but still need to close the port
   * (e.g. you are a post plugin) it is legal to pass an anonymous stream */
  void (*close) (xine_video_port_t *self, xine_stream_t *stream);

  /* called on xine exit */
  void (*exit) (xine_video_port_t *self);

  /* the driver in use */
  vo_driver_t *driver;

};

/* constants for the get/set property functions */
#define VO_PROP_INTERLACED            0
#define VO_PROP_ASPECT_RATIO          1
#define VO_PROP_HUE                   2
#define VO_PROP_SATURATION            3
#define VO_PROP_CONTRAST              4
#define VO_PROP_BRIGHTNESS            5
#define VO_PROP_COLORKEY              6
#define VO_PROP_AUTOPAINT_COLORKEY    7
#define VO_PROP_ZOOM_X                8
#define VO_PROP_PAN_SCAN              9
#define VO_PROP_TVMODE		      10
#define VO_PROP_MAX_NUM_FRAMES        11
#define VO_PROP_GAMMA		      12
#define VO_PROP_ZOOM_Y                13
#define VO_PROP_DISCARD_FRAMES        14 /* not used by drivers */
#define VO_PROP_WINDOW_WIDTH          15 /* read-only */
#define VO_PROP_WINDOW_HEIGHT         16 /* read-only */
#define VO_PROP_BUFS_IN_FIFO          17 /* read-only */
#define VO_PROP_NUM_STREAMS           18 /* read-only */
#define VO_PROP_OUTPUT_WIDTH          19 /* read-only */
#define VO_PROP_OUTPUT_HEIGHT         20 /* read-only */
#define VO_PROP_OUTPUT_XOFFSET        21 /* read-only */
#define VO_PROP_OUTPUT_YOFFSET        22 /* read-only */
#define VO_PROP_SHARPNESS             24
#define VO_PROP_NOISE_REDUCTION       25
#define VO_PROP_BUFS_TOTAL            26 /* read-only */
#define VO_PROP_BUFS_FREE             27 /* read-only */
#define VO_PROP_MAX_VIDEO_WIDTH       28 /* read-only */
#define VO_PROP_MAX_VIDEO_HEIGHT      29 /* read-only */
#define VO_PROP_LAST_PTS              30
#define VO_PROP_DEINTERLACE_SD        31
#define VO_PROP_DEINTERLACE_HD        32
#define VO_NUM_PROPERTIES             33

/* number of colors in the overlay palette. Currently limited to 256
   at most, because some alphablend functions use an 8-bit index into
   the palette. This should probably be classified as a bug. */
#define OVL_PALETTE_SIZE 256

#define OVL_MAX_OPACITY  0x0f

/* number of recent frames to keep in memory
   these frames are needed by some deinterlace algorithms
   FIXME: we need a method to flush the recent frames (new stream)
*/
#define VO_NUM_RECENT_FRAMES     2

/* get_frame flags */
#define VO_TOP_FIELD         1
#define VO_BOTTOM_FIELD      2
#define VO_BOTH_FIELDS       (VO_TOP_FIELD | VO_BOTTOM_FIELD)
#define VO_PAN_SCAN_FLAG     4
#define VO_INTERLACED_FLAG   8
#define VO_NEW_SEQUENCE_FLAG 16 /* set after MPEG2 Sequence Header Code (used by XvMC) */
#define VO_CHROMA_422        32 /* used by VDPAU, default is chroma_420 */
#define VO_STILL_IMAGE       64

/* ((mpeg_color_matrix << 1) | color_range) inside frame.flags bits 12-8 */
#define VO_FULLRANGE 0x100
#define VO_GET_FLAGS_CM(flags) ((flags >> 8) & 31)
#define VO_SET_FLAGS_CM(cm,flags) flags = ((flags) & ~0x1f00) | (((cm) & 31) << 8)

/* video driver capabilities */
#define VO_CAP_YV12                   0x00000001 /* driver can handle YUV 4:2:0 pictures */
#define VO_CAP_YUY2                   0x00000002 /* driver can handle YUY2 pictures */
#define VO_CAP_XVMC_MOCOMP            0x00000004 /* driver can use XvMC motion compensation */
#define VO_CAP_XVMC_IDCT              0x00000008 /* driver can use XvMC idct acceleration   */
#define VO_CAP_UNSCALED_OVERLAY       0x00000010 /* driver can blend overlay at output resolution */
#define VO_CAP_CROP                   0x00000020 /* driver can crop */
#define VO_CAP_XXMC                   0x00000040 /* driver can use extended XvMC */
#define VO_CAP_VDPAU_H264             0x00000080 /* driver can use VDPAU for H264 */
#define VO_CAP_VDPAU_MPEG12           0x00000100 /* driver can use VDPAU for mpeg1/2 */
#define VO_CAP_VDPAU_VC1              0x00000200 /* driver can use VDPAU for VC1 */
#define VO_CAP_VDPAU_MPEG4            0x00000400 /* driver can use VDPAU for mpeg4-part2 */
#define VO_CAP_VAAPI                  0x00000800 /* driver can use VAAPI */
#define VO_CAP_COLOR_MATRIX           0x00004000 /* driver can use alternative yuv->rgb matrices */
#define VO_CAP_FULLRANGE              0x00008000 /* driver handles fullrange yuv */
#define VO_CAP_HUE                    0x00010000
#define VO_CAP_SATURATION             0x00020000
#define VO_CAP_CONTRAST               0x00040000
#define VO_CAP_BRIGHTNESS             0x00080000
#define VO_CAP_COLORKEY               0x00100000
#define VO_CAP_AUTOPAINT_COLORKEY     0x00200000
#define VO_CAP_ZOOM_X                 0x00400000
#define VO_CAP_ZOOM_Y                 0x00800000
#define VO_CAP_CUSTOM_EXTENT_OVERLAY  0x01000000 /* driver can blend custom extent overlay to output extent */
#define VO_CAP_ARGB_LAYER_OVERLAY     0x02000000 /* driver supports true color overlay */
#define VO_CAP_VIDEO_WINDOW_OVERLAY   0x04000000 /* driver can scale video to an area within overlay */
#define VO_CAP_GAMMA                  0x08000000
#define VO_CAP_SHARPNESS              0x10000000
#define VO_CAP_NOISE_REDUCTION        0x20000000


/*
 * vo_driver_s contains the functions every display driver
 * has to implement. The vo_new_port function (see below)
 * should then be used to construct a vo_port using this
 * driver. Some of the function pointers will be copied
 * directly into xine_video_port_s, others will be called
 * from generic vo functions.
 */

#define VIDEO_OUT_DRIVER_IFACE_VERSION  22

struct vo_driver_s {

  uint32_t (*get_capabilities) (vo_driver_t *self); /* for constants see above */

  /*
   * allocate an vo_frame_t struct,
   * the driver must supply the copy, field and dispose functions
   */
  vo_frame_t* (*alloc_frame) (vo_driver_t *self);

  /*
   * check if the given image fullfills the format specified
   * (re-)allocate memory if necessary
   */
  void (*update_frame_format) (vo_driver_t *self, vo_frame_t *img,
			       uint32_t width, uint32_t height,
			       double ratio, int format, int flags);

  /* display a given frame */
  void (*display_frame) (vo_driver_t *self, vo_frame_t *vo_img);

  /* overlay_begin and overlay_end are used by drivers suporting
   * persistent overlays. they can be optimized to update only when
   * overlay image has changed.
   *
   * sequence of operation (pseudo-code):
   *   overlay_begin(this,img,true_if_something_changed_since_last_blend );
   *   while(visible_overlays)
   *     overlay_blend(this,img,overlay[i]);
   *   overlay_end(this,img);
   *
   * any function pointer from this group may be set to NULL.
   */
  void (*overlay_begin) (vo_driver_t *self, vo_frame_t *vo_img, int changed);
  void (*overlay_blend) (vo_driver_t *self, vo_frame_t *vo_img, vo_overlay_t *overlay);
  void (*overlay_end)   (vo_driver_t *self, vo_frame_t *vo_img);

  /*
   * these can be used by the gui directly:
   */
  int (*get_property) (vo_driver_t *self, int property);
  int (*set_property) (vo_driver_t *self,
		       int property, int value);
  void (*get_property_min_max) (vo_driver_t *self,
				int property, int *min, int *max);

  /*
   * general purpose communication channel between gui and driver
   *
   * this should be used to propagate events, display data, window sizes
   * etc. to the driver
   */
  int (*gui_data_exchange) (vo_driver_t *self, int data_type,
			    void *data);

  /* check if a redraw is needed (due to resize)
   * this is only used for still frames, normal video playback
   * must call that inside display_frame() function.
   */
  int (*redraw_needed) (vo_driver_t *self);

  /* Create a new grab video frame */
  xine_grab_video_frame_t* (*new_grab_video_frame)(vo_driver_t *self);

  /*
   * free all resources, close driver
   */
  void (*dispose) (vo_driver_t *self);

  /**
   * @brief Pointer to the loaded plugin node.
   *
   * Used by the plugins loader. It's an opaque type when using the
   * structure outside of xine's build.
   */
#ifdef XINE_COMPILE
  plugin_node_t *node;
#else
  void *node;
#endif
};

struct video_driver_class_s {

  /*
   * open a new instance of this plugin class
   */
  vo_driver_t* (*open_plugin) (video_driver_class_t *self, const void *visual);

  /**
   * @brief short human readable identifier for this plugin class
   */
  const char *identifier;

  /**
   * @brief human readable (verbose = 1 line) description for this plugin class
   *
   * The description is passed to gettext() to internationalise.
   */
  const char *description;

  /**
   * @brief Optional non-standard catalog to use with dgettext() for description.
   */
  const char *text_domain;

  /*
   * free all class-related resources
   */
  void (*dispose) (video_driver_class_t *self);
};

#define default_video_driver_class_dispose (void (*) (video_driver_class_t *this))free

typedef struct rle_elem_s {
  uint16_t len;
  uint16_t color;
} rle_elem_t;

typedef struct argb_layer_s {
  pthread_mutex_t  mutex;
  uint32_t        *buffer;
  /* dirty area */
  int x1, y1;
  int x2, y2;
  int ref_count;
} argb_layer_t;

struct vo_overlay_s {

  rle_elem_t       *rle;           /* rle code buffer                  */
  int               data_size;     /* useful for deciding realloc      */
  int               num_rle;       /* number of active rle codes       */
  int               x;             /* x start of subpicture area       */
  int               y;             /* y start of subpicture area       */
  int               width;         /* width of subpicture area         */
  int               height;        /* height of subpicture area        */

  /* area within osd extent to scale video to */
  int               video_window_x;
  int               video_window_y;
  int               video_window_width;
  int               video_window_height;

  /* extent of reference coordinate system */
  int               extent_width;
  int               extent_height;

  uint32_t          color[OVL_PALETTE_SIZE];  /* color lookup table     */
  uint8_t           trans[OVL_PALETTE_SIZE];  /* mixer key table        */
  int               rgb_clut;      /* true if clut was converted to rgb */

  /* define a highlight area with different colors */
  int               hili_top;
  int               hili_bottom;
  int               hili_left;
  int               hili_right;
  uint32_t          hili_color[OVL_PALETTE_SIZE];
  uint8_t           hili_trans[OVL_PALETTE_SIZE];
  int               hili_rgb_clut; /* true if clut was converted to rgb */

  int               unscaled;      /* true if it should be blended unscaled */

  argb_layer_t     *argb_layer;
};

void set_argb_layer_ptr(argb_layer_t **dst, argb_layer_t *src);

/* API to video_overlay manager
 *
 * Remember that adding new functions to this structure requires
 * adaption of the post plugin decoration layer. Be sure to look into
 * src/xine-engine/post.[ch].
 */
struct video_overlay_manager_s {
  void (*init) (video_overlay_manager_t *this_gen);

  void (*dispose) (video_overlay_manager_t *this_gen);

  int32_t (*get_handle) (video_overlay_manager_t *this_gen, int object_type );

  void (*free_handle) (video_overlay_manager_t *this_gen, int32_t handle);

  int32_t (*add_event) (video_overlay_manager_t *this_gen, void *event);

  void (*flush_events) (video_overlay_manager_t *this_gen );

  int (*redraw_needed) (video_overlay_manager_t *this_gen, int64_t vpts );

  void (*multiple_overlay_blend) (video_overlay_manager_t *this_gen, int64_t vpts,
                                  vo_driver_t *output, vo_frame_t *vo_img, int enabled);
};

/**
 * @brief Build a video output port from a given video driver.
 *
 * @internal
 */
xine_video_port_t *_x_vo_new_port (xine_t *xine, vo_driver_t *driver, int grabonly) XINE_MALLOC;

#ifdef __cplusplus
}
#endif

#endif

