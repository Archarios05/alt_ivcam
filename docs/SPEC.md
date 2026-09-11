# PhoneCam 仕様書 v0.1

iPhone のカメラを Windows の Web カメラとして使うためのソフトウェア（iVCam 代替）の仕様書。

---

## 0. この文書について

- **対象読者**: 実装を担当する Claude Code（VS Code 版）と開発者。
- **位置づけ**: 本書が仕様の唯一の正（single source of truth）。実装が本書と矛盾する必要が出た場合は、コードより先に本書の改訂を提案し、開発者の承認を得てから変更する。
- **仮称**: 製品名は仮に `PhoneCam` とする。名称変更時は識別子・サービス種別・共有メモリ名などを一括置換する（§14 参照）。
- **表記**: 「必須」「推奨」「任意」で要件の強さを示す。`TBD` は未決定事項で §14 に集約する。

---

## 1. 概要

### 1.1 目的

同一 LAN 上の iPhone を Bonjour で自動発見し、そのカメラ映像を低遅延で Windows に送り、DirectShow 仮想カメラとして Zoom / Teams / Discord / OBS / ブラウザ等から利用できるようにする。

### 1.2 確定済みの方針

| 項目 | 決定内容 |
|---|---|
| 接続方式 | 同一 LAN のみ。Bonjour（DNS-SD）で探索し TCP で接続 |
| 役割 | iPhone = サーバ（待ち受け・Bonjour 広告）、PC = クライアント（探索・接続） |
| 仮想カメラ方式 | DirectShow ソースフィルタ（in-proc COM DLL、x86 / x64 の両ビルド） |
| 対応 Windows | Windows 10 22H2 以降（x64 OS）、Windows 11 |
| 対応 iOS | iOS 16.0 以降 |
| 映像コーデック | H.264（ハードウェアエンコード / PC 側は FFmpeg でデコード） |
| 出力フレームレート | 30 fps 固定（v1） |

### 1.3 v1 のスコープ外

USB 有線接続、音声・仮想マイク、HEVC、60 fps、PC 側ハードウェアデコード、Media Foundation 仮想カメラ（Windows 11 の MF 専用アプリ対応）、縦長出力、複数 iPhone の同時利用、macOS 受信側。

---

## 2. システム構成

```
┌──────────────── iPhone アプリ (Swift) ────────────────┐
│ Capture ──▶ H264Encoder ──▶ StreamServer (NWListener) │
│ (AVFoundation)  (VideoToolbox)   + Bonjour 広告        │
└───────────────────────────┬───────────────────────────┘
                            │ TCP（§5 独自プロトコル）
┌──────────────── Windows ──┴───────────────────────────┐
│ Receiver.exe (x64)                                     │
│   Discovery ─▶ Connection ─▶ Decoder ─▶ SharedFrame 書込│
│   (DnsServiceBrowse)          (FFmpeg)   + プレビュー UI│
│                                             │          │
│                          名前付き共有メモリ（§8）       │
│                                             ▼          │
│ PhoneCamFilter.dll (x86/x64) … 各アプリのプロセス内で読込│
│   SharedFrame 読取 ─▶ 変換・拡縮 ─▶ DirectShow 出力ピン  │
└────────────────────────────────────────────────────────┘
```

| コンポーネント | 言語 / 形態 | 責務 |
|---|---|---|
| iOS アプリ | Swift / SwiftUI | カメラ取得、H.264 エンコード、Bonjour 広告、TCP 送出、端末操作 |
| Receiver | C++20 / x64 exe | 探索、接続管理、デコード、共有メモリ書き込み、プレビュー、設定 UI |
| Filter | C++20 / x86・x64 DLL | DirectShow 仮想カメラ。共有メモリから読み、アプリに映像を供給 |
| tools/testpattern | C++20 / x64 exe | 共有メモリにテストパターンを書く（M1 でフィルタ単体検証用） |
| tools/probe.py | Python 3.10+ | iOS アプリ単体検証用クライアント（M2 用） |

---

## 3. リポジトリ構成

```
phonecam/
├─ CLAUDE.md
├─ docs/
│  └─ SPEC.md                       … 本書
├─ ios/
│  └─ PhoneCam/                     … Xcode プロジェクト
│     ├─ App/PhoneCamApp.swift
│     ├─ UI/ContentView.swift, SettingsView.swift, DimOverlayView.swift
│     ├─ Core/StreamCoordinator.swift   … 状態機械（§6.5）
│     ├─ Core/SettingsStore.swift, ThermalMonitor.swift
│     ├─ Capture/CaptureController.swift
│     ├─ Encode/H264Encoder.swift
│     ├─ Network/StreamServer.swift, ClientSession.swift
│     └─ Protocol/Packet.swift, Messages.swift
├─ windows/
│  ├─ CMakeLists.txt
│  ├─ CMakePresets.json
│  ├─ vcpkg.json
│  ├─ common/include/phonecam/
│  │  ├─ protocol.h                 … §5 の定数・構造体
│  │  ├─ shared_frame.h             … §8 の構造体・名前
│  │  └─ version.h
│  ├─ receiver/src/
│  │  ├─ main.cpp
│  │  ├─ discovery/                 … DnsServiceBrowse / Resolve
│  │  ├─ net/                       … Winsock 接続、パケット解析
│  │  ├─ decode/                    … FFmpeg ラッパ
│  │  ├─ shm/                       … 共有メモリ書き込み
│  │  └─ ui/                        … Dear ImGui + D3D11
│  ├─ filter/
│  │  ├─ filter.def
│  │  └─ src/ dllmain.cpp, filter.*, pin.*, formats.*, shm_reader.*, placeholder.*, register.cpp
│  ├─ tools/testpattern/
│  └─ third_party/baseclasses/       … DirectShow BaseClasses（ベンダリング）
├─ tools/probe.py
├─ scripts/
│  ├─ register.ps1                  … 管理者権限で x86/x64 DLL を regsvr32
│  └─ unregister.ps1
└─ installer/phonecam.iss           … Inno Setup（M5）
```

