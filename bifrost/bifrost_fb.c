/* Copyright (c) 2013 FLIR Systems AB. All rights reserved. */
/*
 * Bifrost - FLIR Heimdall Driver, for GNU/Linux kernel 2.6.x
 *
 *  Created on: Mar 1, 2010
 *      Author: Jonas Romfelt <jonas.romfelt@flir.se>
 *
 * Frame buffer parts of driver.
 *
 * NOTE! FPGA only supports 32 bits ARGB but 8-bit CLUT has been kept in
 * the source code to aid if necessary to add support for this pixel-format.
 * The main reason for this is the risk of a performance bottlneck when
 * transferring the overlay graphics over the PCIe bus to the FPGA RAM.
 *
 */

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/errno.h>
#include <linux/string.h>
#include <linux/mm.h>
#include <linux/slab.h>
#include <linux/vmalloc.h>
#include <linux/delay.h>
#include <linux/interrupt.h>
#include <linux/proc_fs.h>
#include <linux/fb.h>
#include <linux/init.h>

#include "bifrost.h"

static struct fb_var_screeninfo bifrost_fb_var_default = {
  .xres = 720,
  .yres = 576,
  .xres_virtual = 1024,
  .yres_virtual = 0, /* automatically calculated by init */

  /* Default is 32 bit ARGB (4 byte, alpha 8@24, red 8@16, green 8@8, blue 8@0) */
  .bits_per_pixel = 32,
  .transp = {24, 8, 0},
  .red = {16, 8, 0},
  .green = {8, 8, 0},
  .blue = {0, 8, 0},

  .activate = FB_ACTIVATE_NOW,
  .height = -1,
  .width = -1,
  .pixclock = 20000,
  .left_margin = 64,
  .right_margin = 64,
  .upper_margin = 32,
  .lower_margin = 32,
  .hsync_len = 64,
  .vsync_len = 2,
  .vmode = FB_VMODE_NONINTERLACED,
};

static struct fb_fix_screeninfo bifrost_fb_fix = {
  .id = BIFROST_FRAMEBUFFER_NAME,
  .type = FB_TYPE_PACKED_PIXELS,
  /* no panning, wrapping or acceleration in hardware */
  .xpanstep = 0,
  .ypanstep = 0,
  .ywrapstep = 0,
  .accel = FB_ACCEL_NONE,
};


/**
 * Check new settings
 *
 * NOTE that no settings should be set to hardware in this function.
 */
static int bifrost_fb_check_var(struct fb_var_screeninfo *var, struct fb_info *info)
{
  u32 line_length;

  /* check that resolution is within limits */
  if (!var->xres)
    var->xres = 1;
  if (!var->yres)
    var->yres = 1;
  if (var->xres > var->xres_virtual)
    var->xres_virtual = var->xres;
  if (var->yres > var->yres_virtual)
    var->yres_virtual = var->yres;
  if (var->xres_virtual < var->xoffset + var->xres)
    var->xres_virtual = var->xoffset + var->xres;
  if (var->yres_virtual < var->yoffset + var->yres)
    var->yres_virtual = var->yoffset + var->yres;

  /* check memory limits */
  line_length = (var->xres_virtual * var->bits_per_pixel) >> 3;
  if ((line_length * var->yres_virtual) > bdev->overlay.size)
  {
    ALERT("Not enough memory to support: %d x %d x %d-bits\n",
        var->xres_virtual, var->yres_virtual, var->bits_per_pixel);
    return -ENOMEM;
  }

  /* check pixel format
   * NOTE at this moment we only support 32 bit
   *
   if (var->bits_per_pixel <= 8)
    var->bits_per_pixel = 8;
   else
  */
  var->bits_per_pixel = 32;

  switch (var->bits_per_pixel)
  {
    case 8: /* CLUT */
      var->red.offset = 0;
      var->red.length = 8;
      var->green.offset = 0;
      var->green.length = 8;
      var->blue.offset = 0;
      var->blue.length = 8;
      var->transp.offset = 0;
      var->transp.length = 0;
      break;
    case 32: /* RGBA 8888 */
      var->red.offset = 0;
      var->red.length = 8;
      var->green.offset = 8;
      var->green.length = 8;
      var->blue.offset = 16;
      var->blue.length = 8;
      var->transp.offset = 24;
      var->transp.length = 8;
      break;
  }
  var->red.msb_right = 0;
  var->green.msb_right = 0;
  var->blue.msb_right = 0;
  var->transp.msb_right = 0;

  INFO("Settings ok: %d x %d x %d-bit\n",
      var->xres_virtual, var->yres_virtual, var->bits_per_pixel);

  return 0;
}


