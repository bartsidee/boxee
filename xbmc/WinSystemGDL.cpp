/*
 *      Copyright (C) 2005-2008 Team XBMC
 *      http://www.xbmc.org
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with XBMC; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#include "system.h"

#ifdef HAS_GDL

#define DEFAULT_OVERSCAN 0.03f

#include "WinSystemGDL.h"
#include "log.h"
#include "Settings.h"
#include "IntelSMDGlobals.h"
#include "GraphicContext.h"
#include "GUISettings.h"
#include "cores/AudioRenderers/IntelSMDAudioRenderer.h"


#ifdef HAS_BOXEE_HAL
#include "HalServices.h"
#include "HalListenerImpl.h"
#endif


//------------------------------------------------------------------------------
// __yes_no()
//------------------------------------------------------------------------------
static void __yes_no(const char * s, gdl_boolean_t cond)
{
    CLog::Log(LOGINFO, "%s %s\n", s, cond ? "yes" : "no");
}

static gdl_boolean_t __sample_size_available(gdl_hdmi_audio_fmt_t fmt)
{
  return (gdl_boolean_t)(fmt == GDL_HDMI_AUDIO_FORMAT_PCM);
}

static gdl_boolean_t __bitrate_available(gdl_hdmi_audio_fmt_t fmt)
{
  return (gdl_boolean_t)(fmt >= GDL_HDMI_AUDIO_FORMAT_AC3 && fmt <= GDL_HDMI_AUDIO_FORMAT_ATRAC);
}

//------------------------------------------------------------------------------
// query_generic_info()
//------------------------------------------------------------------------------
void query_generic_info(gdl_pd_id_t port)
{
    gdl_pd_recv_t cmd = GDL_PD_RECV_HDMI_SINK_INFO;
    gdl_hdmi_sink_info_t si;
    int i, map;

    if (gdl_port_recv(port, cmd, &si, sizeof(si)) == GDL_SUCCESS)
    {
      CLog::Log(LOGINFO, " - Manufacturer ID        : 0x%.4x\n", si.manufac_id  );
      CLog::Log(LOGINFO, " - Product code           : 0x%.4x\n", si.product_code);

        __yes_no(" - HDMI                   :", si.hdmi       );
        __yes_no(" - YCbCr444               :", si.ycbcr444   );
        __yes_no(" - YCbCr422               :", si.ycbcr422   );
        __yes_no(" - 30 bit color           :", si.dc_30      );
        __yes_no(" - 36 bit color           :", si.dc_36      );
        __yes_no(" - YCbCr444 in deep color :", si.dc_y444    );
        __yes_no(" - xvycc601 colorimetry   :", si.xvycc601   );
        __yes_no(" - xvycc709 colorimetry   :", si.xvycc709   );
        __yes_no(" - Supports AI            :", si.supports_ai);

        CLog::Log(LOGINFO, " - Max TMDS clock         : %d\n", si.max_tmds_clock);

        __yes_no(" - Latency valid          :", si.latency_present);
        __yes_no(" - Latency int valid      :", si.latency_int_present);
        if(si.latency_present)
        {
          CLog::Log(LOGINFO, " - Video latency          : %d\n", si.latency_video);
          CLog::Log(LOGINFO, " - Audio latency          : %d\n", si.latency_audio);
        }
        if(si.latency_int_present)
        {
          CLog::Log(LOGINFO, " - Video latency int      : %d\n", si.latency_video_interlaced);
          CLog::Log(LOGINFO, " - Audio latency int      : %d\n", si.latency_audio_interlaced);
        }
        if(si.hdmi_video_present)
        {
            __yes_no(" - hdmi_video             :", si.hdmi_video_present);
            __yes_no(" - hdmi_video 3D          :", si.enabled_3d);
            CLog::Log(LOGINFO, " - Number of 3D modes     : %d\n", si.num_modes_3d);
        }

        CLog::Log(LOGINFO, " - Source Physical Address: %d.%d.%d.%d\n",
               si.spa.a, si.spa.b, si.spa.c, si.spa.d       );

        CLog::Log(LOGINFO, " - Speaker map            :");
        map = si.speaker_map;
        for (i = 0; i < 11; i++)
        {
            //printf(" %s", (map & (1<<i)) ? gdl_dbg_string_speaker_map(1<<i) : "");
        }
        CLog::Log(LOGINFO, "\n");
    }
}

//------------------------------------------------------------------------------
// query_hdcp_info()
//------------------------------------------------------------------------------
void query_hdcp_info(gdl_pd_id_t port)
{
    gdl_pd_recv_t cmd = GDL_PD_RECV_HDMI_HDCP_INFO;
    gdl_hdmi_hdcp_info_t hi;

    if (gdl_port_recv(port, cmd, &hi, sizeof(hi)) == GDL_SUCCESS)
    {
        __yes_no(" - HDCP support           :", GDL_TRUE   );
        __yes_no(" - HDCP 1.1               :", hi.hdcp_1p1);
        __yes_no(" - Repeater               :", hi.repeater);

        // Dump topology information if connected sink is a repeater
        if (hi.repeater)
        {
          CLog::Log(LOGINFO, "   . devices connected    : %d\n", hi.device_count        );
          CLog::Log(LOGINFO, "   . depth                : %d\n", hi.depth               );
            __yes_no("   . max devs  exceeded   :"     , hi.max_devs_exceeded   );
            __yes_no("   . max depth exceeded   :"     , hi.max_cascade_exceeded);
        }

        // Get and print KSVs
        cmd = GDL_PD_RECV_HDMI_HDCP_KSVS;
        /*
        unsigned char ksvs[635];
        if (gdl_port_recv(port, cmd, ksvs, sizeof(ksvs)) == GDL_SUCCESS)
        {
          CLog::Log(LOGINFO, " - List of KSVs:\n");
            for (int i = 0; i < hi.device_count + 1; i++)
            {
              CLog::Log(LOGINFO, "   . %2x %2x %2x %2x %2x\n",
                       ksvs[i * 5 + 0],
                       ksvs[i * 5 + 1],
                       ksvs[i * 5 + 2],
                       ksvs[i * 5 + 3],
                       ksvs[i * 5 + 4]);
            }
        }
        else
        {
          CLog::Log(LOGINFO, " - KSV information unavailable\n");
        }
        */

        // Print HDCP debug information
        /*
        CLog::Log(LOGINFO, "\nHDCP debug information\n");
        CLog::Log(LOGINFO, " - AKSV = %2x %2x %2x %2x %2x\n",
               hi.aksv[0], hi.aksv[1], hi.aksv[2], hi.aksv[3], hi.aksv[4]);
        CLog::Log(LOGINFO, " - BKSV = %2x %2x %2x %2x %2x\n",
               hi.bksv[0], hi.bksv[1], hi.bksv[2], hi.bksv[3], hi.bksv[4]);
        CLog::Log(LOGINFO, " - AN   = 0x%llx\n", hi.an);
        CLog::Log(LOGINFO, " - Ri   = %04x  Ri_prime = %04x\n", hi.ri, hi.ri_prime);
        if (hi.hdcp_1p1)
        {
          CLog::Log(LOGINFO, " - Pj   = %04x  Pj_prime = %04x\n", hi.pj, hi.pj_prime);
        }
        */

    }
    else
    {
        __yes_no(" - HDCP support           :", GDL_FALSE  );
    }
}

