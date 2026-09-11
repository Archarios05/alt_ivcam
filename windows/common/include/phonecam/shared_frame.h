// 共有メモリの構造体と名前（SPEC §8）。
// 書き手は Receiver / testpattern（x64）、読み手はフィルタ DLL（x86 / x64）。
// 32bit / 64bit 間で共有するため固定幅の型のみを使い、レイアウトを static_assert で固定する。
#pragma once

#include <windows.h>

#include <cstddef>
#include <cstdint>

#pragma pack(push, 1)
struct PhoneCamSharedHeader {
    uint32_t magic;           // 0x4D414350 'PCAM'
    uint16_t layoutVersion;   // 1
    uint16_t headerSize;      // 64
    uint32_t maxWidth;        // 1920
    uint32_t maxHeight;       // 1080
    uint32_t dataOffset;      // 64
    uint32_t dataCapacity;    // maxWidth * maxHeight * 3 / 2
    volatile LONG seq;        // seqlock。奇数 = 書き込み中
    uint32_t reserved0;
    uint32_t width;           // 偶数
    uint32_t height;          // 偶数
    uint32_t pixelFormat;     // 1 = NV12
    uint32_t flags;           // bit0 = SOURCE_ACTIVE
    int64_t timestampUs;      // iPhone のキャプチャ時刻
    int64_t heartbeatTickMs;  // 書き手の GetTickCount64()
};
#pragma pack(pop)

static_assert(sizeof(LONG) == 4);
static_assert(sizeof(PhoneCamSharedHeader) == 64);
static_assert(offsetof(PhoneCamSharedHeader, magic) == 0);
static_assert(offsetof(PhoneCamSharedHeader, layoutVersion) == 4);
static_assert(offsetof(PhoneCamSharedHeader, headerSize) == 6);
static_assert(offsetof(PhoneCamSharedHeader, maxWidth) == 8);
static_assert(offsetof(PhoneCamSharedHeader, maxHeight) == 12);
static_assert(offsetof(PhoneCamSharedHeader, dataOffset) == 16);
static_assert(offsetof(PhoneCamSharedHeader, dataCapacity) == 20);
static_assert(offsetof(PhoneCamSharedHeader, seq) == 24);
static_assert(offsetof(PhoneCamSharedHeader, reserved0) == 28);
static_assert(offsetof(PhoneCamSharedHeader, width) == 32);
static_assert(offsetof(PhoneCamSharedHeader, height) == 36);
static_assert(offsetof(PhoneCamSharedHeader, pixelFormat) == 40);
static_assert(offsetof(PhoneCamSharedHeader, flags) == 44);
static_assert(offsetof(PhoneCamSharedHeader, timestampUs) == 48);
static_assert(offsetof(PhoneCamSharedHeader, heartbeatTickMs) == 56);

// ---- §8.1 オブジェクト ----
inline constexpr wchar_t kPhoneCamFrameMappingName[] = L"Local\\PhoneCam_Frame_v1";

// 推奨セキュリティ記述子（M1 で動作検証する。SPEC §14）
inline constexpr wchar_t kPhoneCamFrameMappingSddl[] = L"D:(A;;GA;;;OW)(A;;GR;;;WD)(A;;GR;;;AC)S:(ML;;NW;;;LW)";

// ---- §8.2 ヘッダの固定値 ----
inline constexpr uint32_t kPhoneCamSharedMagic = 0x4D414350;
inline constexpr uint16_t kPhoneCamLayoutVersion = 1;
inline constexpr uint16_t kPhoneCamHeaderSize = 64;
inline constexpr uint32_t kPhoneCamMaxWidth = 1920;
inline constexpr uint32_t kPhoneCamMaxHeight = 1080;
inline constexpr uint32_t kPhoneCamDataOffset = 64;
inline constexpr uint32_t kPhoneCamDataCapacity = kPhoneCamMaxWidth * kPhoneCamMaxHeight * 3 / 2;
inline constexpr uint32_t kPhoneCamMappingSize = kPhoneCamDataOffset + kPhoneCamDataCapacity;

static_assert(kPhoneCamHeaderSize == sizeof(PhoneCamSharedHeader));
static_assert(kPhoneCamDataCapacity == 3'110'400);

inline constexpr uint32_t kPhoneCamPixelFormatNV12 = 1;
inline constexpr uint32_t kPhoneCamFlagSourceActive = 0x00000001; // flags bit0

// ---- §7.5 / §8.3 タイミングと seqlock ----
inline constexpr uint32_t kPhoneCamHeartbeatIntervalMs = 200; // 書き手の更新間隔
inline constexpr uint64_t kPhoneCamHeartbeatTimeoutMs = 2000; // これより古ければ書き手不在
inline constexpr int kPhoneCamSeqlockMaxRetries = 3;          // 失敗したら前回のフレームを使う
