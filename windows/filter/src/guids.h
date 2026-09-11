// フィルタの識別子（SPEC §9.1）。
// CLSID は M0 で生成したもの。登録済みの環境と食い違うため、以後変更しないこと。
#pragma once

#include <guiddef.h>

// {01325355-763C-4CFF-ADC9-1AF0EB21D0B0}
inline constexpr GUID CLSID_PhoneCamFilter = {
    0x01325355, 0x763c, 0x4cff, {0xad, 0xc9, 0x1a, 0xf0, 0xeb, 0x21, 0xd0, 0xb0}};

inline constexpr wchar_t kPhoneCamFilterClsidString[] = L"{01325355-763C-4CFF-ADC9-1AF0EB21D0B0}";
inline constexpr wchar_t kPhoneCamFilterName[] = L"PhoneCam";
inline constexpr wchar_t kPhoneCamPinName[] = L"Capture";