void query_audio_info(gdl_pd_id_t port)
{
    gdl_pd_recv_t cmd  = GDL_PD_RECV_HDMI_AUDIO_CTRL;
    gdl_hdmi_audio_ctrl_t ctrl;
    unsigned int rate, i, size;

    // Init query data
    ctrl.cmd_id = GDL_HDMI_AUDIO_GET_CAPS;
    ctrl.data._get_caps.index = 0;

    // Get audio capabilities one after another
    while (gdl_port_recv(port, cmd, &ctrl, sizeof(ctrl)) == GDL_SUCCESS)
    {
        printf(" - Format: %s; Max channels: %d; ",
               gdl_dbg_string_audio_format((gdl_hdmi_audio_fmt_t)ctrl.data._get_caps.cap.format),
               ctrl.data._get_caps.cap.max_channels);

        rate = ctrl.data._get_caps.cap.fs;
        size = ctrl.data._get_caps.cap.ss_bitrate;

        printf("Sampling rates, KHz: ");
        for (i = 0; i < 7; i++)
        {

            printf("%s", (rate & (1<<i)) ? " " : "");
            printf("%s", (rate & (1<<i)) ? gdl_dbg_string_sampling_rate(gdl_hdmi_audio_fs_t(1<<i)) : "");
        }

        if (__sample_size_available((gdl_hdmi_audio_fmt_t)ctrl.data._get_caps.cap.format))
        {
            printf("; Sample size, bits:");
            for (i = 0; i < 3; i++)
            {
                printf("%s", (size & (1<<i)) ? " " : "");
                printf("%s", (size & (1<<i)) ? gdl_dbg_string_sample_size((gdl_hdmi_audio_ss_t)(1<<i)) : "");
            }
        }

        else
        if (__bitrate_available((gdl_hdmi_audio_fmt_t)ctrl.data._get_caps.cap.format))
        {
            printf("; Max bitrate, kbps: %d", size * 8);
        }

        printf("\n");

        ctrl.data._get_caps.index++;
    }
}

CWinSystemGDL::CWinSystemGDL() : CWinSystemBase()
{
  m_eWindowSystem = WINDOW_SYSTEM_GDL;
  m_gdlPlane = GDL_PLANE_ID_UNDEFINED;
}

CWinSystemGDL::~CWinSystemGDL()
{
  DestroyWindowSystem();
}

bool CWinSystemGDL::InitWindowSystem()
{
  gdl_ret_t ret = GDL_ERR_FAILED;
  int counter = 0;

  while (counter < 5)
  {
    ret = gdl_init(0);
    if (ret != GDL_SUCCESS)
    {
      CLog::Log(LOGINFO, "CWinSystemGDL::InitWindowSystem gdl_init failed");
      counter++;
      sleep(1);
    }
    else
      break;
  }

  if(ret != GDL_SUCCESS)
    return false;

  if (!CWinSystemBase::InitWindowSystem())
    return false;

  return true;
}

bool CWinSystemGDL::DestroyWindowSystem()
{
  gdl_close();

  return true;
}

bool CWinSystemGDL::EnableHDCP(bool enable)
{
  gdl_boolean_t hdcpControlEnabled = enable ? GDL_TRUE : GDL_FALSE;

  if (gdl_port_set_attr(GDL_PD_ID_HDMI, GDL_PD_ATTR_ID_HDCP,
      &hdcpControlEnabled) != GDL_SUCCESS)
  {
    CLog::Log(LOGINFO, "Could not enable HDCP control");
    return false;
  }

  return true;
}

void* CWinSystemGDL::GetRenderContext()
{
  return m_eglBinding.GetContext();
}

