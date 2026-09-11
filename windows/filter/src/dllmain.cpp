// PhoneCamFilter.dll のエントリポイント。
// M0: エクスポートだけを持つ空実装。DirectShow フィルタ本体（CSource / CSourceStream）は M1 で実装する。
#include <windows.h>
#include <olectl.h>

#include <streams.h> // BaseClasses を読み込めることの確認
#include <libyuv.h>
#include <wil/resource.h>

#include <phonecam/shared_frame.h>

#include "guids.h"

BOOL APIENTRY DllMain(HINSTANCE, DWORD, LPVOID) {
    return TRUE;
}

STDAPI DllGetClassObject(REFCLSID, REFIID, LPVOID *ppv) {
    if (ppv) {
        *ppv = nullptr;
    }
    return CLASS_E_CLASSNOTAVAILABLE;
}

STDAPI DllCanUnloadNow() {
    return S_OK;
}

STDAPI DllRegisterServer() {
    return E_NOTIMPL;
}

STDAPI DllUnregisterServer() {
    return E_NOTIMPL;
}