---

## 4. 開発環境・ビルド

### 4.1 Windows

| 項目 | 内容 |
|---|---|
| コンパイラ | Visual Studio 2022 Build Tools（MSVC v143）、Windows SDK 10.0.19041 以降 |
| ビルド | CMake 3.25 以降 + Ninja、`CMakePresets.json` |
| 依存管理 | vcpkg（manifest モード） |
| エディタ | VS Code + CMake Tools + C/C++ 拡張 |

依存ライブラリ:

| ライブラリ | 用途 | 使用箇所 | 導入 |
|---|---|---|---|
| DirectShow BaseClasses | フィルタ基底クラス | filter | Windows-classic-samples からベンダリング（MIT） |
| libyuv | 色変換・拡縮 | filter / receiver / testpattern | vcpkg `libyuv` |
| WIL | RAII ハンドル等 | 全 Windows コード | vcpkg `wil` |
| FFmpeg (avcodec, avutil) | H.264 デコード | receiver のみ | vcpkg `ffmpeg`（GPL 機能は無効、DLL 動的リンク） |
| Dear ImGui | UI | receiver のみ | vcpkg `imgui[dx11-binding,win32-binding]` |

vcpkg の manifest feature `receiver` に FFmpeg と ImGui を入れ、x86 ビルドでは取得しない。

プリセット（必須）:

| プリセット | アーキテクチャ | ビルド対象 |
|---|---|---|
| `x64-debug` | x64 | receiver, filter, testpattern |
| `x64-release` | x64 | receiver, filter, testpattern |
| `x86-release` | x86 | filter のみ（`PHONECAM_BUILD_RECEIVER=OFF`） |

```powershell
cmake --preset x64-debug   && cmake --build --preset x64-debug
cmake --preset x86-release && cmake --build --preset x86-release
```

### 4.2 iOS

- macOS + Xcode 15 以降が必須。**Windows 上では iOS アプリをビルドできない**。Swift コードの編集は Windows 側でも行えるが、ビルド・実機確認は Mac で行う。
- Swift 5 言語モード、SwiftUI、外部依存なし（Apple 標準フレームワークのみ）。
- 実機テストは Apple ID の開発署名で可。配布は Apple Developer Program が必要。

### 4.3 共通

- C++: `.clang-format`（LLVM ベース、インデント 4、列 120）。例外を COM 境界の外に出さない。
- Swift: Xcode 標準フォーマットに準拠。
- 文字コード UTF-8、改行 LF（`.sln` 等 Windows 固有ファイルを除く）。

---

## 5. 通信プロトコル（v1）

### 5.1 基本事項

- トランスポートは TCP。iPhone・PC の両側で `TCP_NODELAY` を有効にする（必須）。
- **マルチバイト整数はすべてリトルエンディアン**。
- iPhone は既定ポート `48620` で待ち受ける。使用中の場合は OS 割り当てポートにフォールバックし、実ポートを Bonjour で広告する。手動 IP 接続時は既定ポートを使う。
- 同時に扱う接続は 1 本。新しい接続が来た場合、既存接続に `ERROR{code:"replaced"}` を送って切断し、新しい接続を採用する。

### 5.2 Bonjour

| 項目 | 値 |
|---|---|
| サービス種別 | `_phonecam._tcp`（ドメイン `local.`） |
| インスタンス名 | ユーザーがアプリで設定した表示名（既定 `iPhone`）。衝突時は OS のリネームに従う |
| TXT `pv` | プロトコルのメジャーバージョン（`"1"`） |
| TXT `id` | 端末 ID（アプリ初回起動時に生成して保存する UUID 文字列） |
| TXT `app` | アプリバージョン（例 `"0.1.0"`） |
| TXT `model` | 機種識別子（例 `"iPhone15,2"`） |

PC は `pv` が自分の対応メジャーバージョンと異なる端末を「非対応バージョン」として一覧に灰色表示し、接続しない。

### 5.3 パケットヘッダ（24 バイト固定）

| オフセット | サイズ | 型 | フィールド | 内容 |
|---|---|---|---|---|
| 0 | 4 | u32 | magic | `0x4D414350`（バイト列 `P` `C` `A` `M`） |
| 4 | 1 | u8 | version | `1` |
| 5 | 1 | u8 | type | §5.4 |
| 6 | 2 | u16 | flags | type ごとに定義。未定義ビットは 0 |
| 8 | 4 | u32 | payloadLength | ペイロード長。上限 16 MiB を超えたら切断 |
| 12 | 4 | u32 | sequence | 送信側で type を問わず単調増加 |
| 16 | 8 | i64 | timestampUs | 用途は type ごと。未使用なら 0 |

C++ では `#pragma pack(1)` の構造体に `static_assert(sizeof == 24)` を付ける。Swift ではバイト列を明示的に組み立てる。

### 5.4 メッセージ種別

| type | 名前 | 方向 | ペイロード |
|---|---|---|---|
| 0x01 | CLIENT_HELLO | PC → iPhone | JSON |
| 0x02 | SERVER_HELLO | iPhone → PC | JSON |
| 0x10 | VIDEO_CONFIG | iPhone → PC | バイナリ |
| 0x11 | VIDEO_FRAME | iPhone → PC | H.264 Annex B |
| 0x20 | SET_SETTINGS | PC → iPhone | JSON |
| 0x21 | REQUEST_KEYFRAME | PC → iPhone | なし |
| 0x22 | CONTROL | PC → iPhone | JSON |
| 0x23 | STATUS | iPhone → PC | JSON |
| 0x30 | PING | PC → iPhone | 8 バイト（PC 時刻 µs） |
| 0x31 | PONG | iPhone → PC | PING のペイロードをそのまま返す |
| 0x7F | ERROR | 双方向 | JSON |