bool CWinSystemGDL::CreateNewWindow(const CStdString& name, bool fullScreen, RESOLUTION_INFO& res, PHANDLE_EVENT_FUNC userFunction)
{
  gdl_display_info_t  di;

  CSingleLock lock(CIntelSMDAudioRenderer::m_SMDAudioLock);

  gdl_pixel_format_t pixelFormat = GDL_PF_ARGB_32;
  gdl_color_space_t colorSpace = GDL_COLOR_SPACE_RGB;
  gdl_rectangle_t srcRect;
  gdl_rectangle_t dstRect;
  gdl_ret_t rc = GDL_SUCCESS;

  memset(&di, 0, sizeof (gdl_display_info_t));

  m_Res = res;
  m_gdlPlane = GDL_GRAPHICS_PLANE;

  CLog::Log(LOGINFO, "WinSystemGDL creating new window: Width = %d  Height = %d refresh %f",
      res.iWidth, res.iHeight, m_Res.fRefreshRate);

  // print overscan values
  int left = res.Overscan.left;
  int top = res.Overscan.top;
  int right = res.Overscan.right;
  int bottom = res.Overscan.bottom;

  CLog::Log(LOGINFO, "WinSystemGDL overscan values left %d top %d right %d bottom %d", left, top, right, bottom);

  // Setup composite output to NTSC. In order to support PAL we need to use 720x576i50.
  di.id           = GDL_DISPLAY_ID_1; // composite video
  di.flags        = 0;
  di.bg_color     = 0;
  di.color_space  = GDL_COLOR_SPACE_BT601;
  di.gamma        = GDL_GAMMA_LINEAR;
  di.tvmode.interlaced = GDL_TRUE;
#if GDL_HEADER_VERSION_MAJOR >= 79
  di.tvmode.stereo_type = GDL_STEREO_NONE;
#endif

  // Set the video output format according to HAL
  int videoOutput = RES_NTSC_4x3;

#ifdef HAS_BOXEE_HAL
  CHalSoftwareInfo softwareInfo;
  CHalServicesFactory::GetInstance().GetSoftwareInfo(softwareInfo);
  if(softwareInfo.compositeVideo.compare("PAL") == 0)
    videoOutput = RES_PAL_4x3;
  else if(softwareInfo.compositeVideo.compare("NTSC") == 0)
    videoOutput = RES_NTSC_4x3;
#endif

  switch (videoOutput)
  {
  case RES_PAL_4x3:
    di.tvmode.width = 720;
    di.tvmode.height = 576;
    di.tvmode.refresh = GDL_REFRESH_50;
    break;
  case RES_NTSC_4x3:
    di.tvmode.width = 720;
    di.tvmode.height = 480;
    di.tvmode.refresh = GDL_REFRESH_59_94;
    break;
  }
  g_IntelSMDGlobals.DisableAudioOutput(g_IntelSMDGlobals.GetHDMIOutput());
  rc = gdl_set_display_info(&di);
  g_IntelSMDGlobals.EnableAudioOutput(g_IntelSMDGlobals.GetHDMIOutput());

  if(rc != GDL_SUCCESS)
  {
    CLog::Log(LOGINFO, "Could not set display mode for display 1");
  }

  if(!ConfigureHDMIOutput())
  {
    CLog::Log(LOGERROR, "WinSystemGDL ConfigureHDMIOutput failed");
    return false;
  }

  m_nWidth = res.iWidth;
  m_nHeight = res.iHeight;
  m_bFullScreen = fullScreen;

  dstRect.origin.x = 0;
  dstRect.origin.y = 0;
  dstRect.width = res.iWidth;
  dstRect.height = res.iHeight;

  srcRect.origin.x = 0;
  srcRect.origin.y = 0;
  srcRect.width = res.iWidth;
  srcRect.height = res.iHeight;

  if (GDL_SUCCESS == rc)
  {
    rc = gdl_plane_config_begin(m_gdlPlane);
  }

  rc = gdl_plane_reset(m_gdlPlane);
  if (GDL_SUCCESS == rc)
  {
    rc = gdl_plane_config_begin(m_gdlPlane);
  }

  if (GDL_SUCCESS == rc)
  {
    rc = gdl_plane_set_attr(GDL_PLANE_SRC_COLOR_SPACE, &colorSpace);
  }

  if (GDL_SUCCESS == rc)
  {
    rc = gdl_plane_set_attr(GDL_PLANE_PIXEL_FORMAT, &pixelFormat);
  }

  if (GDL_SUCCESS == rc)
  {
    rc = gdl_plane_set_attr(GDL_PLANE_DST_RECT, &dstRect);
  }

  if (GDL_SUCCESS == rc)
  {
    rc = gdl_plane_set_attr(GDL_PLANE_SRC_RECT, &srcRect);
  }

  if(GDL_SUCCESS == rc)
  {
    gdl_boolean_t scalineEnabled = GDL_FALSE;
    rc = gdl_plane_set_attr(GDL_PLANE_SCALE, &scalineEnabled);
  }

  if (GDL_SUCCESS == rc)
  {
    rc = gdl_plane_config_end(GDL_FALSE);
  }
  else
  {
    gdl_plane_config_end(GDL_TRUE);
  }

  if (GDL_SUCCESS != rc)
  {
    CLog::Log(LOGINFO, "GDL configuration failed! GDL error code is 0x%x\n", rc);
    return false;
  }

  CLog::Log(LOGINFO, "GDL plane setup complete");

  if (!m_eglBinding.CreateWindow(EGL_DEFAULT_DISPLAY, (NativeWindowType) m_gdlPlane))
  {
    return false;
  }


  gdl_flip(GDL_PLANE_ID_UPP_A, GDL_SURFACE_INVALID, GDL_FLIP_ASYNC);
  gdl_flip(GDL_PLANE_ID_UPP_B, GDL_SURFACE_INVALID, GDL_FLIP_ASYNC);
  gdl_flip(GDL_PLANE_ID_UPP_C, GDL_SURFACE_INVALID, GDL_FLIP_ASYNC);
  gdl_flip(GDL_PLANE_ID_UPP_D, GDL_SURFACE_INVALID, GDL_FLIP_ASYNC);

  CLog::Log(LOGINFO, "GDL plane attach to EGL complete");

  g_graphicsContext.UpdateGraphicsRects();

  return true;
}

