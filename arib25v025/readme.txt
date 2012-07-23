b25.exe ver.0.2.5 のLinux対応版です。
arib25ディレクトリでmakeと打てばarib25/src/b25にビルドされます。
使用ライブラリはPCSC-Lite(http://pcsclite.alioth.debian.org/)で、カードリーダが使用可能な状態になっていることが必要です。
Windowsでは鉄板のHX-520UJJはLinuxでは使用できないので注意が必要です。
NTTComのSCR3310-NTTCom及びathena-scs.hs.shopserve.jp/SHOP/RW001.htmlで動作報告があります。
B-CASカードの表裏に注意して下さい。チップがある面が上です。
初期版の白凡内蔵のカードリーダを使用する場合は、PCSC-LiteのページにあるCCID driverを使用し、
/etc/libccid_Info.plist内の
====
	<key>ifdVendorID</key>
	<array>
		<string>0xXXXX</string>
		         略
	</array>

	<key>ifdProductID</key>
	<array>
		<string>0xXXXX</string>
		         略
	</array>

	<key>ifdFriendlyName</key>
	<array>
		<string>XXXXXXXXXXXXXXXXXX</string>
		               略
	</array>
====
をこのzip内にある同名のファイルの同じ部分と入れ替えて下さい。
※このファイル位置はDebian依存かもしれません。
  Gentooでは/usr/lib/readers/usb/ifd-ccid.bundle/Contents/Info.plistにあったという報告がありました。
黒凡及び最近の白凡ではカードリーダが変わっている為、/etc/libccid_Info.plistの変更は不要です。

smartcard_list.txtはPCSC-Liteに含まれるpcsc_scan用です。
PCSC-Liteのファイルと置き換えるとB-CASがB-CASとして認識されます。
新しめのOSでは既にB-CASの設定が入っているため置き変えなくてもB-CASとして表示されるようになっています。

PCSC-LiteのAPIがWindowsスマートカードアクセス用APIと互換であった為、
ほぼLinuxでコンパイルエラーになる部分の対処のみです。
ver.0.2.0より32bit環境で2Gbyteファイルの問題がなくなったはずです。64bit環境では2Gbyte以上のファイルを処理出来ることを確認しています。

PCSC-Liteに含まれるDWORD等の定義は非32bit環境の場合に問題がありますが、関係なく動きました。

変更点:
upstreamに合わせてupdateしました。
ver.0.1.2に対するパッチでopenの引数を削ってしまっていた箇所を元に戻しました。
ver.0.1.5で出力ファイルのパーミッションが適当すぎたのを直しました。umaskに従うようになっているはずです。
ver.0.2.0で、32bitLinux上で2GByte以上のファイルが処理できないバグを修正したはずです。

extrecdを使用している方への注意点:
b25の呼び出し時に-p 0 -v 0オプションを付ける必要があります(b25から何か出力があればエラーとみなしている為)。以下の修正を行なって下さい。
430行目
my @b25_cmd    = (@b25_prefix, $path_b25, $target_encts, $target_ts);
↓
my @b25_cmd    = (@b25_prefix, $path_b25, "-p", "0", "-v", "0", $target_encts, $target_ts);

ライセンス:
このソースはまるも氏作成のb25ほぼそのままなので、まるも氏の判断に従います。
よってarib25/readme.txtにあるオリジナルb25に添付されているreadme.txtに書いてある、
>　・ソースコードを利用したことによって、特許上のトラブルが発生しても
>　　茂木 和洋は責任を負わない
>　・ソースコードを利用したことによって、プログラムに問題が発生しても
>　　茂木 和洋は責任を負わない
>
>　上記 2 条件に同意して作成された二次的著作物に対して、茂木 和洋は
>　原著作者に与えられる諸権利を行使しない
が適用されます。

その他:
このプログラムはAS-ISで提供されます。なにか問題が起こっても責任は持てません。

動作確認環境:
  Debian GNU/Linux squeeze(stable)
  Linux 2.6.37.2 SMP PREEMPT x86_64

◆N/E9PqspSk / Clworld.