未知の type は payloadLength ぶん読み飛ばして無視する（前方互換、必須）。

### 5.5 シーケンス

```
PC                                  iPhone
 │── TCP connect ─────────────────────▶│
 │── CLIENT_HELLO ────────────────────▶│  (接続後 3 秒以内。超過で iPhone が切断)
 │◀──────────────────── SERVER_HELLO ──│  (受信後 3 秒以内。超過で PC が切断)
 │── SET_SETTINGS (任意) ─────────────▶│
 │◀──────────────────── VIDEO_CONFIG ──│
 │◀──────────────────── STATUS ────────│
 │◀──────────────────── VIDEO_FRAME* ──│  (最初の 1 枚は必ずキーフレーム)
 │── PING (1 秒ごと) ─────────────────▶│
 │◀──────────────────── PONG ──────────│
```

- 無通信タイムアウト: PC は 5 秒間何も受信しなければ切断し再接続へ。iPhone は 5 秒間 PING を受信しなければ切断し待ち受けに戻る。
- プロトコルのメジャーバージョン不一致時は `ERROR{code:"version_mismatch"}` を送って切断。

### 5.6 ペイロード詳細

**CLIENT_HELLO**
```json
{ "protocolVersion": 1, "clientName": "DESKTOP-ABC123", "clientVersion": "0.1.0" }
```

**SERVER_HELLO**
```json
{
  "protocolVersion": 1,
  "deviceId": "2F4E...UUID",
  "deviceName": "iPhone",
  "model": "iPhone15,2",
  "appVersion": "0.1.0",
  "capabilities": {
    "resolutions": [[1920, 1080], [1280, 720]],
    "fps": [30],
    "cameras": ["back", "front"],
    "torch": true,
    "maxZoom": 10.0
  },
  "settings": { "width": 1280, "height": 720, "fps": 30, "bitrateKbps": 4000, "camera": "back" }
}
```

**VIDEO_CONFIG**（バイナリ）

| オフセット | サイズ | 内容 |
|---|---|---|
| 0 | 1 | codec（`1` = H.264） |
| 1 | 3 | 予約（0） |
| 4 | 4 | width（u32） |
| 8 | 4 | height（u32） |
| 12 | 4 | fps（u32） |
| 16 | 可変 | SPS / PPS を Annex B（`00 00 00 01` 開始コード付き）で連結 |

ストリーム開始時と、解像度・カメラ等の変更でフォーマットが変わった時に送る。

**VIDEO_FRAME**
- ペイロードは 1 アクセスユニット（1 フレーム）を Annex B 形式で格納。VideoToolbox の AVCC（長さプレフィックス）から変換して送る。
- flags bit0 = `KEYFRAME`。キーフレームは先頭に SPS / PPS を必ず付ける（途中参加でもデコード開始できるようにする）。
- `timestampUs` = キャプチャ時刻（iPhone のホストクロック、µs）。PC は表示タイミングには使わず、統計用にのみ使う。

**SET_SETTINGS**（部分指定可。指定されたキーのみ変更）
```json
{ "width": 1920, "height": 1080, "fps": 30, "bitrateKbps": 8000, "camera": "front" }
```
iPhone は適用後に `STATUS` で実際の設定を返し、フォーマットが変わった場合は `VIDEO_CONFIG` → キーフレームの順で送る。能力外の値は近い値に丸めて適用する。

**CONTROL**
```json
{ "action": "zoom", "value": 2.0 }
{ "action": "torch", "value": true }
```

**STATUS**
```json
{
  "state": "streaming",
  "settings": { "width": 1280, "height": 720, "fps": 30, "bitrateKbps": 4000, "camera": "back" },
  "thermalState": "nominal",
  "throttled": false,
  "batteryLevel": 0.82,
  "charging": true
}
```
`state` は `streaming` / `interrupted`（カメラ中断中）のいずれか。状態・設定・熱状態が変化した時、および 10 秒ごとに送る。

**ERROR**
```json
{ "code": "replaced", "message": "another client connected" }
```
`code` の定義値: `version_mismatch`, `replaced`, `bad_request`, `camera_unavailable`, `internal`。

### 5.7 輻輳制御（バックプレッシャ）

遅延の蓄積を防ぐため、古いフレームを溜めずに捨てる。

**iPhone 側（必須）**
- `inFlight` = 送信を依頼したが `NWConnection.send` の完了ハンドラがまだ呼ばれていないフレーム数。
- 新しい非キーフレームの時点で `inFlight >= 3` なら、そのフレームを破棄し `needKeyframe = true` にする。
- `inFlight >= 6` ならキーフレームも破棄する。
- `needKeyframe == true` かつ `inFlight < 2` になったら、次のエンコードで `kVTEncodeFrameOptionKey_ForceKeyFrame` を指定する。
- 破棄数を統計として保持し、STATUS に含めてもよい（任意）。

**PC 側（必須）**
- 受信スレッドとデコードスレッドの間のキューは最大 2 アクセスユニット。
- 溢れた場合はキューを空にし、次のキーフレームまで破棄する。同時に `REQUEST_KEYFRAME` を送る（1 秒に 1 回まで）。
- デコードエラー時も同様に、キーフレームまで破棄して `REQUEST_KEYFRAME` を送る。

---

## 6. iOS アプリ仕様

