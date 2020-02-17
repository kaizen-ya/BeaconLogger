# BeaconLogger

M5StackにiBeacon受信プログラムを書き込み、iBeaconモードに設定したGimbalビーコンを使って動線分析をする。

# Gimbalビーコン購入方法

Beacon購入方法.pdf を参照

# Gimbalビーコン設定方法

まず、受信機と送信機で共通となるUUIDをコマンドプロンプトにて取得する（Windowsの場合）。
> powershell -Command "[Guid]::NewGuid()"

下記の手順でGimbalビーコンをiBeaconモードに設定する
尚、手順中の
[web]は https://manager.gimbal.com/login/users/sign_in
[app]は スマートフォンアプリのGimbal Beacon Manager
を意味する。

１．[web] Beacons > Beacon Configurations 追加
　　識別用のMajor,Minorを設定する。
２．[app] Activate Beacon
　　GimbalビーコンのQRコードをアプリで読み込む
３．[web] Beacons > Beacon Management
　　Assigined Configuration変更
４．[app] Configure
　　iBeaconとして認識される

# M5Stack設定方法

１．設定ファイルconf.txt編集
　　devid ・・・ M5Stackが複数ある場合用に管理番号を設定
  　uuid ・・・ Gimbalビーコン設定時に取得したUUIDを設定
   編集したconf.txtをmicroSDカードに保存し、M5Stackに格納する。
   
２．USBドライバーのインストール
　　https://m5stack.com/pages/download　からCP210X Driverをダウンロードし、インストール。

３．M5StackをPCに接続し、ポート番号を確認。
　　M5StackをPCに接続。
　　「スタートボタン」の上で右クリック→「デバイスマネージャー」を開いて、ポート番号を確認。

４．load.batを編集
　　３．で確認したポート番号に合わせてPORTを編集。

５．プログラム書き込み
　　load.batを実行。
 
 ３０秒に１度、最も近接したビーコンのMajor-MinorがCSVデータ（devid-追番.csv）として保存される。
 
