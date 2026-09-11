# PhoneCam iOS アプリ

macOS + Xcode 15 以降でビルドする（SPEC §4.2）。Windows 上ではビルドできない。

## Xcode プロジェクトの作成（M0、Mac で実施）

1. Xcode で iOS App を新規作成する（Product Name `PhoneCam`、Interface SwiftUI、Language Swift）。
   保存先はこの `ios/` とし、`ios/PhoneCam/PhoneCam.xcodeproj` になるようにする。
2. テンプレートが作る `PhoneCamApp.swift` を `App/`、`ContentView.swift` を `UI/` に移し、
   SPEC §3 のフォルダ（`App` `UI` `Core` `Capture` `Encode` `Network` `Protocol`）をグループとして登録する。
3. Build Settings:
   - Swift Language Version: Swift 5
   - iOS Deployment Target: 16.0
   - Info.plist File (`INFOPLIST_FILE`): `PhoneCam/Info.plist`（このフォルダの `Info.plist`。§6.8 のキーを記載済み）
   - Generate Info.plist File: Yes のままでよい（生成分と `Info.plist` がマージされる）
4. ビルドして、生成された Info.plist に次の 3 キーが入っていることを確認する。
   `NSCameraUsageDescription` / `NSLocalNetworkUsageDescription` / `NSBonjourServices = ["_phonecam._tcp"]`

バンドル ID は未定（SPEC §14）。仮に `dev.local.PhoneCam` などで作成してよい。