/**
 * Set the video mode according to parameters.
 *
 * NOTE that settings have been checked in bifrost_fb_check_var() prior
 * call to this function.
 */
static int bifrost_fb_set_par(struct fb_info *info)
{
  struct fb_var_screeninfo *var = &info->var;
  struct fb_fix_screeninfo *fix = &info->fix;

  fix->line_length = (var->xres_virtual * var->bits_per_pixel) >> 3;
  fix->visual = (var->bits_per_pixel == 8) ? FB_VISUAL_PSEUDOCOLOR : FB_VISUAL_TRUECOLOR;

  return 0;
}


static int bifrost_fb_setcolreg(unsigned regno, unsigned red, unsigned green,
                                unsigned blue, unsigned transp, struct fb_info *info)
{
  if (regno >= 256)
    return 1;

  /* grayscale works only partially under directcolor:
   * grayscale = 0.30*R + 0.59*G + 0.11*B */
  if (info->var.grayscale)
    red = green = blue = (red * 77 + green * 151 + blue * 28) >> 8;

  /* Directcolor:
   *   var->{color}.offset contains start of bitfield
   *   var->{color}.length contains length of bitfield
   *   {hardwarespecific} contains width of RAMDAC
   *   cmap[X] is programmed to (X << red.offset) | (X << green.offset) | (X << blue.offset)
   *   RAMDAC[X] is programmed to (red, green, blue)
   *
   * Pseudocolor:
   *    var->{color}.offset is 0 unless the palette index takes less than
   *                        bits_per_pixel bits and is stored in the upper
   *                        bits of the pixel value
   *    var->{color}.length is set so that 1 << length is the number of available
   *                        palette entries
   *    cmap is not used
   *    RAMDAC[X] is programmed to (red, green, blue)
   *
   * Truecolor:
   *    does not use DAC. Usually 3 are present.
   *    var->{color}.offset contains start of bitfield
   *    var->{color}.length contains length of bitfield
   *    cmap is programmed to (red << red.offset) | (green << green.offset) |
   *                      (blue << blue.offset) | (transp << transp.offset)
   *    RAMDAC does not exist
   */
#define CNVT_TOHW(val,width) ((((val)<<(width))+0x7FFF-(val))>>16)
  switch (info->fix.visual)
  {
    case FB_VISUAL_TRUECOLOR:
    case FB_VISUAL_PSEUDOCOLOR:
      red = CNVT_TOHW(red, info->var.red.length);
      green = CNVT_TOHW(green, info->var.green.length);
      blue = CNVT_TOHW(blue, info->var.blue.length);
      transp = CNVT_TOHW(transp, info->var.transp.length);
      break;
    case FB_VISUAL_DIRECTCOLOR:
      red = CNVT_TOHW(red, 8); /* expect 8 bit DAC */
      green = CNVT_TOHW(green, 8);
      blue = CNVT_TOHW(blue, 8);
      transp = CNVT_TOHW(transp, 8);
      break;
  }
#undef CNVT_TOHW

  /* Truecolor has hardware independent palette */
  if (info->fix.visual == FB_VISUAL_TRUECOLOR)
  {
    u32 v;

    if (regno >= 16)
      return 1;

    v = (red << info->var.red.offset) | (green << info->var.green.offset) |
        (blue << info->var.blue.offset) | (transp << info->var.transp.offset);

    switch (info->var.bits_per_pixel)
    {
      case 8:
        break;
      case 32:
        ((u32 *) (info->pseudo_palette))[regno] = v;
        break;
    }
    return 0;
  }

  return 0;
}


/**
 * Driver specific mmap() to handle virtual frame buffer as well as when PCI device
 * is available.
 */