bool CWinSystemGDL::DestroyWindow()
{
  if (!m_eglBinding.DestroyWindow())
  {
    return false;
  }

  return true;
}

bool CWinSystemGDL::ResizeWindow(int newWidth, int newHeight, int newLeft, int newTop)
{
  return true;
}

bool CWinSystemGDL::SetFullScreen(bool fullScreen, RESOLUTION_INFO& res, bool blankOtherDisplays)
{
  // check if we're trying to create the same window
#ifdef HAS_EMBEDDED
  if (m_Res == res)
  {
    CLog::Log(LOGINFO, "CWinSystemGDL::SetFullScreen same window, ignoring request.\n");
    return true;
  }
#endif

  m_nWidth = res.iWidth;
  m_nHeight = res.iHeight;
  m_bFullScreen = fullScreen;

  m_eglBinding.ReleaseSurface();
  CreateNewWindow("", fullScreen, res, NULL);

  return true;
}

bool CWinSystemGDL::EnableHDMIClamp(bool bEnable)
{
  gdl_boolean_t hdmiClamp = bEnable ? GDL_TRUE : GDL_FALSE;

  if(gdl_port_set_attr(GDL_PD_ID_HDMI, GDL_PD_ATTR_ID_OUTPUT_CLAMP, &hdmiClamp) != GDL_SUCCESS)
  {
    CLog::Log(LOGERROR, "Could not set gdl output clamp");
    return false;
  }

  gdl_pd_attribute_t test;
  gdl_port_get_attr(GDL_PD_ID_HDMI, GDL_PD_ATTR_ID_OUTPUT_CLAMP, &test);

  if(test.content._bool.value == GDL_TRUE)
    CLog::Log(LOGINFO, "HDMI Clamp enabled\n");
  else
    CLog::Log(LOGINFO, "HDMI Clamp disabled\n");

  return true;
}