### 6.1 モジュール

| モジュール | 責務 |
|---|---|
| `StreamCoordinator` | 状態機械。Capture / Encoder / Server を統括。設定変更の適用順序を管理 |
| `CaptureController` | `AVCaptureSession` の構成、カメラ切替、ズーム、トーチ、向き補正 |
| `H264Encoder` | `VTCompressionSession` の管理、AVCC → Annex B 変換、SPS/PPS 抽出 |
| `StreamServer` | `NWListener` による待ち受けと Bonjour 広告、接続の差し替え |
| `ClientSession` | 1 接続分のパケット送受信、ハンドシェイク、PING 監視、バックプレッシャ |
| `Packet` / `Messages` | §5 のエンコード / デコード |
| `ThermalMonitor` | 熱状態の監視と §6.6 のポリシー適用 |
| `SettingsStore` | `UserDefaults` への設定・端末 ID・表示名の保存 |

キャプチャとエンコードは専用のシリアル `DispatchQueue`（QoS `.userInteractive`）で行い、メインスレッドを使わない。

### 6.2 キャプチャ

| 項目 | 設定 |
|---|---|
| ピクセル形式 | `kCVPixelFormatType_420YpCbCr8BiPlanarVideoRange` |
| 解像度 | 1920×1080 / 1280×720（セッションプリセットまたは `activeFormat` で選択） |
| フレームレート | `activeVideoMinFrameDuration` と `activeVideoMaxFrameDuration` を 1/30 秒に固定 |
| 遅延対策 | `alwaysDiscardsLateVideoFrames = true`、`preferredVideoStabilizationMode = .off` |
| カメラ | `back`（背面広角、既定）/ `front` |
| 向き | 出力は常に横長。iOS 17 以降は `AVCaptureDevice.RotationCoordinator` と `videoRotationAngle`、iOS 16 は `videoOrientation` で端末の向きを補正する |
| ミラー | 前面カメラでも反転しない（反転表示は会議アプリ側が行う） |
| プレビュー | `AVCaptureVideoPreviewLayer` を画面に表示 |

### 6.3 エンコード

| 項目 | 設定 |
|---|---|
| エンコーダ指定 | `kVTVideoEncoderSpecification_EnableLowLatencyRateControl = true` |
| プロファイル | `kVTProfileLevel_H264_High_AutoLevel` |
| `RealTime` | `true` |
| `AllowFrameReordering` | `false`（B フレームなし） |
| `ExpectedFrameRate` | 30 |
| `AverageBitRate` | 既定 1080p: 8000 kbps、720p: 4000 kbps。ユーザー設定範囲 1000〜20000 kbps |
| キーフレーム間隔 | `MaxKeyFrameInterval = 120`、`MaxKeyFrameIntervalDuration = 4` 秒。加えて要求時に強制発行 |
| 色 | BT.709、リミテッドレンジ |

SPS / PPS は `CMVideoFormatDescriptionGetH264ParameterSetAtIndex` で取得する。解像度・カメラ変更時はセッションを作り直し、`VIDEO_CONFIG` を送ってからキーフレームを送る。

### 6.4 待ち受けと Bonjour

- `NWListener`（パラメータは `.tcp`、`noDelay = true`）。§5.2 の `NWListener.Service` を設定する。
- `serviceRegistrationUpdateHandler` で実際の登録名を取得し、UI に表示する。
- ローカルネットワーク許可が拒否された場合（Listener の状態が `.waiting` / `.failed` で、エラーが `kDNSServiceErr_PolicyDenied` 相当）は、設定アプリへの誘導画面を表示する。

### 6.5 状態機械（`StreamCoordinator`）

| 状態 | 説明 | 遷移 |
|---|---|---|
| `idle` | 起動直後・権限未取得 | カメラ権限とローカルネットワーク権限を得たら `advertising` |
| `advertising` | Bonjour 広告中、接続待ち。カメラはプレビューのため稼働 | 接続・ハンドシェイク成功で `streaming` |
| `streaming` | 送出中 | 切断で `advertising`、カメラ中断で `interrupted` |
| `interrupted` | カメラ中断中（他アプリ使用・通話等） | 復帰で `streaming`、切断で `advertising` |
| `error` | 回復不能なエラー | ユーザー操作で `idle` からやり直し |

### 6.6 ライフサイクルと熱対策

- `advertising` / `streaming` 中は `UIApplication.shared.isIdleTimerDisabled = true`。
- バックグラウンド移行時はカメラが止まるため、`STATUS{state:"interrupted"}` を送ってから接続を閉じ、フォアグラウンド復帰時に広告を再開する。
- **画面減光モード**（必須）: 黒の全画面オーバーレイと画面輝度の最小化。タップで解除し、元の輝度に戻す。
- 熱ポリシー（`ProcessInfo.thermalStateDidChangeNotification` を監視）:

| 熱状態 | 動作 |
|---|---|
| `nominal` / `fair` | ユーザー設定どおり |
| `serious` | 1280×720 に下げ、ビットレートを設定値の 60% に |
| `critical` | 1280×720・15 fps・ビットレート 40% に |
| 回復 | `fair` 以下が 60 秒続いたら設定値に戻す |

制限中は STATUS の `throttled = true` とし、PC 側 UI に表示する。

### 6.7 画面（SwiftUI）

- **メイン画面**: カメラプレビュー、状態表示（接続待ち / 接続中: PC 名 / 中断中）、表示名、カメラ切替ボタン、減光ボタン、設定ボタン。
- **設定画面**: 表示名、既定解像度、既定ビットレート、既定カメラ、アプリ情報。
- 画面の向きは縦横どちらでもよい（出力は §6.2 のとおり常に横長）。

