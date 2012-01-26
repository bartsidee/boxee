/* 
 * File:   LicenseConfig.h
 * Author: ishai
 *
 * Created on March 21, 2011, 4:28 PM
 */

#ifndef LICENSECONFIG_H
#define	LICENSECONFIG_H

#include "system.h"

#ifdef HAS_EMBEDDED

#include <map>

#include "utils/log.h"

enum AUDIO_CODEC_VENDOR {
    AUDIO_VENDOR_NONE,
    AUDIO_VENDOR_DOLBY,
    AUDIO_VENDOR_DTS,
    NUM_OF_VENDOR_VALS
};

extern const int DEF_MAX_PCM_CHANNELS_DECODE;
extern const int DEF_MAX_CHANNELS_ENCODE;
extern const AUDIO_CODEC_VENDOR DEF_PREFERRED_ENCODING;

struct CVendorLicense {
    int m_max_channels_decode;
    int m_max_channels_encode;
};
class CLicenseSettings {
    std::map<AUDIO_CODEC_VENDOR, CVendorLicense> m_vendor_licenses;
    AUDIO_CODEC_VENDOR m_preferred_encoding;
    bool m_is_initialized;
    bool m_is_dolby_sw_decode_allowed;
    bool m_is_dts_sw_decode_allowed;
public:
    void Load();
    int get_max_channels_decode(AUDIO_CODEC_VENDOR vendor) const;
    int get_max_channels_encode(AUDIO_CODEC_VENDOR vendor) const;
    bool is_dolby_sw_decode_allowed() const;
    bool is_dts_sw_decode_allowed() const;
    AUDIO_CODEC_VENDOR get_preferred_encoding() const;
    CLicenseSettings() {m_is_initialized=false;}
    void dump_audio_licnense_file();
    void ModifyAudioChannelsByLicense(int& iChannels, AUDIO_CODEC_VENDOR vendor);
};

extern CLicenseSettings g_lic_settings;
#endif

#endif	/* LICENSECONFIG_H */

