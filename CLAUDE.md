# PhoneCam

iPhone のカメラを、同一 LAN 上の Windows で DirectShow 仮想カメラとして使うソフトウェア（iVCam 代替）。
iPhone がサーバ（Bonjour 広告 + TCP 待ち受け）、Windows の Receiver がクライアント。
Receiver がデコードした映像を共有メモリに書き、DirectShow フィルタ DLL が各アプリ内でそれを読む。

**仕様は `docs/SPEC.md` が唯一の正。作業前に必ず該当節を読むこと。**

## 現在のマイルストーン

M0（足場づくり）。受け入れ基準は SPEC §11。

## 作業ルール

- 1 回の作業は 1 マイルストーンの範囲に留める。受け入れ基準を満たしたら SPEC §11 のチェックボックスを更新する。
- 仕様と矛盾する実装が必要になったら、コードを書く前に SPEC.md の変更案を示して確認を取る。
- 通信プロトコル（SPEC §5）と共有メモリレイアウト（SPEC §8）は、`windows/common/include/phonecam/`、`ios/PhoneCam/Protocol/`、`tools/probe.py` の 3 箇所で必ず一致させる。変更時は 3 箇所と SPEC を同時に更新する。
- `regsvr32`、`scripts/register.ps1` など管理者権限やシステム登録を伴う操作は実行しない。実行すべきコマンドを提示し、開発者に依頼する。
- フィルタ CLSID は一度生成したら変更しない。
- フィルタ DLL に FFmpeg / ImGui などの重い依存を入れない。CRT は `/MT`。COM メソッドから例外を外に出さない。
- iOS アプリはこの Windows 環境ではビルドできない。Swift を変更した場合は「未ビルド・未検証」であることを明記し、Mac で確認すべき点を列挙する。
- ffplay やアプリでの目視確認が必要な受け入れ基準は、確認手順を提示して開発者に結果を尋ねる。

## ビルド（Windows）

```powershell
cmake --preset x64-debug   && cmake --build --preset x64-debug     # receiver, filter, testpattern
cmake --preset x86-release && cmake --build --preset x86-release   # filter (x86) のみ
```

## 主要ディレクトリ

- `windows/common/include/phonecam/` … protocol.h / shared_frame.h（両プロセス共通の定義）
- `windows/receiver/` … Receiver.exe（x64）
- `windows/filter/` … PhoneCamFilter.dll（x86 / x64）
- `windows/tools/testpattern/` … フィルタ単体検証用
- `ios/PhoneCam/` … iOS アプリ（Swift / SwiftUI、Mac でビルド）
- `tools/probe.py` … iOS アプリ単体検証用クライアント

## コーディング規約

- C++20、`.clang-format` に従う。ハンドル類は WIL の RAII 型で管理する。
- 32bit / 64bit 間で共有する構造体は固定幅の型のみ、`#pragma pack(1)` と `static_assert(sizeof)` を付ける。
- Swift 5 言語モード。キャプチャ・エンコードはメインスレッド以外のシリアルキューで行う。
- ログ・コメントは日本語可。識別子は英語。