bool CWinSystemGDL::ConfigureHDMIOutput()
{
  g_IntelSMDGlobals.DisableAudioOutput(g_IntelSMDGlobals.GetHDMIOutput());

  gdl_display_info_t di;
  gdl_pd_output_pixel_format_t pixelformat = GDL_PD_OPF_RGB444;
  gdl_color_space_t color_space = GDL_COLOR_SPACE_RGB;
  gdl_gamma_t gamma = GDL_GAMMA_LINEAR;
  gdl_refresh_t refresh = GDL_REFRESH_59_94;
  gdl_pd_output_pixel_depth_t pixel_depth = GDL_PD_OPD_24BIT;
  gdl_boolean_t hdcpControlEnabled = GDL_FALSE;
  int use_edid = false;

  memset(&di, 0, sizeof (gdl_display_info_t));

  HDMIOutputType hdmi_output = (HDMIOutputType)g_guiSettings.GetInt("videoscreen.hdmioutput");
  HDMIPixelDepth hdmi_pixel_depth = (HDMIPixelDepth)g_guiSettings.GetInt("videoscreen.hdmipixeldepth");
  use_edid = g_guiSettings.GetBool("videoscreen.forceedid");

  CLog::Log(LOGINFO, "CWinSystemGDL::ConfigureHDMIOutput use_edid %d\n", use_edid);

  if (m_Res.fRefreshRate < 23.99f && m_Res.fRefreshRate > 23.97)
    refresh = GDL_REFRESH_23_98;
  else if (m_Res.fRefreshRate < 24.01f && m_Res.fRefreshRate > 23.99)
    refresh = GDL_REFRESH_24;
  else if (m_Res.fRefreshRate < 25.01f && m_Res.fRefreshRate > 24.99)
    refresh = GDL_REFRESH_25;
  else if (m_Res.fRefreshRate < 29.98f && m_Res.fRefreshRate > 29.96)
    refresh = GDL_REFRESH_29_97;
  else if (m_Res.fRefreshRate < 30.01f && m_Res.fRefreshRate > 29.99)
    refresh = GDL_REFRESH_30;
  else if (m_Res.fRefreshRate < 50.01f && m_Res.fRefreshRate > 49.99)
    refresh = GDL_REFRESH_50;
  else if (m_Res.fRefreshRate < 59.95f && m_Res.fRefreshRate > 59.93)
    refresh = GDL_REFRESH_59_94;
  else if (m_Res.fRefreshRate < 60.01f && m_Res.fRefreshRate > 59.99)
    refresh = GDL_REFRESH_60;
  else if (m_Res.fRefreshRate < 48.01f && m_Res.fRefreshRate > 47.99)
    refresh = GDL_REFRESH_48;
  else if (m_Res.fRefreshRate < 47.97f && m_Res.fRefreshRate > 47.95)
    refresh = GDL_REFRESH_47_96;
  else
  {
    CLog::Log(LOGERROR, "Unsupported refresh rate: %f. Using 59_94 instead", m_Res.fRefreshRate);
    refresh = GDL_REFRESH_59_94;
  }

  switch (hdmi_output)
  {
  case HDMI_OUTPUT_RGB_HI:
  case HDMI_OUTPUT_RGB_LOW:
    pixelformat = GDL_PD_OPF_RGB444;
    color_space = GDL_COLOR_SPACE_RGB;
    break;
  case HDMI_OUTPUT_YUV_422:
    pixelformat = GDL_PD_OPF_YUV422;
    color_space = GDL_COLOR_SPACE_BT709;
    break;
  case HDMI_OUTPUT_YUV_444:
    pixelformat = GDL_PD_OPF_YUV444;
    color_space = GDL_COLOR_SPACE_BT709;
    break;
  case HDMI_OUTPUT_COUNT:
    // unhandled?
    break;
  }

  switch (hdmi_pixel_depth)
  {
  case HDMI_PIXEL_DEPTH_24:
    pixel_depth = GDL_PD_OPD_24BIT;
    break;
  case HDMI_PIXEL_DEPTH_30:
    pixel_depth = GDL_PD_OPD_30BIT;
    break;
  case HDMI_PIXEL_DEPTH_36:
    pixel_depth = GDL_PD_OPD_36BIT;
    break;
  case HDMI_PIXEL_DEPTH_COUNT:
    // unhandled?
    break;
  }


  // set display info
  di.id = GDL_DISPLAY_ID_0; // hdmi
  di.flags = 0;
  di.bg_color = 0;
  di.color_space = color_space;
  di.gamma = gamma;
  di.tvmode.width = m_Res.iWidth;
  di.tvmode.height = m_Res.iHeight;
  di.tvmode.refresh = refresh;
  di.tvmode.interlaced = (m_Res.dwFlags & D3DPRESENTFLAG_INTERLACED ? GDL_TRUE
      : GDL_FALSE);
#if GDL_HEADER_VERSION_MAJOR >= 79
  di.tvmode.stereo_type = GDL_STEREO_NONE;
#endif

  // if we got any bad parameter, reset to 720p60
  if(di.tvmode.width == 0 || di.tvmode.height == 0)
  {
    di.tvmode.width = 1280;
    di.tvmode.height = 720;
  }

  if (gdl_set_display_info(&di) != GDL_SUCCESS)
  {
    CLog::Log(LOGINFO, "Could not set display mode for display 0");
    goto fail;
  }

  query_generic_info(GDL_PD_ID_HDMI);
  query_hdcp_info(GDL_PD_ID_HDMI);

  if (gdl_port_set_attr(GDL_PD_ID_HDMI, GDL_PD_ATTR_ID_HDCP, &hdcpControlEnabled) != GDL_SUCCESS)
  {
    CLog::Log(LOGINFO, "Could not enable HDCP control");
  }

  if (gdl_port_set_attr(GDL_PD_ID_HDMI, GDL_PD_ATTR_ID_USE_EDID, &use_edid) != GDL_SUCCESS)
  {
    CLog::Log(LOGINFO, "Could not enable EDID control");
  }

  // pixel format
  if (gdl_port_set_attr(GDL_PD_ID_HDMI, GDL_PD_ATTR_ID_PIXEL_FORMAT_OUTPUT, &pixelformat) == GDL_SUCCESS)
  {
    CLog::Log(LOGINFO, "CWinSystemGDL::SetHDMIOutput setting GDL_PD_ATTR_ID_PIXEL_FORMAT_OUTPUT %d", pixelformat);
  }
  else
    CLog::Log(LOGINFO, "CWinSystemGDL::SetHDMIOutput setting GDL_PD_ATTR_ID_PIXEL_FORMAT_OUTPUT failed");

  gdl_pd_attribute_t test;
  if (gdl_port_get_attr(GDL_PD_ID_HDMI, GDL_PD_ATTR_ID_PIXEL_FORMAT_OUTPUT, &test) == GDL_SUCCESS)
  {
    CLog::Log(LOGINFO, "CWinSystemGDL::SetHDMIOutput reading GDL_PD_ATTR_ID_PIXEL_FORMAT_OUTPUT %d", test.content._uint.value);
  }
  else
    CLog::Log(LOGINFO, "CWinSystemGDL::SetHDMIOutput reading GDL_PD_ATTR_ID_PIXEL_FORMAT_OUTPUT failed");



  // pixel depth
  if (gdl_port_set_attr(GDL_PD_ID_HDMI, GDL_PD_ATTR_ID_PIXEL_DEPTH, &pixel_depth) == GDL_SUCCESS)
  {
    CLog::Log(LOGINFO, "CWinSystemGDL::SetHDMIOutput setting GDL_PD_ATTR_ID_PIXEL_DEPTH %d", pixel_depth);
  }
  else
    CLog::Log(LOGINFO, "CWinSystemGDL::SetHDMIOutput setting GDL_PD_ATTR_ID_PIXEL_DEPTH failed");

  if (gdl_port_get_attr(GDL_PD_ID_HDMI, GDL_PD_ATTR_ID_PIXEL_DEPTH, &test) == GDL_SUCCESS)
  {
    CLog::Log(LOGINFO, "CWinSystemGDL::SetHDMIOutput reading GDL_PD_ATTR_ID_PIXEL_DEPTH %d\n", test.content._uint.value);
  }
  else
    CLog::Log(LOGINFO, "CWinSystemGDL::SetHDMIOutput reading GDL_PD_ATTR_ID_PIXEL_DEPTH failed");


  if(hdmi_output == HDMI_OUTPUT_RGB_HI)
    EnableHDMIClamp(false);
  else if(hdmi_output == HDMI_OUTPUT_RGB_LOW)
    EnableHDMIClamp(true);

  g_IntelSMDGlobals.EnableAudioOutput(g_IntelSMDGlobals.GetHDMIOutput());

  return true;

  fail:
  g_IntelSMDGlobals.EnableAudioOutput(g_IntelSMDGlobals.GetHDMIOutput());
  return false;
}