static int bifrost_fb_mmap(struct fb_info *info, struct vm_area_struct *vma)
{
  unsigned long start = vma->vm_start;
  unsigned long size = vma->vm_end - vma->vm_start;
  int rc;

  if (vma->vm_pgoff != 0)
  {
    INFO("Only offset 0 is supported, not %lx\n", vma->vm_pgoff);
    return -EINVAL;
  }

  /* we can't map more memory than the memory size provided by device */
  if (size > bdev->overlay.size)
  {
    INFO("Request to mmap() %ld bytes, but only %d is allowed\n", size, bdev->overlay.size);
    return -EINVAL;
  }

  /* set protection flags to not use caching */
  //  vma->vm_page_prot = pgprot_noncached(vma->vm_page_prot);

  /* Build page table to map physical memory to virtual memory. Shift physical
   * address using PAGE_SHIFT to get page frame number (pfn).
   *
   * NOTE use io_remap_* for portability, same as remap_pfn_range() on most systems */
  rc = io_remap_pfn_range(vma,
          start,
          virt_to_phys((void *)bdev->overlay.addr) >> PAGE_SHIFT,
          size,
          vma->vm_page_prot);

  if (rc)
  {
    ALERT("Re-mapping memory failed: %d\n", rc);
    return -EIO;
  }

  return 0;
}

/* blank display */
int bifrost_fb_blank(int blank, struct fb_info *info)
{
  if (blank > FB_BLANK_POWERDOWN)
    blank = FB_BLANK_POWERDOWN;

  INFO("blank=%d\n", blank);

  return 0;
}

/* wait for blit idle, optional */
int bifrost_fb_sync(struct fb_info *info)
{
  INFO("sync\n");

  return 0;
}

static struct fb_ops bifrost_fb_ops = {
  .fb_check_var = bifrost_fb_check_var,
  .fb_set_par = bifrost_fb_set_par,
  .fb_setcolreg = bifrost_fb_setcolreg,
  .fb_mmap = bifrost_fb_mmap,
  .fb_sync = bifrost_fb_sync,
  .fb_blank = bifrost_fb_blank,
  /*
  .fb_fillrect = sys_fillrect,
  .fb_copyarea = sys_copyarea,
  .fb_imageblit = sys_imageblit,

  .fb_read = NULL,
  .fb_write = NULL,
  .fb_pan_display = NULL,
  .fb_setcmap = NULL, / * set color registers in batch * /
  .fb_cursor = NULL,
  .fb_rotate = NULL,
  */
};

int bifrost_fb_init(struct bifrost_device *dev)
{
  struct fb_info *info;
  int rc = 0;

  info = framebuffer_alloc(sizeof(u32) * 256, NULL);
  if (!info)
  {
    ALERT("framebuffer_alloc() failed\n");
    goto err_fb_alloc;
  }

  info->screen_base = (char __iomem *)bdev->overlay.addr;
  info->fbops = &bifrost_fb_ops;
  info->var = bifrost_fb_var_default;

  bifrost_fb_fix.smem_start = (unsigned long)bdev->overlay.addr;
  bifrost_fb_fix.smem_len = bdev->overlay.size;
  bifrost_fb_fix.line_length = (info->var.xres_virtual * info->var.bits_per_pixel) >> 3;
  bifrost_fb_fix.visual = (info->var.bits_per_pixel == 8) ? FB_VISUAL_PSEUDOCOLOR : FB_VISUAL_TRUECOLOR;
  info->var.yres_virtual = bdev->overlay.size / bifrost_fb_fix.line_length;
  info->fix = bifrost_fb_fix;
  info->pseudo_palette = info->par;
  info->par = NULL;
  info->flags = FBINFO_FLAG_DEFAULT;

  rc = fb_alloc_cmap(&info->cmap, 256, 0);
  if (rc < 0)
  {
    ALERT("fb_alloc_cmap() failed: %d\n", rc);
    goto err_cmap;
  }

  rc = register_framebuffer(info);
  if (rc < 0)
  {
    ALERT("register_framebuffer() failed: %d\n", rc);
    goto err_reg_fb;
  }

  dev->fb_info = info;
  dev->fb_id = info->node;

  INFO("fb%d: frame buffer device, using %dK of video memory\n",
      dev->fb_id, bdev->overlay.size >> 10);

  return 0;

err_fb_alloc:
  fb_dealloc_cmap(&info->cmap);
err_cmap:
  framebuffer_release(info);
err_reg_fb:
  return rc;
}

void bifrost_fb_exit(struct bifrost_device *dev)
{
  if (dev->fb_info)
  {
    unregister_framebuffer(dev->fb_info);
    fb_dealloc_cmap(&dev->fb_info->cmap);
    framebuffer_release(dev->fb_info);
  }
}