### 6.8 Info.plist

| キー | 値 |
|---|---|
| `NSCameraUsageDescription` | カメラ映像を PC に送るために使用します |
| `NSLocalNetworkUsageDescription` | 同じネットワーク上の PC と接続するために使用します |
| `NSBonjourServices` | `["_phonecam._tcp"]` |
| `UIApplicationSupportsIndirectInputEvents` | 既定のまま |

---

## 7. Windows 受信アプリ仕様（Receiver.exe）

### 7.1 モジュール

| モジュール | 責務 |
|---|---|
| `discovery` | `DnsServiceBrowse` / `DnsServiceResolve` による端末一覧の管理 |
| `net` | Winsock 接続、ハンドシェイク、パケット解析、PING、再接続 |
| `decode` | FFmpeg による H.264 デコード、I420 → NV12 変換 |
| `shm` | 共有メモリの作成と書き込み（§8） |
| `ui` | Dear ImGui + Direct3D 11 のウィンドウ |
| `app` | 設定、ログ、モジュール間の配線 |

スレッドは UI（メイン）、探索コールバック、受信、デコード + 共有メモリ書き込みの 4 系統を基本とする。

### 7.2 探索

- `DnsServiceBrowse` で `_phonecam._tcp.local` を検索し、PTR レコードのインスタンス名を `DnsServiceResolve` で解決して IPv4 アドレス・ポート・TXT を得る。IPv6 は v1 では使わない。
- 10 秒ごとに検索をやり直し、30 秒間見つからない端末は一覧から消す。接続失敗も「消えた」とみなしてよい。
- 端末は TXT の `id` で同一性を判定する（表示名の変更に追従するため）。
- 手動接続: IP アドレス（とポート、既定 48620）を入力して接続できる（必須）。
- `INetworkListManager` で接続中ネットワークのカテゴリを調べ、パブリックの場合は「mDNS がファイアウォールで遮断される可能性があります。ネットワークをプライベートに変更してください」と UI に警告を出す。

### 7.3 接続

- Winsock TCP クライアント。`TCP_NODELAY` 有効、受信バッファ 1 MiB。
- 自動接続: 前回接続した端末 `id` が一覧に現れたら自動で接続する（設定で ON / OFF、既定 ON）。
- 切断時の再接続: 1, 2, 4, 8 秒…と間隔を延ばし、最大 10 秒間隔で繰り返す。ユーザーが明示的に切断した場合は再接続しない。

### 7.4 デコード

- FFmpeg の `h264` デコーダ。`AV_CODEC_FLAG_LOW_DELAY` を指定し、`thread_type = FF_THREAD_SLICE`（フレームスレッドは遅延が増えるため使わない）。
- 受信した VIDEO_FRAME のペイロードを 1 パケットとしてそのまま `avcodec_send_packet` に渡す。
- 出力（通常 `yuv420p`）を libyuv で NV12 に変換し、共有メモリに書き込む。
- §5.7 の PC 側ルールに従う。

### 7.5 共有メモリ書き込み

- 起動時に §8 の共有メモリを作成する。既に存在する（多重起動）場合は、既存インスタンスをアクティブにして終了する。
- 映像はストリームの解像度のまま書き込む（拡縮はフィルタ側で行う）。
- 映像の有無にかかわらず、200 ms ごとに `heartbeatTickMs` を更新する。映像受信中のみ `flags.SOURCE_ACTIVE` を立てる。

### 7.6 UI

- **端末一覧**: 表示名、IP、状態（接続可 / 非対応バージョン / 接続中）、接続 / 切断ボタン、手動接続の入力欄。
- **プレビュー**: デコード結果を表示（NV12 → RGBA を libyuv で変換し、D3D11 動的テクスチャに書き込む）。
- **統計**: 受信 fps、ビットレート、RTT（PING / PONG）、破棄フレーム数、iPhone の熱状態・バッテリー。
- **操作**: 解像度、ビットレート、カメラ切替、ズーム、トーチ。
- **警告表示**: パブリックネットワーク、iPhone 側の熱制限、フィルタ未登録（`CLSID_PhoneCamFilter` がレジストリにない場合）。
- ウィンドウを閉じるとアプリを終了する（タスクトレイ常駐は v1 ではスコープ外）。

### 7.7 設定とログ

- 設定: `%APPDATA%\PhoneCam\settings.json`（前回の端末 ID、自動接続、既定の解像度・ビットレート、ウィンドウ位置）。
- ログ: `%LOCALAPPDATA%\PhoneCam\logs\receiver.log`（1 MiB でローテーション、3 世代）。

---

## 8. 共有メモリ仕様

### 8.1 オブジェクト

| 項目 | 値 |
|---|---|
| ファイルマッピング名 | `Local\PhoneCam_Frame_v1` |
| 作成者 | Receiver（testpattern も同じ名前で作成してよい。同時起動は想定しない） |
| 読み手 | フィルタ DLL（アプリごとに 1 インスタンス、同時に複数） |
| サイズ | ヘッダ 64 バイト + NV12 最大領域（1920×1080×3/2 = 3,110,400 バイト） |
| 通知 | なし。フィルタは自分のフレーム周期で最新フレームを取りに行く（§9.6） |

セキュリティ記述子（推奨、実装時に動作検証すること）:
`D:(A;;GA;;;OW)(A;;GR;;;WD)(A;;GR;;;AC)S:(ML;;NW;;;LW)`
（所有者は全権限、Everyone と ALL APPLICATION PACKAGES は読み取りのみ、低整合性レベルのプロセスからも読み取り可能）

### 8.2 レイアウト

32bit プロセスと 64bit プロセスが同じ領域を読むため、固定幅の型のみを使いパディングを明示する。`seq` は 32bit プロセスでも 1 命令で読めるよう 32bit 整数にする。

