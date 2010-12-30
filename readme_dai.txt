樋口M 配布の DxOpenNI ( http://www.geocities.jp/higuchuu4/ ) の勝手拡張です。
派生元バージョンは 1.0 で、1.10 まで追随しています。

DxOpenNI 1.0 は、OpenNI で取得した関節位置を提供する DLL です。
MMD からこの DLL の公開関数を利用して、関節位置を取得しているはずです。

この派生では、DxOpenNI 1.0 を元に次の変更を入れています:
 * プログラムのクラス構造化
 * 一定数のセンサーデータを貯めておき、提供時に平均値を渡す「平滑化」機能
 * GUI

-- 
Dai.K.
   email: dai1975@gmail.com
   twitter: @dai197x

