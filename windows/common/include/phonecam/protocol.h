// 通信プロトコル v1 の定数と構造体（SPEC §5）。
// ios/PhoneCam/Protocol/、tools/probe.py、SPEC と必ず一致させること。
#pragma once

#include <bit>
#include <cstddef>
#include <cstdint>

// §5.1 マルチバイト整数はすべてリトルエンディアン。構造体をそのまま読み書きする前提。
static_assert(std::endian::native == std::endian::little);

namespace phonecam::protocol {

// ---- §5.1 基本事項 ----
inline constexpr uint16_t kDefaultPort = 48620;
inline constexpr uint32_t kMaxPayloadLength = 16u * 1024u * 1024u; // 超えたら切断

// ---- §5.2 Bonjour ----
inline constexpr uint32_t kProtocolMajorVersion = 1; // TXT "pv"
inline constexpr char kServiceType[] = "_phonecam._tcp";
inline constexpr wchar_t kServiceQueryName[] = L"_phonecam._tcp.local"; // DnsServiceBrowse 用
inline constexpr wchar_t kTxtKeyProtocolVersion[] = L"pv";
inline constexpr wchar_t kTxtKeyDeviceId[] = L"id";
inline constexpr wchar_t kTxtKeyAppVersion[] = L"app";
inline constexpr wchar_t kTxtKeyModel[] = L"model";

// ---- §5.3 パケットヘッダ（24 バイト固定）----
inline constexpr uint32_t kPacketMagic = 0x4D414350; // バイト列 'P' 'C' 'A' 'M'
inline constexpr uint8_t kPacketVersion = 1;

#pragma pack(push, 1)
struct PacketHeader {
    uint32_t magic;         // kPacketMagic
    uint8_t version;        // kPacketVersion
    uint8_t type;           // MessageType
    uint16_t flags;         // type ごとに定義。未定義ビットは 0
    uint32_t payloadLength; // kMaxPayloadLength 以下
    uint32_t sequence;      // 送信側で type を問わず単調増加
    int64_t timestampUs;    // 用途は type ごと。未使用なら 0
};
#pragma pack(pop)

static_assert(sizeof(PacketHeader) == 24);
static_assert(offsetof(PacketHeader, magic) == 0);
static_assert(offsetof(PacketHeader, version) == 4);
static_assert(offsetof(PacketHeader, type) == 5);
static_assert(offsetof(PacketHeader, flags) == 6);
static_assert(offsetof(PacketHeader, payloadLength) == 8);
static_assert(offsetof(PacketHeader, sequence) == 12);
static_assert(offsetof(PacketHeader, timestampUs) == 16);

// ---- §5.4 メッセージ種別 ----
// 未知の type は payloadLength ぶん読み飛ばして無視する。
enum class MessageType : uint8_t {
    ClientHello = 0x01,     // PC → iPhone, JSON
    ServerHello = 0x02,     // iPhone → PC, JSON
    VideoConfig = 0x10,     // iPhone → PC, VideoConfigHeader + SPS/PPS (Annex B)
    VideoFrame = 0x11,      // iPhone → PC, H.264 Annex B（1 アクセスユニット）
    SetSettings = 0x20,     // PC → iPhone, JSON
    RequestKeyframe = 0x21, // PC → iPhone, ペイロードなし
    Control = 0x22,         // PC → iPhone, JSON
    Status = 0x23,          // iPhone → PC, JSON
    Ping = 0x30,            // PC → iPhone, 8 バイト（PC 時刻 µs）
    Pong = 0x31,            // iPhone → PC, PING のペイロードをそのまま返す
    Error = 0x7F,           // 双方向, JSON
};

// ---- §5.5 シーケンスとタイムアウト ----
inline constexpr uint32_t kHelloTimeoutMs = 3000;   // CLIENT_HELLO / SERVER_HELLO の待ち時間
inline constexpr uint32_t kIdleTimeoutMs = 5000;    // 無通信タイムアウト
inline constexpr uint32_t kPingIntervalMs = 1000;   // PC → iPhone
inline constexpr uint32_t kStatusIntervalMs = 10000; // iPhone → PC の定期 STATUS

// ---- §5.6 ペイロード ----
// VIDEO_FRAME flags
inline constexpr uint16_t kVideoFrameFlagKeyframe = 0x0001;

// VIDEO_CONFIG 固定部。直後に SPS / PPS を Annex B で連結したバイト列が続く。
inline constexpr uint8_t kCodecH264 = 1;

#pragma pack(push, 1)
struct VideoConfigHeader {
    uint8_t codec;       // kCodecH264
    uint8_t reserved[3]; // 0
    uint32_t width;
    uint32_t height;
    uint32_t fps;
};
#pragma pack(pop)

static_assert(sizeof(VideoConfigHeader) == 16);
static_assert(offsetof(VideoConfigHeader, codec) == 0);
static_assert(offsetof(VideoConfigHeader, width) == 4);
static_assert(offsetof(VideoConfigHeader, height) == 8);
static_assert(offsetof(VideoConfigHeader, fps) == 12);

// PING / PONG
inline constexpr uint32_t kPingPayloadSize = 8;

// ERROR の code
namespace error_code {
inline constexpr char kVersionMismatch[] = "version_mismatch";
inline constexpr char kReplaced[] = "replaced";
inline constexpr char kBadRequest[] = "bad_request";
inline constexpr char kCameraUnavailable[] = "camera_unavailable";
inline constexpr char kInternal[] = "internal";
} // namespace error_code

} // namespace phonecam::protocol