```cpp
// windows/common/include/phonecam/shared_frame.h
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
    int64_t  timestampUs;     // iPhone のキャプチャ時刻
    int64_t  heartbeatTickMs; // 書き手の GetTickCount64()
};
#pragma pack(pop)
static_assert(sizeof(PhoneCamSharedHeader) == 64);

inline constexpr wchar_t kPhoneCamFrameMappingName[] = L"Local\\PhoneCam_Frame_v1";
```

NV12 はパディングなしで格納する（Y 面 `width × height`、続けて UV 面 `width × height / 2`、ストライド = width）。

### 8.3 seqlock 手順

**書き込み（Receiver）**
1. `InterlockedIncrement(&seq)`（奇数になる）
2. `width` / `height` / `pixelFormat` / `timestampUs` / `flags` とピクセルデータを書き込む
3. `InterlockedIncrement(&seq)`（偶数に戻る）

**読み取り（フィルタ）**
1. `s1 = seq`。奇数なら `YieldProcessor()` して再試行。
2. ヘッダの各フィールドとピクセルデータを自前のバッファにコピーする。
3. `s2 = seq`。`s1 != s2` なら破棄して再試行。
4. 3 回失敗したら、前回のフレームを使う。
5. `s1` が前回と同じなら新しいフレームは無いので、コピーを省略して前回のフレームを使う。

`heartbeatTickMs` の値が現在の `GetTickCount64()` より 2000 ms 以上古い場合、書き手は不在とみなす。

---

## 9. DirectShow フィルタ仕様（PhoneCamFilter.dll）

### 9.1 識別子

| 項目 | 値 |
|---|---|
| フィルタ CLSID | `{01325355-763C-4CFF-ADC9-1AF0EB21D0B0}`（M0 で生成し `filter/src/guids.h` に記録済み。以後変更禁止） |
| フレンドリ名 | `PhoneCam` |
| カテゴリ | `CLSID_VideoInputDeviceCategory` |
| 出力ピン名 | `Capture` |
| DLL 名 | `PhoneCamFilter.dll`（x86 / x64 同名、別フォルダに配置） |

### 9.2 ビルド要件

- x86 と x64 の両方をビルドする（必須）。32bit の会議アプリ等から利用されるため。
- CRT は静的リンク（`/MT`）とし、ホストアプリの VC++ ランタイムに依存しない。
- 依存は BaseClasses、libyuv、WIL と OS ライブラリ（`strmiids.lib`, `winmm.lib`, `ole32`, `oleaut32`）のみ。FFmpeg や ImGui を持ち込まない。
- エクスポートは `filter.def` で `DllGetClassObject`, `DllCanUnloadNow`, `DllRegisterServer`, `DllUnregisterServer` の 4 つ。

### 9.3 実装が必要なインターフェース

| クラス | 基底 / インターフェース | 要件 |
|---|---|---|
| `CPhoneCamFilter` | `CSource` | フィルタ本体 |
| | `IAMFilterMiscFlags` | `AM_FILTER_MISC_FLAGS_IS_SOURCE` を返す |
| `CPhoneCamPin` | `CSourceStream` | `FillBuffer`, `GetMediaType`, `CheckMediaType`, `SetMediaType`, `DecideBufferSize`, `OnThreadCreate`, `OnThreadDestroy` |
| | `IAMStreamConfig` | §9.5 |
| | `IKsPropertySet` | `AMPROPSETID_Pin` / `AMPROPERTY_PIN_CATEGORY` の `Get` に `PIN_CATEGORY_CAPTURE` を返す。`Set` は `E_NOTIMPL` |

`NonDelegatingQueryInterface` で上記インターフェースを公開すること。COM メソッドから C++ 例外を外に出さない（すべて捕捉して `E_FAIL` 等に変換）。`DllMain` では重い処理をしない。

### 9.4 メディアタイプ

`GetMediaType(i)` はこの順で返す（インデックス 0 が既定）。

| i | サブタイプ | 解像度 | biCompression | biBitCount |
|---|---|---|---|---|
| 0 | `MEDIASUBTYPE_YUY2` | 1280×720 | `MAKEFOURCC('Y','U','Y','2')` | 16 |
| 1 | `MEDIASUBTYPE_YUY2` | 1920×1080 | 同上 | 16 |
| 2 | `MEDIASUBTYPE_YUY2` | 640×480 | 同上 | 16 |
| 3 | `MEDIASUBTYPE_RGB24` | 1280×720 | `BI_RGB` | 24 |
| 4 | `MEDIASUBTYPE_RGB24` | 1920×1080 | `BI_RGB` | 24 |
| 5 | `MEDIASUBTYPE_RGB24` | 640×480 | `BI_RGB` | 24 |

共通: `FORMAT_VideoInfo`（`VIDEOINFOHEADER`）、`AvgTimePerFrame = 333333`（30 fps）、`biHeight` は正（RGB24 はボトムアップ）、`biSizeImage` は行を 4 バイト境界に揃えて計算、`bFixedSizeSamples = TRUE`。

### 9.5 IAMStreamConfig

- `GetNumberOfCapabilities`: 個数 6、サイズ `sizeof(VIDEO_STREAM_CONFIG_CAPS)`。
- `GetStreamCaps(i)`: §9.4 の i 番目のメディアタイプと、最小 = 最大の解像度、フレーム間隔 333333 固定の `VIDEO_STREAM_CONFIG_CAPS` を返す。
- `SetFormat`: §9.4 の一覧に含まれる形式のみ受け付ける。未接続なら次の接続で使う形式として保持し、接続中なら `IFilterGraph::Reconnect` で再接続する。
- `GetFormat`: `SetFormat` 済みならその形式、未設定ならインデックス 0 を返す。