bool CWinSystemGDL::EnableHardwareScaler(bool bEnable, CRect srcRect, CRect dstRect, gdl_plane_id_t plane)
{
  gdl_ret_t rc;
  gdl_boolean_t scaleEnabled;
  gdl_rectangle_t rect;

  //printf("CWinSystemGDL::EnableHardwareScaler plane %d set to %d\n", plane, bEnable);

  if(g_graphicsContext.GetHeight() <= 720 && bEnable)
  {
    /*
    printf("CWinSystemGDL::EnableHardwareScaler: screen height %d <= 720. Ignoring\n",
        g_graphicsContext.GetHeight());
        */
    return true;
  }

  if(srcRect.Width() > 1280 && bEnable)
  {
    //printf("CWinSystemGDL::EnableHardwareScaler: src width %d > 1280. Ignoring\n", srcRect.Width());
    return true;
  }

  /*
  printf("CWinSystemGDL::EnableHardwareScaler src %.0f %.0f %.0f %.0f  dst %.0f %.0f %.0f %.0f\n",
      srcRect.x1, srcRect.y1, srcRect.x2, srcRect.y2,
      dstRect.x1, dstRect.y1, dstRect.x2, dstRect.y2);
      */

  if (bEnable)
  {
    // reset any leftovers from plance C which is used for the browser
    gdl_rectangle_t rect;
    gdl_boolean_t scale = GDL_FALSE;
    rect.origin.x = rect.origin.y = 0;
    rect.width=1280;
    rect.height=720;
    gdl_plane_config_begin(GDL_PLANE_ID_UPP_C);
    gdl_plane_set_attr(GDL_PLANE_SCALE, &scale);
    gdl_plane_set_rect(GDL_PLANE_DST_RECT, &rect);
    gdl_plane_set_rect(GDL_PLANE_SRC_RECT, &rect);
    gdl_plane_config_end(GDL_FALSE);
  }

  rc = gdl_plane_config_begin(plane);

  rect.origin.x = (gdl_int32)srcRect.x1;
  rect.origin.y = (gdl_int32)srcRect.y1;
  rect.width = (gdl_int32)(srcRect.x2 - srcRect.x1);
  rect.height = (gdl_int32)(srcRect.y2 - srcRect.y1);

  if (GDL_SUCCESS == rc)
    rc = gdl_plane_set_attr(GDL_PLANE_SRC_RECT, &rect);


  rect.origin.x = (gdl_int32)dstRect.x1;
  rect.origin.y = (gdl_int32)dstRect.y1;
  rect.width = (gdl_int32)(dstRect.x2 - dstRect.x1);
  rect.height = (gdl_int32)(dstRect.y2 - dstRect.y1);

  if(GDL_SUCCESS == rc)
    rc = gdl_plane_set_attr(GDL_PLANE_DST_RECT, &rect);

  if(GDL_SUCCESS == rc)
  {
    scaleEnabled = (gdl_boolean_t)bEnable;
    rc = gdl_plane_set_attr(GDL_PLANE_SCALE, &scaleEnabled);
  }

  if (GDL_SUCCESS == rc)
    gdl_plane_config_end(GDL_FALSE);
  else
    gdl_plane_config_end(GDL_TRUE);

  if (GDL_SUCCESS != rc)
  {
    CLog::Log(LOGERROR, "CWinSystemGDL::EnableHardwareScaler failed. GDL error code is 0x%x\n", rc);
    return false;
  }

  return true;
}

bool CWinSystemGDL::IsHardwareScalerEnabled(gdl_plane_id_t plane)
{
  gdl_ret_t rc;
  gdl_boolean_t scaleEnabled = GDL_FALSE;

  rc = gdl_plane_get_attr(plane, GDL_PLANE_SCALE, &scaleEnabled);
  if(rc != GDL_SUCCESS)
  {
    CLog::Log(LOGERROR, "CWinSystemGDL::IsHardwareScalerEnabled failed. GDL error code is 0x%x\n", rc);
    return false;
  }

  //printf("CWinSystemGDL::IsHardwareScalerEnabled %d\n", (bool)scalineEnabled);

  return (bool)scaleEnabled;
}

bool CWinSystemGDL::GetPlaneRect(gdl_plane_id_t plane, CRect& myRect, gdl_plane_attr_t attrib)
{
  gdl_ret_t rc;
  gdl_rectangle_t rect;

  if(attrib != GDL_PLANE_DST_RECT && attrib != GDL_PLANE_SRC_RECT)
    return false;

  rc = gdl_plane_get_attr(plane, attrib, &rect);
  if(rc != GDL_SUCCESS)
  {
    CLog::Log(LOGERROR, "CWinSystemGDL::GetPlaneDstRect failed. GDL error code is 0x%x\n", rc);
    return false;
  }

  myRect.x1 = (float)(rect.origin.x);
  myRect.y1 = (float)(rect.origin.y);
  myRect.x2 = (float)(rect.origin.x + rect.width);
  myRect.y2 = (float)(rect.origin.y + rect.height);

  return true;
}

