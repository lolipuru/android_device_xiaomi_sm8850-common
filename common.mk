#
# Copyright (C) 2026 The LineageOS Project
#
# SPDX-License-Identifier: Apache-2.0
#

# Enable virtual AB with vendor ramdisk
$(call inherit-product, $(SRC_TARGET_DIR)/product/virtual_ab_ota/vabc_features.mk)

# Enable project quotas and casefolding for emulated storage without sdcardfs.
$(call inherit-product, $(SRC_TARGET_DIR)/product/emulated_storage.mk)

# Setup dalvik vm configs
$(call inherit-product, frameworks/native/build/phone-xhdpi-6144-dalvik-heap.mk)

# Enforce generic ramdisk allow list
$(call inherit-product, $(SRC_TARGET_DIR)/product/generic_ramdisk.mk)

# Add common definitions for Qualcomm
$(call inherit-product, hardware/qcom-caf/common/common.mk)

# Virtualization service
$(call inherit-product, packages/modules/Virtualization/apex/product_packages.mk)

# Call the proprietary setup.
$(call inherit-product, vendor/xiaomi/sm8850-common/sm8850-common-vendor.mk)

# SHIPPING API
BOARD_SHIPPING_API_LEVEL := 202504
PRODUCT_SHIPPING_API_LEVEL := 36

# AAPT
PRODUCT_AAPT_CONFIG := normal
PRODUCT_AAPT_PREF_CONFIG := xxhdpi

# A/B
AB_OTA_POSTINSTALL_CONFIG += \
    RUN_POSTINSTALL_system=true \
    POSTINSTALL_PATH_system=system/bin/otapreopt_script \
    FILESYSTEM_TYPE_system=ext4 \
    POSTINSTALL_OPTIONAL_system=true

AB_OTA_POSTINSTALL_CONFIG += \
    RUN_POSTINSTALL_vendor=true \
    POSTINSTALL_PATH_vendor=bin/checkpoint_gc \
    FILESYSTEM_TYPE_vendor=ext4 \
    POSTINSTALL_OPTIONAL_vendor=true

PRODUCT_PACKAGES += \
    checkpoint_gc \
    otapreopt_script

PRODUCT_VIRTUAL_AB_COMPRESSION_METHOD := lz4

# Hypsy
PRODUCT_PACKAGES += \
    xiaomi.system.hypsys.common-service

# Rootdir
PRODUCT_PACKAGES += \
    charger_fstab.qcom \
    fstab.qcom \
    init.recovery.qcom.rc \
    init.qcom.rc \
    init.target.rc \
    ueventd.qcom.rc

PRODUCT_COPY_FILES += \
    $(LOCAL_PATH)/rootdir/etc/fstab.qcom:$(TARGET_COPY_OUT_VENDOR_RAMDISK)/first_stage_ramdisk/fstab.qcom

# WiFi firmware symlinks
PRODUCT_PACKAGES += \
    firmware_wlanmdsp.otaupdate_symlink \
    firmware_wlan_mac.bin_symlink \
    firmware_WCNSS_qcom_cfg.ini_symlink