### 9.6 FillBuffer

```
OnThreadCreate:
  高分解能ウェイタブルタイマを作成（CreateWaitableTimerExW + CREATE_WAITABLE_TIMER_HIGH_RESOLUTION。
  失敗時は通常タイマにフォールバック）
  frameIndex = 0

FillBuffer(sample):
  次のフレーム時刻まで待機（負荷で遅れた場合は待たずに進め、遅れを追いかけない）
  共有メモリが未オープンなら 1 秒に 1 回まで OpenFileMapping を試みる
  §8.3 の手順で最新フレームを取得
  if 共有メモリなし or heartbeat が古い or SOURCE_ACTIVE でない:
      プレースホルダ画像を出力形式で書き込む
  else:
      中央クロップ → 拡縮 → 出力形式へ変換して書き込む
  rtStart = frameIndex * AvgTimePerFrame; rtStop = rtStart + AvgTimePerFrame
  sample.SetTime(rtStart, rtStop); sample.SetSyncPoint(TRUE); sample.SetActualDataLength(...)
  frameIndex++
  return S_OK
```

- 新しいフレームが無い場合は直前のフレームを再送する。映像が止まってもブロックしない（必須）。
- 変換の推奨経路（libyuv）: NV12 → I420（`NV12ToI420`）→ `I420Scale`（`kFilterBilinear`）→ `I420ToYUY2` または `I420ToRGB24Matrix`（BT.709）。RGB24 は高さに負値を渡して上下反転する。
- 拡縮の方針: アスペクト比を保って出力サイズを覆うよう中央クロップしてから拡縮する（640×480 では左右が切れる）。
- 変換用バッファは `SetMediaType` 時に確保し、`FillBuffer` 内で毎回確保しない。

### 9.7 プレースホルダ画像

- 共有メモリが無い、heartbeat が 2 秒以上古い、または `SOURCE_ACTIVE` が立っていない時に表示する。
- 暗い灰色の背景に、中央へ「PhoneCam」と「iPhone のアプリと PC の PhoneCam を起動してください」を表示する。
- メディアタイプ確定時に GDI（DIB セクション、フォント `Yu Gothic UI`）で 1 回だけ描画し、出力形式に変換してキャッシュする。

### 9.8 登録

- `DllRegisterServer`: `AMovieDllRegisterServer2(TRUE)` で COM 登録し、`IFilterMapper2::RegisterFilter` で `CLSID_VideoInputDeviceCategory` にフレンドリ名 `PhoneCam`、メリット `MERIT_DO_NOT_USE` で登録する。
- `DllUnregisterServer`: 上記の逆。
- `scripts/register.ps1`: 管理者権限で `System32\regsvr32.exe`（x64 DLL）と `SysWOW64\regsvr32.exe`（x86 DLL）を実行する。**開発中に自動実行しないこと**（開発者が手動で実行する）。

---

## 10. 非機能要件

| 項目 | 目標 |
|---|---|
| 遅延（撮影からアプリ表示まで） | 720p30・5 GHz Wi-Fi で 150 ms 以下を目標、200 ms 以下を必須 |
| Receiver CPU 使用率 | 720p30 で 4 コア相当の CPU の 10% 以下 |
| フィルタ CPU 使用率 | 720p30 出力で 1 アプリあたり 3% 以下 |
| 連続稼働 | 2 時間でクラッシュなし、メモリ・ハンドル数が増え続けない |
| 再接続 | iPhone アプリ再起動から 5 秒以内に映像が戻る |
| 端末発見 | Receiver 起動から 5 秒以内に一覧に表示 |
| 異常時 | iPhone 側の切断・停止でホストアプリ（Zoom 等）がフリーズ・クラッシュしない |

---

## 11. マイルストーンと受け入れ基準

各マイルストーンの受け入れ基準をすべて満たしてから次へ進む。完了した項目は本書のチェックボックスを更新する。

### M0: 足場づくり
- [x] §3 のディレクトリ構成を作成
- [ ] `x64-debug` / `x64-release` / `x86-release` の各プリセットでビルドが通る（中身は空でよい）
- [ ] vcpkg の依存が解決できる（x86 では FFmpeg / ImGui を取得しない）
- [ ] BaseClasses をベンダリングし、x86 / x64 で静的ライブラリとしてビルドできる
- [ ] `protocol.h` と `shared_frame.h` を §5 / §8 どおりに作成し、`static_assert` が通る
- [x] フィルタ CLSID を生成し、§9.1 と `guids.h` に記録
- [ ] （Mac）Xcode プロジェクトを作成し、§6.8 の Info.plist キーを設定

### M1: 仮想カメラ単体
- [ ] `tools/testpattern` が共有メモリに 1280×720 の動くテストパターン（カラーバー + フレーム番号）を書き込む
- [ ] フィルタ x86 / x64 がビルドでき、`register.ps1` で登録できる
- [ ] 「PhoneCam」として ffplay（`ffplay -f dshow -i video="PhoneCam"`）、OBS、Zoom または Discord、Chrome で映る
- [ ] OBS で §9.4 の 6 形式すべてを選択でき、正しく表示される（上下反転・色ずれなし）
- [ ] testpattern を止めると 2 秒以内にプレースホルダになり、再開するとアプリを開き直さずに映像が戻る
- [ ] 2 つのアプリで同時に使える
- [ ] 32bit のホスト（x86 版 ffmpeg 等）で映る
- [ ] 30 分間の連続表示でメモリ・ハンドル数が増え続けない