void CWinSystemGDL::UpdateResolutions()
{
  gdl_display_id_t   display_id = GDL_DISPLAY_ID_0;
  gdl_pd_id_t        pd_id = GDL_PD_MIN_SUPPORTED_DRIVERS;
  gdl_pd_attribute_t attr;
  gdl_ret_t          rc;
  gdl_tvmode_t       m;
  int                i = 0;
  RESOLUTION         boxeeResolution = RES_DESKTOP;
  RESOLUTION         Res720p = RES_INVALID;

  CLog::Log(LOGINFO, "Display <%d> supports the following modes:", display_id);

  // Traverse through all ports
  while (pd_id < GDL_PD_MAX_SUPPORTED_DRIVERS)
  {
    // Find any port on given display
    rc = gdl_port_get_attr(pd_id, GDL_PD_ATTR_ID_DISPLAY_PIPE, &attr);
    if ((rc == GDL_SUCCESS) && (attr.content._uint.value == (gdl_uint32) display_id))
    {
      // Get modes supported by given port, 1 by 1
      while (gdl_get_port_tvmode_by_index(pd_id, i++, &m) == GDL_SUCCESS)
      {
        bool ignored = true;

        // Check if given mode is supported on given display
        if (gdl_check_tvmode(display_id, &m) == GDL_SUCCESS)
        {
          // We only care about 23.98, 59.94 or 50 Hz resolutions
          if (m.refresh == GDL_REFRESH_23_98 || m.refresh == GDL_REFRESH_59_94 ||
              m.refresh == GDL_REFRESH_50)
          {
            ignored = false;
          }

          /*
          if(m.refresh == GDL_REFRESH_24)
          {
            m_bSupportTrue24p = true;
            printf("Device supports True 24p\n");
          }
          */

          CLog::Log(LOGINFO, "Resolution: %dx%d%c%s - %s\n",
              m.width,
              m.height,
              m.interlaced ? 'i' : 'p',
              gdl_dbg_string_refresh(m.refresh),
              (ignored ? "ignored" : "added"));

          if (ignored)
            continue;

          float refresh;
          switch (m.refresh)
          {
          case GDL_REFRESH_23_98: refresh = 23.98; break;
          case GDL_REFRESH_24:    refresh = 24.00; break;
          case GDL_REFRESH_25:    refresh = 25.00; break;
          case GDL_REFRESH_29_97: refresh = 29.97; break;
          case GDL_REFRESH_30:    refresh = 30.00; break;
          case GDL_REFRESH_50:    refresh = 50.00; break;
          case GDL_REFRESH_59_94: refresh = 59.94; break;
          case GDL_REFRESH_60:    refresh = 60.00; break;
          case GDL_REFRESH_48:    refresh = 48.00; break;
          case GDL_REFRESH_47_96: refresh = 47.96; break;
          default:                refresh = 0.0;   break;
          }

          if (g_settings.m_ResInfo.size() <= (unsigned int) boxeeResolution)
          {
            RESOLUTION_INFO res;
            g_settings.m_ResInfo.push_back(res);
          }

          UpdateDesktopResolution(g_settings.m_ResInfo[boxeeResolution], 0, m.width, m.height, refresh, m.interlaced);

          // Update default overscan
          if (DEFAULT_OVERSCAN > 0)
          {
            g_settings.m_ResInfo[boxeeResolution].Overscan.left = (int) ((float) m.width * DEFAULT_OVERSCAN);
            g_settings.m_ResInfo[boxeeResolution].Overscan.top = (int) ((float) m.height * DEFAULT_OVERSCAN);
            g_settings.m_ResInfo[boxeeResolution].Overscan.right = m.width - (int) ((float) m.width * DEFAULT_OVERSCAN);
            g_settings.m_ResInfo[boxeeResolution].Overscan.bottom =  m.height - (int) ((float) m.height * DEFAULT_OVERSCAN);
          }

          if(m.width == 1280 && m.height == 720 && !m.interlaced && m.refresh == GDL_REFRESH_59_94)
            Res720p = boxeeResolution;

          boxeeResolution = (RESOLUTION) (((int) boxeeResolution) + 1);
        }
      }

      // We are done
      break;
    }

    pd_id = (gdl_pd_id_t) (((int) pd_id) + 1);
  }

  // swap desktop for 720p if available
  if(Res720p != RES_INVALID)
  {
    CLog::Log(LOGINFO, "Found 720p at %d, setting to desktop", (int)Res720p);

    RESOLUTION_INFO desktop = g_settings.m_ResInfo[RES_DESKTOP];
    g_settings.m_ResInfo[RES_DESKTOP] = g_settings.m_ResInfo[Res720p];
    g_settings.m_ResInfo[Res720p] = desktop;
  }
  else
  {
    CLog::Log(LOGINFO, "Warning: 720p not supported, desktop not changed");
  }
}

#define LOWER_NIBBLE( x ) \
        ((1|2|4|8) & (x))

#define UPPER_NIBBLE( x ) \
        (((128|64|32|16) & (x)) >> 4)

#define DETAILED_TIMING_DESCRIPTIONS_START      0x36
#define DETAILED_TIMING_DESCRIPTION_SIZE        18
#define NO_DETAILED_TIMING_DESCRIPTIONS         4

#define UNKNOWN_DESCRIPTOR      -1
#define DETAILED_TIMING_BLOCK   -2

#define COMBINE_HI_8LO( hi, lo ) \
        ( (((unsigned)hi) << 8) | (unsigned)lo )

#define PIXEL_CLOCK_LO     (unsigned)block[ 0 ]
#define PIXEL_CLOCK_HI     (unsigned)block[ 1 ]
#define PIXEL_CLOCK        (COMBINE_HI_8LO( PIXEL_CLOCK_HI,PIXEL_CLOCK_LO )*10000)

#define H_BLANKING_LO      (unsigned)block[ 3 ]
#define H_BLANKING_HI      LOWER_NIBBLE( (unsigned)block[ 4 ] )
#define H_BLANKING         COMBINE_HI_8LO( H_BLANKING_HI, H_BLANKING_LO )

#define V_BLANKING_LO      (unsigned)block[ 6 ]
#define V_BLANKING_HI      LOWER_NIBBLE( (unsigned)block[ 7 ] )
#define V_BLANKING         COMBINE_HI_8LO( V_BLANKING_HI, V_BLANKING_LO )

#define V_ACTIVE_LO        (unsigned)block[ 5 ]
#define V_ACTIVE_HI        UPPER_NIBBLE( (unsigned)block[ 7 ] )
#define V_ACTIVE           COMBINE_HI_8LO( V_ACTIVE_HI, V_ACTIVE_LO )

