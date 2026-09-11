// testpattern: 共有メモリにテストパターンを書き込み、フィルタを iPhone なしで検証するツール（SPEC §12.1）。
// M0: ビルド確認用の空実装。テストパターンの書き込みは M1 で実装する。
#include <windows.h>

#include <cwchar>

#include <libyuv.h>
#include <wil/resource.h>

#include <phonecam/shared_frame.h>
#include <phonecam/version.h>

int wmain() {
    std::wprintf(L"PhoneCam testpattern %hs (M0 stub)\n", PHONECAM_VERSION_STRING);
    std::wprintf(L"  mapping %ls, %u bytes\n", kPhoneCamFrameMappingName, kPhoneCamMappingSize);
    return 0;
}