### M2: iOS アプリの送出
- [ ] `tools/probe.py` が Bonjour で端末を発見し、ハンドシェイク後に H.264 を標準出力に書き出して `ffplay -f h264 -fflags nobuffer -flags low_delay -` で再生できる
- [ ] SET_SETTINGS で 1080p / 720p、カメラ、ビットレートを切り替えられる
- [ ] REQUEST_KEYFRAME の直後にキーフレームが届く
- [ ] ローカルネットワーク許可を拒否した場合に誘導画面が出る
- [ ] バックグラウンド移行で切断し、復帰で広告を再開する
- [ ] 30 分間の連続送出でクラッシュしない

### M3: 受信アプリで一気通貫
- [ ] Receiver 起動から 5 秒以内に端末が一覧に表示される
- [ ] 接続するとプレビューに映り、Zoom 等で iPhone の映像が使える
- [ ] §10 の遅延要件（200 ms 以下）を満たす
- [ ] iPhone アプリを終了すると 2 秒以内にプレースホルダになり、再起動すると 5 秒以内に自動再接続する
- [ ] 手動 IP 接続ができる
- [ ] パブリックネットワークで警告が出る

### M4: 操作と堅牢化
- [ ] PC 側 UI からカメラ切替・ズーム・トーチ・解像度・ビットレートを操作できる
- [ ] 熱ポリシー（§6.6）が働き、PC 側に制限中と表示される
- [ ] 統計表示（fps、ビットレート、RTT、破棄数）
- [ ] iPhone・PC の設定が再起動後も保持される
- [ ] 電波の悪い環境でも映像が永久に止まらず、回復する
- [ ] 2 時間の連続稼働

### M5: 配布
- [ ] Inno Setup インストーラ（Receiver、x86 / x64 フィルタの配置と登録、ショートカット、アンインストール時の登録解除）
- [ ] クリーンな Windows 10 22H2 と Windows 11 の VM で動作確認
- [ ] `LICENSE` と `THIRD_PARTY_NOTICES.md`
- [ ] 利用者向け `README.md`

---

## 12. テスト

### 12.1 ツール

| ツール | 用途 |
|---|---|
| `tools/testpattern` | フィルタを iPhone なしで検証する |
| `tools/probe.py` | iOS アプリを Receiver なしで検証する。依存は `zeroconf`。Bonjour 探索、手動 IP、ハンドシェイク、SET_SETTINGS 送信、H.264 の標準出力またはファイルへの書き出し |
| ffplay / ffmpeg | `-f dshow` によるフィルタ確認、`-f h264` によるストリーム確認 |
| GraphStudioNext | フィルタのピン・メディアタイプの確認 |

### 12.2 動作確認マトリクス

| 区分 | 対象 |
|---|---|
| Windows | 10 22H2 x64、11 23H2 以降 x64 |
| ホストアプリ | OBS Studio、Zoom、Discord、Microsoft Teams、Chrome、Edge、ffplay（x64 / x86） |
| iOS | iOS 16 / 17 / 18 の実機（背面・前面カメラ） |
| ネットワーク | 5 GHz、2.4 GHz、プライベート / パブリックプロファイル |

### 12.3 遅延の測り方

PC 画面にミリ秒表示のストップウォッチを表示し、iPhone でその画面を撮影する。会議アプリのプレビューとストップウォッチが同時に写るように画面キャプチャし、両者の表示の差を読む。10 回測って中央値をとる。

### 12.4 自動テスト（推奨）

- C++: パケットのエンコード / デコード、seqlock の読み書き、メディアタイプ生成を単体テストする（GoogleTest を vcpkg で導入）。
- Swift: `Packet` / `Messages` のエンコード、AVCC → Annex B 変換を XCTest で単体テストする。
- プロトコルの互換性確認として、同じバイト列のテストベクタ（`tests/vectors/*.bin`）を C++ と Swift の両方で読む。

---

## 13. ライセンス・配布上の注意

- FFmpeg は LGPL の範囲で使い、DLL として動的リンクする。GPL 機能（x264 等）は有効にしない。
- BaseClasses（MIT）、libyuv（BSD）、Dear ImGui（MIT）、WIL（MIT）の表記を `THIRD_PARTY_NOTICES.md` に含める。
- OBS Studio のコードは GPL のため、参考にするのは設計のみとし、コードを流用しない。
- H.264 のデコード機能を含むソフトウェアを配布する場合の特許ライセンスの扱いは、公開前に確認する。

---

## 14. 未決定事項（TBD）

| 項目 | 状況 |
|---|---|
| 製品名 | 仮称 `PhoneCam`。変更時の置換対象: サービス種別、共有メモリ名、フレンドリ名、フォルダ名、バンドル ID |
| フィルタ CLSID | 決定済み（M0）: `{01325355-763C-4CFF-ADC9-1AF0EB21D0B0}`（§9.1） |
| iOS バンドル ID | 未定 |
| 既定ポート 48620 | 仮決定。他製品との衝突を確認して確定 |
| 共有メモリの SDDL | §8.1 の案を M1 で検証。ブラウザ等のサンドボックス環境で読めない場合は再検討 |
| アイコン・画面デザイン | 未定 |
| 前面カメラのミラー設定 | v1 は反転なし。要望があれば設定項目化 |

---

## 付録: 参考資料

- DirectShow BaseClasses: microsoft/Windows-classic-samples リポジトリの `Samples/Win7Samples/multimedia/directshow/baseclasses`
- Softcam（MIT、DirectShow 仮想カメラの実装例）: tshino/softcam
- Microsoft Learn: DnsServiceBrowse / DnsServiceResolve、IAMStreamConfig、IKsPropertySet、Writing Source Filters
- Apple Developer: AVCaptureSession、VTCompressionSession、NWListener、Local Network Privacy
