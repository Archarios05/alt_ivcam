// Receiver.exe のエントリポイント。
// M0: 依存ライブラリ（FFmpeg / Dear ImGui / libyuv / WIL）のリンク確認のみ。本体は M3 で実装する。
#include <windows.h>

#include <cstdint>
#include <cstdio>

extern "C" {
#include <libavcodec/avcodec.h>
}
#include <imgui.h>
#include <libyuv.h>
#include <wil/resource.h>

#include <phonecam/protocol.h>
#include <phonecam/shared_frame.h>
#include <phonecam/version.h>

int wmain() {
    const AVCodec *h264 = avcodec_find_decoder(AV_CODEC_ID_H264);

    // libyuv のリンク確認（2x2 の I420 → NV12）
    uint8_t y[4] = {16, 16, 16, 16}, u[1] = {128}, v[1] = {128};
    uint8_t nvY[4] = {}, nvUV[2] = {};
    const int yuvResult = libyuv::I420ToNV12(y, 2, u, 1, v, 1, nvY, 2, nvUV, 2, 2, 2);

    std::printf("PhoneCam Receiver %s (M0 stub)\n", PHONECAM_VERSION_STRING);
    std::printf("  protocol v%u, default port %u\n", phonecam::protocol::kProtocolMajorVersion,
                phonecam::protocol::kDefaultPort);
    std::printf("  avcodec %u, H.264 decoder: %s\n", avcodec_version(), h264 ? "available" : "missing");
    std::printf("  Dear ImGui %s\n", ImGui::GetVersion());
    std::printf("  libyuv I420ToNV12: %d\n", yuvResult);

    return (h264 && yuvResult == 0) ? 0 : 1;
}
