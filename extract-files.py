#!/usr/bin/env -S PYTHONPATH=../../../tools/extract-utils python3
#
# SPDX-FileCopyrightText: 2026 The LineageOS Project
# SPDX-License-Identifier: Apache-2.0
#

from extract_utils.fixups_blob import (
    blob_fixup,
    blob_fixups_user_type,
)
from extract_utils.fixups_lib import (
    lib_fixup_remove,
    lib_fixups,
    lib_fixups_user_type,
)
from extract_utils.main import (
    ExtractUtils,
    ExtractUtilsModule,
)

namespace_imports = [
    'device/xiaomi/sm8850-common',
    'hardware/qcom-caf/sm8850',
    'hardware/qcom-caf/wlan',
    'hardware/xiaomi',
    'vendor/qcom/opensource/commonsys/display',
    'vendor/qcom/opensource/commonsys-intf/display',
    'vendor/qcom/opensource/dataservices',
]

def lib_fixup_vendor_suffix(lib: str, partition: str, *args, **kwargs):
    return f'{lib}_{partition}' if partition == 'vendor' else None


lib_fixups: lib_fixups_user_type = {
    **lib_fixups,
    (
        'vendor.qti.ImsRtpService-V2-ndk',
        'vendor.qti.diaghal-V1-ndk',
        'vendor.qti.qccsyshal_aidl-V1-ndk',
        'vendor.qti.qccvndhal_aidl-V1-ndk',
    ): lib_fixup_vendor_suffix,
    (
        'libcamxcommonutils',
        'libframemaster',
        'libmialgo',
    ): lib_fixup_remove,
}

blob_fixups: blob_fixups_user_type = {
    'system_ext/etc/init/qspa_system.rc': blob_fixup()
        .regex_replace(r'\$\{ro\.boot\.vendor\.qspa:-default\}', 'default'),
    (
        'vendor/lib64/libVoiceSdk.so',
        'vendor/lib64/libcapiv2uvvendor.so',
        'vendor/lib64/liblistensoundmodel2vendor.so',
    ): blob_fixup()
        .replace_needed('libtensorflowlite_c.so', 'libtensorflowlite_c_vendor.so'),
    (
        'odm/bin/hw/vendor.xiaomi.sensor.citsensorservice.aidl',
        'odm/lib64/hw/displayfeature.default.so',
        'odm/lib64/libmiXmlParser.so',
        'vendor/bin/hw/audiohalservice.qti',
        'vendor/bin/hw/vendor.qti.hardware.display.composer-service',
        'vendor/bin/poweropt-service',
        'vendor/bin/poweropt-service',
        'vendor/bin/qsap_mpamsvc',
        'vendor/lib64/hw/libaudioeffecthal.qti.so',
        'vendor/lib64/libaodoptfeature.so',
        'vendor/lib64/libaudiocloudctrl.so',
        'vendor/lib64/libcamerapoweroptfeature.so',
        'vendor/lib64/libdemura_oem_plugin.so',
        'vendor/lib64/libgamepoweroptfeature.so',
        'vendor/lib64/liblearningmodule.so',
        'vendor/lib64/liboffscreenpoweroptfeature.so',
        'vendor/lib64/libpowercallback.so',
        'vendor/lib64/libpowercore.so',
        'vendor/lib64/libpsmoptfeature.so',
        'vendor/lib64/libsdmclient.so',
        'vendor/lib64/libstandbyfeature.so',
        'vendor/lib64/libvideooptfeature.so',
        'vendor/lib64/libxlog.so',
        'vendor/lib64/soundfx/libquasar.so',
    ): blob_fixup()
        .replace_needed('libtinyxml2.so', 'libtinyxml2-v36.so'),
     (
       'odm/lib64/hw/displayfeature.default.so',
       'odm/lib64/libadaptivehdr.so',
       'odm/lib64/libcolortempmode.so',
       'odm/lib64/libdither.so',
       'odm/lib64/libflatmode.so',
       'odm/lib64/libhistprocess.so',
       'odm/lib64/libmiBrightness.so',
       'odm/lib64/libmiSensorCtrl.so',
       'odm/lib64/libpaperMode.so',
       'odm/lib64/librhytheyecare.so',
       'odm/lib64/libsdr2hdr.so',
       'odm/lib64/libsre.so',
       'odm/lib64/libtruetone.so',
       'odm/lib64/libvideomode.so',
     ): blob_fixup()
        .replace_needed('android.hardware.sensors-V2-ndk.so','android.hardware.sensors-V3-ndk.so'),
     (
       'odm/bin/hw/vendor.xiaomi.sensor.citsensorservice.aidl',
     ): blob_fixup()
        .replace_needed('android.hardware.graphics.common-V6-ndk.so','android.hardware.graphics.common-V7-ndk.so'),
    (
        'vendor/lib64/libqcodec2_core.so',
    ): blob_fixup()
        .replace_needed('android.hardware.graphics.common-V5-ndk.so', 'android.hardware.graphics.common-V7-ndk.so'),
    (
        'vendor/bin/hw/vendor.qti.media.c2@1.0-service',
        'vendor/bin/hw/vendor.qti.media.c2audio@1.0-service',
    ): blob_fixup()
        .replace_needed('android.hardware.media.c2-V1-ndk.so', 'android.hardware.media.c2-V2-ndk.so'),
    (
        'odm/lib64/libmt.so',
    ): blob_fixup()
        .replace_needed('libkeymint.so', 'libkeymint-sm8850.so')
        .replace_needed('libkeymint_remote_prov_support.so', 'libkeymint_remote_prov_support-sm8850.so')
        .replace_needed('libkeymint_support.so', 'libkeymint_support-sm8850.so'),
    (
        'vendor/lib64/libkeymint_remote_prov_support-sm8850.so',
    ): blob_fixup()
        .replace_needed('libcppcose_rkp.so', 'libcppcose_rkp-sm8850.so'),
}  # fmt: skip

module = ExtractUtilsModule(
    'sm8850-common',
    'xiaomi',
    blob_fixups=blob_fixups,
    lib_fixups=lib_fixups,
    namespace_imports=namespace_imports,
)

if __name__ == '__main__':
    utils = ExtractUtils.device(module)
    utils.run()