#define H_ACTIVE_LO        (unsigned)block[ 2 ]
#define H_ACTIVE_HI        UPPER_NIBBLE( (unsigned)block[ 4 ] )
#define H_ACTIVE           COMBINE_HI_8LO( H_ACTIVE_HI, H_ACTIVE_LO )

#define FLAGS              (unsigned)block[ 17 ]
#define INTERLACED         (FLAGS&128)

const unsigned char edid_v1_descriptor_flag[] = { 0x00, 0x00 };

void CWinSystemGDL::GetNativeDisplayResolution(std::vector<RESOLUTION_INFO>& resolutions)
{
  unsigned char* edid_buffer;
  int edid_len;

  if (ReadEDID(&edid_buffer, &edid_len))
  {
    GetNativeResolutionFromEDID(edid_buffer, edid_len, resolutions);
  }

  if (resolutions.size() == 0)
  {
    RESOLUTION_INFO fallthrough;
    fallthrough.iWidth = 1280;
    fallthrough.iHeight = 720;
    fallthrough.fRefreshRate = 59.94f;
    resolutions.push_back(fallthrough);
  }
}

bool ResolutionSortPredicate(const RESOLUTION_INFO& d1, const RESOLUTION_INFO& d2)
{
  return (d1.iWidth * d1.iHeight) > (d2.iWidth * d2.iHeight);
}

bool CWinSystemGDL::GetNativeResolutionFromEDID(unsigned char* edid_data, int edid_len, std::vector<RESOLUTION_INFO>& resolutions)
{
  std::vector<RESOLUTION_INFO> interlacedResolutions;

  unsigned char* block = edid_data + DETAILED_TIMING_DESCRIPTIONS_START;

  for (int i = 0; i < NO_DETAILED_TIMING_DESCRIPTIONS; i++, block += DETAILED_TIMING_DESCRIPTION_SIZE)
  {
    if (GetEDIDBlockType( block ) == DETAILED_TIMING_BLOCK)
    {
      RESOLUTION_INFO resolution;

      int htotal, vtotal;
      htotal = H_ACTIVE + H_BLANKING;
      vtotal = V_ACTIVE + V_BLANKING;
      float vfreq = (float)PIXEL_CLOCK/((float)vtotal*(float)htotal);

      resolution.iWidth = H_ACTIVE;
      resolution.iHeight = V_ACTIVE;
      resolution.fRefreshRate = vfreq;
      resolution.dwFlags = 0;
      if (INTERLACED)
      {
        resolution.dwFlags |= D3DPRESENTFLAG_INTERLACED;
        resolution.iHeight *= 2;
        interlacedResolutions.push_back(resolution);
      }
      else
      {
        resolutions.push_back(resolution);
      }
    }
  }

  // sort resolutions according to size
  std::sort(resolutions.begin(), resolutions.end(), ResolutionSortPredicate);
  std::sort(interlacedResolutions.begin(), interlacedResolutions.end(), ResolutionSortPredicate);

  // append interlaced resolution at the end
  resolutions.insert(resolutions.end(), interlacedResolutions.begin(), interlacedResolutions.end());

  return true;
}

int CWinSystemGDL::GetEDIDBlockType( unsigned char* block )
{
  if (!strncmp( (const char*) edid_v1_descriptor_flag, (const char*) block, 2))
  {
     if (block[2] != 0)
       return UNKNOWN_DESCRIPTOR;
     return block[ 3 ];
  }
  else
  {
    /* detailed timing block */
    return DETAILED_TIMING_BLOCK;
  }
}


bool CWinSystemGDL::ReadEDID(unsigned char** edid_data, int* len)
{
  gdl_ret_t rc = GDL_SUCCESS;
  unsigned int n;
  gdl_hdmi_edid_block_t eb;
  eb.index = 0;

  *edid_data = NULL;
  *len = 0;

  // Read first block
  if (gdl_port_recv(GDL_PD_ID_HDMI, GDL_PD_RECV_HDMI_EDID_BLOCK, (void *) &eb,
      sizeof(gdl_hdmi_edid_block_t)) != GDL_SUCCESS)
  {
    return false;
  }

  // Determine total number of blocks
  n = eb.data[126] + 1;

  *edid_data = (unsigned char*) malloc(n * 128);

  // Read and print all EDID blocks
  for (eb.index = 0; eb.index < n; eb.index++)
  {
    rc = gdl_port_recv(GDL_PD_ID_HDMI, GDL_PD_RECV_HDMI_EDID_BLOCK,
        (void *) &eb, sizeof(gdl_hdmi_edid_block_t));

    if (rc == GDL_SUCCESS)
    {
      memcpy(*edid_data + *len, eb.data, 128);
      *len += 128;
    }

    if (rc != GDL_SUCCESS)
      break;
  }

  return true;
}

void CWinSystemGDL::NotifyAppActiveChange(bool bActivated)
{
}

bool CWinSystemGDL::Minimize()
{
  return true;
}

bool CWinSystemGDL::Restore()
{
  return true;
}

bool CWinSystemGDL::Hide()
{
  return true;
}

bool CWinSystemGDL::Show(bool raise)
{
  return true;
}

EGLDisplay CWinSystemGDL::GetEGLDisplay()
{
  return m_eglBinding.GetDisplay();
}

gdl_gamma_t GetGDLGamma(GAMMA gamma)
{
  switch(gamma)
  {
  case GAMMA_LINEAR:
    return GDL_GAMMA_LINEAR;
  case GAMMA_2_2:
    return GDL_GAMMA_2_2;
  case GAMMA_2_8:
    return GDL_GAMMA_2_8;
  }

  return GDL_GAMMA_LINEAR;
}

#endif
