﻿BonDriverを使用したEPGデータの取得、予約録画を行うためのツール群です。


◆このプログラムを使用して発生した全ての問題に対して責任は持ちません。


■注意■
　すべてを新規作成したので人柱１０以前の過去バージョンとは互換性がなく
　なっています。
　設定ファイルも互換性がないため、人柱１０以前からのバージョンアップの
　場合は一度すべてを削除してください。
　まだ動作の不安定な部分やおかしい部分などが残っている可能性があります。
　スクランブルを解除する機能はありません。

■動作環境■
OS    ：Windows XP SP3以降
CPU   ：Intel Core 2 Duo以上推奨
メモリ：1GB以上推奨

動作には.NetFramework4.0とVC++2010のランタイムが必要です。
他に各BonDriverや外部モジュールで使用するランタイムが必要な場合があります。

Microsoft .NET Framework 4 (スタンドアロンのインストーラー)
http://www.microsoft.com/downloads/details.aspx?familyid=0A391ABD-25C1-4FC0-919F-B21F31AB88B7&displaylang=ja
Microsoft Visual C++ 2010 再頒布可能パッケージ (x86)
http://www.microsoft.com/downloads/details.aspx?FamilyID=a7b7a05e-6de6-4d3a-a423-37bf0912db84&displaylang=ja
Microsoft Visual C++ 2010 再頒布可能パッケージ (x64)
http://www.microsoft.com/downloads/details.aspx?familyid=BD512D9E-43C8-4655-81BF-9350143D5867&displaylang=ja


■64bitOSへの対応■
　x64フォルダにあるモジュールは64bitでビルドしたモジュールになっていま
　す。64bitネイティブで動作することが可能です。
　64bit版を使用するにはBonDriverも64bitでビルドされている必要があります。
　32bitのモジュールが１つでも必要な場合は使用できません。


■ソース（src.zip）の取り扱いについて■
　特にGPLとかにはしないのでフリーソフトに限っては自由に改変してもらった
　り組み込んでもらって構わないです。
　改変したり組み込んだりして公開する場合は該当部分のソースぐらいは一緒
　に公開してください。（強制ではないので別に公開しなくてもいいです）
　商用、シェアウェアなどに許可なく組み込むのは不可です。


■EpgDataCap3.dll、CtrlCmdCLI.dll、SendTSTCP.dll、BonDriver_TCP.dllの
取り扱いについて
　フリーソフトに組み込む場合は特に制限は設けません。
　このdllを使用したことによって発生した問題について保証は一切行いません。
　商用、シェアウェアなどに許可なく組み込むのは不可です。

■twitter.dllの取り扱いについて
　他のDLLとは異なり、そのまま他のソフトで使用するのは不可です。
　APIキーを所得し、ビルドし直したものを使用するのは構いません。
　商用、シェアウェアなどに許可なく組み込むのは不可です。

■基本的な使用準備■
　１．64bitOSで64bitネイティブで動作させるには「x64」フォルダを、
　　　それ以外の場合は「x86」フォルダを使用してください。

　２．使用デバイス用のBonDriverを用意し、BonDriver フォルダに入れる。
　　　BonDriverによってはiniファイルなどで設定できる内容があるので、あ
　　　らかじめ設定をしておく

　３．EpgDataCap_Bon.exeを起動し、「設定」→「基本設定」タブで「設定関
　　　係保存フォルダ」を設定する。

　４．チューナーから使用チューナーを選んで「チャンネルスキャン」を行う。
　　　地デジで5分程はかかると思います。
　　　使用するチューナーの種類が複数の場合は同じ事を行う。
　　　同時に複数起動して、チャンネルスキャンを行うことができます。

　５．全てのチューナーのチャンネルスキャンが完了するまで待ちます。

　６．EpgDataCap_Bon.exeを終了する。
　　　（複数起動している場合は、チャンネルスキャンが終了したものから終
　　　わらせてください）

　８．EpgTimer.exeを起動する。

　９．「設定」→「基本設定」→「チューナー」タブで各BonDriverで使用する
　　　チューナー数とEPGデータの取得に使用するかを設定します。
　　　◆チューナー数が正しく設定されないと、正常に動作しません◆

１０．「EPG取得」タブでEPG取得対象サービスを設定します。

１１．EpgTimer.exeを終了する。
　　　◆チューナー数の設定は起動時にのみ反映するので、終了することが重要です◆

１２．EpgTimer.exeを起動する。

１３．基本的な使用準備は終わり。
　　　EPG取得を行い、必要に応じて各種設定や、予約登録など行う。

EpgDataCap_Bon.exeの詳細はReadme_EpgDataCap_Bon.txt
EpgTimer.exeの詳細はReadme_EpgTimer.txt
を参照してください。

◆チューナーによって地デジの受信チャンネルが異なる特殊な受信環境の場合、
◆同一サービスのチャンネルが複数、チャンネルスキャンで引っかかっている
◆可能性があります。
◆中継局の増加により、受信レベルの低いチャンネルが引っかかっている可能
◆性もあります。
◆正常に受信できる１チャンネル分のサービスのみ残して、他のチャンネルの
◆サービスは削除してください。
◆（EpgDataCap_Bon.exeで表示されるspace、chの値を参考に、設定->サービス
◆表示設定より削除）
◆同一サービスが複数あると予約録画などの動作が正常に動作しない可能性が
◆あります。


■バグ報告について
　http://2sen.dip.jp/dtv/の掲示板をバグ報告用として利用させて頂いてます
　が、アクセス規制の煽りを食らって書き込みやアップロードできない状態に
　なることがあります。
　バグ報告に関するレスは修正という形で取らせてもらうのが大半になると思
　います。
　バグ報告以外の内容に関しては基本的に対応は行いません。
　要望に関しては簡単にできそうな物なら組み込んでいこうと考えています。
　◆バグ報告には、使用環境（ハード、アプリ、BonDriver、バージョンなど）、
　◆何から予約登録を行い、どのような設定でだったかなどの詳細も記載願い
　◆ます。
　◆多種多様な環境を構築できるようになってきたため、詳細の記載なき報告
　◆は基本的に確認を行いません（行えません）。
　◆DbgView、DbgMonなどを使用してログの取得を行ってもらえると、解決の糸
　◆口が見つかりやすくなります。
　◆再現性のある予約録画に関する内容はログの取得を行ってください。ログ
　◆のないものは基本的に調査を行えません。


■更新履歴■
12/05/25　人柱版１０．６９
・スクランブル解除モジュールを使用する機能を削除（録画済み一覧のScramble用背景色の変更を推奨）
　一部設定項目が残りますが、機能はしません
・EpgTimerSrv.exeの2重起動時にReserve.txtの追加読み込みが発生していなかったので修正
・ドロップ判定処理を修正
12/05/20　人柱版１０．６８
・DMSぽい機能のポート指定が正常に動作しなかたので修正
・DMSぽい機能の通信処理に問題あったので修正
・後ろの予約を同一ファイルで出力するに問題あったので修正
・録画後動作抑制条件が復帰処理開始時間+5分より短い場合、強制的に復帰処理開始時間+5分になるように変更
・ドロップ判定処理を修正
12/04/22　人柱版１０．６７
・ブラウザ機能で予約の検索処理に問題あったので修正
・DMSぽい機能追加（設定方法の詳細はReadme_EpgTimer.txt参照）
12/04/01　人柱版１０．６６
・xmlファイルの読み込みでエラーとなった場合、バックアップファイルを読み込めるように変更
・ブラウザ機能での予約時にプリセット設定がうまくいかない場合があったのを修正
・ブラウザ機能で予約の検索処理に問題あったので修正
・しょぼいカレンダーへのアップロード内容がおかしい場合があったので修正
・Convert.txtに抜けあったので修正
・BonDriverオープン後にWaitを入れる設定を追加（設定方法の詳細はReadme_EpgDataCap_Bon.txt参照）
11/11/23　人柱版１０．６５
・$TitleF$と$Title2F$のマクロが文字化けすることあったので修正
11/11/07　人柱版１０．６４
・しょぼいカレンダーへの予約のアップロード機能を追加（設定方法の詳細はReadme_EpgTimer.txt参照）
・ブラウザ表示機能に自動予約登録EPG予約を追加
11/10/30　人柱版１０．６３
・録画開始マージンにマイナス値が入っていた場合、録画開始のタイミングが問題になる可能性あったのを修正
・文字変換に一部問題あったので修正
・デザインをロードしない設定でロードしてる場所あったので修正
11/10/10　人柱版１０．６２
・モータースポーツのジャンルが抜けていたので修正
・録画フォルダを開くでファイル名に「,」があると問題があったので修正
・デザインをロードしない設定を追加（設定方法の詳細はReadme_EpgTimer.txt参照）
・予約一覧の背景色を変更できる設定を追加（設定方法の詳細はReadme_EpgTimer.txt参照）
・録画済み一覧の背景色を変更できる設定を追加（設定方法の詳細はReadme_EpgTimer.txt参照）
11/09/24　人柱版１０．６１．１
・EITの解析処理を少し変更（これで表示はできるようになった？蓄積判定は未実装）
・チャンネルスキャンのタイムアウト値を変更できる設定を追加（設定方法の詳細はReadme_EpgDataCap_Bon.txt参照）
11/09/23　人柱版１０．６１
今回は不安定な部分あるかも？
・SDT、NIT、EITの解析処理を少し変更（SDの解析のできた部分の対応）
・EPG取得のタイムアウト値を変更できる設定を追加（設定方法の詳細はReadme_EpgDataCap_Bon.txt参照）
・EPG取得でタイムアウト発生時にファイルを保存する設定を追加（設定方法の詳細はReadme_EpgDataCap_Bon.txt参照）
11/09/19　人柱版１０．６０
・PMTが2パケットで構成されている物から1パケットでの構成に変更になった場合に正常に処理できない可能性があったので修正
・HTML番組表で予約が終了した番組詳細の処理に問題あったので修正
・番組情報のアクセスできない状態（読み込み中や存在しない）時に、番組情報の必要なマクロ（$SubTitle$など）を使用した場合にマクロ部分を削除するように変更
・2つ以上の同時出力が設定されている予約の予定ファイル名の表示に問題あったので修正
11/09/12　人柱版１０．５９．１
・RecName_Macro.dllのファイルを間違えていたので置き換え
11/09/11　人柱版１０．５９
・番組表のマウススクロールに自動の設定を追加（TOUCH MOUSE使用時などに指定）
・TOUCH MOUSEで最小化→元に戻すとやると表示に問題あったので修正
・RecName_Macro.dllのマクロにサブタイトルを追加
・そろそろ開発環境をVS2010 SP1にするかも？
11/09/07　人柱版１０．５８
・メモリリークあったので修正
・マルチモニター環境でフルスクリーン時のEpgTimerPlugInの動作を修正？（環境ないので不明）
・予定ファイル名はソートできないように修正
11/09/05　人柱版１０．５７
・予約一覧の追加項目のソート処理で問題あったので修正
・イベントリレーで追加する予約のチューナー指定を自動で行うように変更
・非表示状態でシャットダウン要請が通知された場合に表示するように変更
・予約録画時に強制的に起動時のサービス指定を行う設定を追加（設定方法の詳細はReadme_EpgDataCap_Bon.txt参照）
・RecName_Macro.dllのマクロにサブタイトルを追加
11/08/29　人柱版１０．５６
・EpgDataCap_Bon.exeで録画中にシャットダウン要請が通知されると拒否するように変更
・予約一覧、録画済み一覧、自動予約登録で表示項目の選択を行えるように変更
・予約一覧の表示項目に予定ファイル名とぴったり（？）を追加
・自動予約登録のEPG予約の表示項目に登録対象数を追加（対象項目の確認は予約条件のダイアログを開いて検索してください）
11/08/21　人柱版１０．５５
・情報通知ログを自動的にファイルに保存する設定を追加（設定方法の詳細はReadme_EpgTimer.txt参照）
・自動予約登録のEPG予約で変更処理でも条件追加のボタンを表示するように変更
・最小化時にタスクトレイに格納する設定を追加
・TSデータとファイル出力データのバッファリング最大値を変更できる設定を追加（設定方法の詳細はReadme_EpgDataCap_Bon.txt参照）
・変更できる予約割り振りのアルゴリズムを追加
・録画モードが視聴の予約の結果判定に問題あったので修正
11/08/15　人柱版１０．５４
・録画後動作の抑制条件に共有フォルダのTSファイルにアクセスがある場合を追加
（機能が有効になるにはEpgTimerSrv.exeが管理者権限で実行されている必要があります）
・予約一覧と録画済み一覧のカラム位置と横幅を覚えるように変更（今までの横幅の記憶は無効になります）
・番組表の予約枠表示に問題あったので修正
・録画済み一覧の右クリックメニューに録画フォルダを開くを追加
11/08/07　人柱版１０．５３
・ブラウザ表示機能にプログラム予約を追加
・連続録画で同一ファイル出力時に、番組情報を追記で保存するように変更
・自動予約登録の対象期間に時間単位の設定を追加（設定方法の詳細はReadme_EpgTimer.txt参照）
・録画設定の録画マージン設定で「:」による時:分:秒入力を行えるように変更
　（入力確定後は秒換算された値に変わります）
・一度もEpgTimerのウインドウが表示されない状態でタスクトレイの右クリック
　メニューから設定を押した場合にウインドウを表示するように変更
11/08/06　人柱版１０．５２
・イベントリレーでリレー先のサービスがチャンネルスキャンされていなかった場合に問題あったので修正
・録画時にファイル作成に失敗した場合に問題あったので修正
11/08/01　人柱版１０．５１．１
・RecName_Macro.dllの曜日マクロに問題あったので修正
・EpgTimerを最小化で起動にしている場合に問題あったので修正
・録画後bat起動時の形式を変える設定を追加（設定方法の詳細はReadme_EpgTimer.txt参照）
・休止、スタンバイボタンを押した場合に確認ダイアログを出す設定を追加（設定方法の詳細はReadme_EpgTimer.txt参照）
11/07/31　人柱版１０．５１
・RecName_Macro.dllのマクロに曜日を追加
・録画後バッチのマクロに曜日とフォルダパスを追加
・DPIが100%以外の場合に番組表の表示を少し変更（中途半端な倍率設定では、ぼやけた表示なる可能性があります）
・バッチファイル実行時に最小化で実行するように変更
・録画時間がマイナスになる場合にエラーが出ていたのを修正
・BonDriverから1パケットに満たないデータが返されると問題があったので修正（指摘ありがとうございます）
・情報通知ログのダイアログにファイルに保存するを追加
・プロテクトファイルの処理を少し変更
・古い録画ファイルの自動削除処理を少し変更
・録画開始時にディスクの空き確保を行わない設定を追加（設定方法の詳細はReadme_EpgTimer.txt参照）
11/07/24　人柱版１０．５０
・EpgTimerNWから追っかけ再生ができなくなっていたので修正
・サーバー連携機能の設定表示に問題あったので修正
・録画済み一覧にプロテクト機能を追加
（EpgTimerSrv.exe制御からの削除を行わないようにするだけで、エクスプロー
ラーなどから録画ファイルの削除はできます。録画一覧の保持件数を設定して
いる場合、プロテクト数が多すぎると新規の内容がすぐに削除される可能性ある
ので数に注意）
11/07/17　人柱版１０．４９
・番組表でシングルクリックで予約ダイアログを開く設定を追加
・番組表で番組詳細タブが選択された状態で開く設定を追加
・10分以上のアイドル状態が続いている場合は、バルーンチップによる通知を一度だけ行うように変更
・チューナー確保に失敗した予約の扱いを変更
（録画時間が終わるまでは録画済みにはしないことで、EPG再読み込み時の再登録を抑制）
11/07/10　人柱版１０．４８
・Write_Default.dllの設定値の読み込みに問題あったので修正
・エラーログにPIDの種類を追加
・変更できる予約割り振りのアルゴリズムを追加
・録画済み一覧でダブルクリックでファイル再生を行う設定を追加
11/07/02　人柱版１０．４７
・EpgTimerTaskを追加
　EpgTimerSrvからのexe実行を中継するためだけの物。
　サービス化してEpgTimerは普段起動しないが録画時にGUIを表示したい人用。
・Write_Default.dllで書き込みバッファのサイズを設定できるように変更
・同一チャンネルで複数サービスの録画が発生した場合、録画終了時にメインとなるサービスを切り替えるように変更
・予約録画時のチャンネル切換リトライ処理を少し変更
・EPG取得で1チャンネル取得完了ごとに取得完了通知が発生していたのを修正
・録画終了のツイートをエラー時とドロップ数によって行う設定を追加（設定方法の詳細はReadme_EpgTimer.txt参照）
11/06/22　人柱版１０．４６
・ブラウザ表示の番組表のHTML出力形式を変更
・ブラウザ表示で時間軸を表示する間隔を設定できる項目を追加（設定方法の詳細はReadme_EpgTimer.txt参照）
・ブラウザ表示で表示モードとジャンル絞り込みの対応追加（検索絞り込みとリスト表示は未対応の方向で）
（一度設定を開いてOKで閉じてください。必要な設定情報が保存されます。）
11/06/21　人柱版１０．４５
・イベントリレー先のサービスがチャンネルスキャン結果に存在しない場合、番組詳細の表示内容に問題あったので修正
・イベントリレー時の動作ログの出力内容を追加
11/06/20　人柱版１０．４４
・ストリーミング配信時のUDPの設定を少し変更
・番組表で番組のない時間帯を表示しないようにしている場合、予約枠の表示に問題があったので修正
・メモリリークあったので修正
11/06/13　人柱版１０．４３
・1チューナーでの複数サービス同時録画時にUDP、TCPで最初のサービスしか送信できていなかったのを修正
（複数サービスで送信時はTVTest側でサービスを切り替えてください）
・デコード処理が常に全サービスで行われていたので修正
・番組表の表示を少し高速化
・ブラウザ表示機能を追加（設定方法の詳細はReadme_EpgTimer.txt参照）
　カスタム表示は表示サービスの設定のみ有効（ジャンル絞り込みや表示モードは未実装）
　番組表のいいHTMLの書き方思いつかなかったので、あれば教えてください。
　フルブラウザ非搭載の携帯表示を想定してるので、スタイルシートやJavaScriptは使用しない方向で。
11/06/04　人柱版１０．４２
・番組表の文字色を変更できる設定を追加
・自動予約登録のEPG予約で同一番組名の録画結果があれば無効にする条件を追加
　（動作仕様の詳細はReadme_EpgTimer.txt参照）
　（１０．４２以降から録画を行った番組が対象になります。現状の録画結果は対象になりません。）
・試しに起動時にスプラッシュを出すようにしてみた
11/05/29　人柱版１０．４１
・録画情報の番組情報とエラーログの読み込みに問題あったので修正
11/05/29　人柱版１０．４０
・録画開始マージンにマイナス値を設定した場合、番組表の予約枠の表示に問題があったのを修正
・録画時の番組情報とエラーログを指定フォルダに保存するようにした場合、録画情報に番組情報と
　エラーログを読み込めていなかったのを修正
11/05/25　人柱版１０．３９
・ファイル名PlugInを個別に指定している場合、禁則文字のチェックが抜けていたのを修正
・録画済み一覧の背景色設定に問題あったので修正
・プログラム予約で追従とぴったり（？）録画をするに設定できるようになっていたので修正
・録画時の番組情報とエラーログを指定フォルダに保存できる設定を追加（設定方法の詳細はReadme_EpgTimer.txt参照）
・録画済み一覧から削除を行うときにTSファイルも一緒に削除できる設定を追加（設定方法の詳細はReadme_EpgTimer.txt参照）
・禁則文字チェックで「\」の変換を行わない設定を追加（設定方法の詳細はReadme_EpgTimer.txt参照）
11/05/24　人柱版１０．３８
・録画開始時にエラーが発生した場合、サブ録画フォルダでのリトライ処理を追加
・録画済み一覧のDropとScrambleがある場合の背景色を変更する設定を追加（設定方法の詳細はReadme_EpgTimer.txt参照）
・番組表で表示するツールチップの背景色と文字色を変更する設定を追加（設定方法の詳細はReadme_EpgTimer.txt参照）
11/05/20　人柱版１０．３７
・スクランブル解除モジュールで例外発生時に暗号状態で処理を続けるように変更
・短時間での連続したチャンネル切換発生時に受信データ遅延に対する処理を強化
11/05/15　人柱版１０．３６
・予約変更ダイアログで番組詳細が表示されない場合があったので修正
・番組詳細にイベントリレーの情報を追加
・TVTest連携でTVTest起動時とBonDriver切換時のwaitを変更できる設定を追加（設定方法の詳細はReadme_EpgTimer.txt参照）
・録画後実行batで使えるマクロを追加（詳細はReadme_EpgTimer.txt参照）
・RecName_Macro.dllで使えるマクロを追加（詳細はReadme_EpgTimer.txt参照）
・RecName_PlugInのインターフェースに番組情報も渡すAPIを追加
11/05/11　人柱版１０．３５
・EPGデータの解析に問題あったので修正
11/05/08　人柱版１０．３４
・TVTest連携でBonDriverにSpinelを使用していると、BonDriverが切り替わるようなチャンネル切換で失敗するため
　BonDriver切換後2秒のwaitを挿入
・OpenするBonDriverのファイル名にSpinelがあった場合、Open後1秒のwaitを試しに挿入
・イベントリレーで追加した予約がEventID変更検知の追従処理に引っかかる可能性があったので修正
・録画が正常に行えなかった場合でも録画後実行batを行う設定を追加（設定方法の詳細はReadme_EpgTimer.txt参照）
11/05/06　人柱版１０．３３
・番組情報に音声情報が複数表示される可能性があったので修正
・番組表で番組情報のツールチップを番組名が表示しきれない物のみ表示する設定を追加
・番組表で番組情報のツールチップが表示されるまでの時間を設定できる項目を追加
・一部のみ録画になる場合の情報通知が何度も行われるのを修正
・自動予約登録の条件に部分受信サービス同時出力のフォルダ設定が保存できていなかったのを修正
・番組表から録画モードと優先度を変更した場合にエラーになっていたのを修正
11/04/21　人柱版１０．３２
・EpgTimerNWの録画情報ダイアログからの再生に問題があったので修正
・チャンネル切換後、スクランブル解除モジュールにデータを渡すタイミングを少し変更
11/04/20　人柱版１０．３１．１
・バイナリの入れ替え忘れがあったので入れ替え
11/04/20　人柱版１０．３１
・EpgTimerNWのストリーミング再生が未実装だったので実装
・EpgTimerNWで接続が行われるまでに予約情報の取得を行わないように変更
・EpgTimerNWで接続するとEpgTimerの情報が更新されない場合があったので修正
・録画プリセットが正常に保存できていなかったので修正
・録画設定の含まれるダイアログでタブの移動をすると設定内容がリセットされるのを修正
・デザインの統一ができていない場所あったので修正
・同一物理チャンネルで連続となる予約の割り振り処理を少し変更
11/04/17　人柱版１０．３０
・チャンネルスキャンで同一サービスが連続で検出されたときに問題あったので修正
・デフォルトの録画マージンでマイナス値を設定してると問題あったので修正
・検索時のサービス絞り込みに指定ネットワークの映像サービスにチェックのみ入れるボタンを追加
（他のチェックをクリアしません）
・検索結果のスクロールバーのダブルクリックを反応しないように変更
・部分受信同時出力のコードがおかしくなっていたので修正
11/04/14　人柱版１０．２９
・番組表を大きく変更
・設定タブを整理
・録画設定をプリセットで複数持てるように変更
・検索時の条件を変更
・追従処理を修正
・バルーンチップによる状態通知を追加
・サーバー連携機能を追加
・視聴、録画時のEPG取得開始時間を設定できるように変更
・その他GUI、内部処理をいろいろと変更
11/03/19　人柱版１０．２８
・EventIDが変更になったEPG予約があると2重に予約が登録される可能性あったので修正
・時間未定の追従を指定時間行うと問題があったので修正
11/03/13　人柱版１０．２７
・追従処理を少し変更
・エラーログに使用BonDriveを追加
・機能追加に向けて内部処理いろいろ変更
11/02/20　人柱版１０．２６
・一度もEpgTimerのWindowを開かないでタスクアイコンの右クリックメニューから設定を行おうとするとエラーになるのを修正
・機能追加に向けて内部処理いろいろ変更
11/02/06　人柱版１０．２５
・１つの予約に２つ以上の録画フォルダが指定されている場合に、ファイル名PlugInの設定に問題あったので修正
・自動予約登録のプログラム予約で録画フォルダが指定されている場合に、ファイル名PlugInの設定に問題あったので修正
・Vista以降のOSの場合、サスペンド抑制通知にAwayModeのフラグを追加
・著作権法改正の動向がよくわからないので、とりあえずB25Decoder.dllの同梱はやめました
このまま成立した場合に、関連する他のアプリ含めてどうなるのか法律に詳しい人の意見を聞いてみたいと思ったり
11/01/29　人柱版１０．２４
・Twitter機能を無効にしても、録画開始のツイートしていたので修正
・スクランブル解除するの設定が設定後すぐに反映されていなかったので修正
・予約情報に不正な時間があった場合に落ちる場合あったので修正（落ちなくても正常に動作はしない可能性あります）
11/01/26　人柱版１０．２３
・EpgTimerNWでiEPGからの登録に問題あったので修正
・メイリオフォントがインストールされてない環境でエラーになっていたかもしれないので修正
・予約録画時チューナー初期化失敗で別チューナーに予約が割り振られたときに問題あったので修正
・EpgDataCap_Bonの内部TSバッファがオーバーフローしたときに問題あったので修正
（オーバーフローが発生する＝処理が追いついていない なのでまともに動作しません）
11/01/23　人柱版１０．２２
・EpgDataCap_Bonのサービス設定でサービスの削除に問題あったので修正
11/01/16　人柱版１０．２１
・EpgTimerNWでツールチップ表示を抑制していると、使用予定チューナーで問題あったので修正
・iniファイルで予約割り振りのアルゴリズムを別のものに変更できる設定を追加
（詳しくはReadme_EpgTimer.txtを参照）
11/01/10　人柱版１０．２０
・EpgTimerNWにEPGデータを自動的に読み込まない設定を追加（EPG再読み込みでデータロード）
・即時録画時のキャンセルで確認ダイアログが出るように変更
・Write_Default.dllで上書き保存設定でもエラーになった場合は別ファイル名で保存するように変更
11/01/05　人柱版１０．１９
・EpgTimerNWの自動予約登録のプログラム予約で曜日の選択に問題あったので修正
・空き容量不足で別ドライブへの保存変更が発生した場合に、データを一部保存できていなかったのを修正
・ストリーミング再生時にサーバーがスタンバイ／休止に移行した場合、送信処理を終了するように変更
・録画後動作の抑制条件に追っかけ再生、ストリーミング再生を行っている場合を追加
・EpgDataCap_Bonに次回起動時に終了前のサービスで起動する設定を追加
（強制的に起動時のサービス指定したい場合はReadme_EpgDataCap_Bon.txtを参照）
・検索結果のダブルクリックから予約登録/変更ダイアログが出るように変更
10/12/30　人柱版１０．１８
・TVTest用PlugInでストリーミング再生中ではないときにGUIが表示されるのを修正
・EpgTimerNWの録画設定のデフォルト設定で録画モードが保存されなくなっていたのを修正
・終了時間未定の追従が発生したときに、録画開始のツイートを連続で送ることがあったのを修正
（最初の開始時のみツイートします）
10/12/26　人柱版１０．１７
・ストリーミング再生時に自動的にBonDriverと受信ポートを変更するように変更
・ストリーミング再生時のGUIに終了ボタンを追加
・特定の動作状況をTwitterにツイートする機能を追加
・予約割り振りで同一物理チャンネルで連続となるチューナーの使用を優先する設定を追加
10/12/25　人柱版１０．１６
・自動削除機能の仕様を変更
（録画予定フォルダ内での削除で足りない場合に、登録されてる削除フォルダ全体から削除処理実行）
・予約配置の内部最適化処理前に、チューナー不足になる予約がある場合に問題あったので修正
・TVTest用PlugInにストリーミング再生対応機能を追加（追っかけ再生、ストリーミング再生機能を使うには必須になります）
・追っかけ再生機能を追加（録画中のものをUDP or TCPで送信します。TVTest連携の設定が必要です）
・EpgTimerNWにストリーミング再生機能を追加（TVTest連携の設定が必要です）
・EpgTimerNWのカスタムボタン機能に実装抜けあったので修正
・録画設定のデフォルトで設定が保存されていない項目があったので修正
・EpgTimerNWでファイル名マクロなしが設定できなかったので修正
・EpgTimerNWに起動時に前回のサーバーに接続する設定を追加
・EpgTimerNWに休止/スタンバイ移行時にEpgTimerNWを終了する設定を追加
10/12/12　人柱版１０．１５
・録画モードでデコードなしの設定でもデコードしていたので修正
（ただし、ネットワーク送信を開始したり、他のサービスでデコードする設定に
なっているものがある場合はデコード処理します）
・視聴モードの処理がいくつか未実装だったので実装
・ChSetのファイル作成時の処理変更
Twitter機能はDLLがあればそのまま使えます
10/12/05　人柱版１０．１４
・録画設定のデフォルト値にファイル名PlugInの設定が反映されていなかったので修正
10/12/04　人柱版１０．１３
・録画設定にファイル名PlugInを指定できる項目追加（個別録画フォルダとして設定）
・検索条件にジャンルと時間絞り込みをNOT扱いにできる設定を追加
・BS1とBS2のBitrate.iniの値を変更
・NetWorkTV終了処理に問題あったので修正
10/11/23　人柱版１０．１２
・予約割り振りの処理を少し変更
・EPGデータ読み込み中の排他制御を少し強化
・番組表以外のツールチップを表示しない設定を追加
・EpgTimerNWの右クリックメニューに再生が残っていたので削除
・同一チャンネルの別サービスの予約が登録できなくなっていたので修正
10/11/21　人柱版１０．１１
・放送波時間でPC時計と同期する設定を追加（Vista以降ではEpgTimerSrv.exeが
　管理者権限で起動（サービスとして起動など）している必要があります）
・EpgDataCap_Bonからの手動でのサービス切り替え時は物理チャンネルを考慮
　するように変更（予約録画時に物理チャンネル考慮できないのは仕様）
・予約削除時にメモリリークあったので修正
・予約割り振り時にメモリリークあったので修正
・NetWorkTV終了時にTVTestも終了するように変更
・エラー処理少し強化
10/11/19　人柱版１０．１０
・録画済み一覧自動削除の設定値が間違っていたので修正
10/11/18　人柱版１０．９
・予約情報更新時に内部保持リストのリセットができていなかったので修正
・EPG取得でデッドロック発生する可能性あったので修正
・録画済み一覧の自動削除機能追加
10/11/14　人柱版１０．８
・番組表の表示に問題あったので修正
10/11/14　人柱版１０．７
・自動削除の削除対象フォルダの判定に問題あったので修正
・ネットワークドライブの空き容量がうまく取得できていなかったので修正
・サービス表示設定の削除で選択項目以外が削除されることあったので修正
10/11/10　人柱版１０．６
・追従処理に問題あったので修正
・録画結果の種類追加
10/11/07　人柱版１０．５
・追従処理少し変更
・番組表の予約枠に一部実行の色を変更できる設定を追加
・番組表のツールチップを表示しない設定を追加
・番組情報変更時の情報更新に一部問題あったので修正
・EpgTimerNWにぴったり？録画の項目が表示されてなかったので修正
10/10/30　人柱版１０．４
・TVTest連携にNetworkTVモードを追加（EpgDataCap_BonからUDP or TCP送信するモード）
・ボタンに「NetworkTV終了」を追加
・動作設定に録画時のプロセス優先度指定できる設定追加
・エラーログにSignal値を追加
・EpgTimerNWにカスタムボタンとTVTest連携（NetworkTVモードのみ）を追加
（起動するプロセスはローカルPCのものになります）
・テストコード残っていたので削除
10/10/25　人柱版１０．３
・番組情報変更時の情報更新に一部問題あったので修正
・GC.Collect()を行うようにしてみた
10/10/20　人柱版１０．２
・自動予約登録の録画モード無効時の動作がおかしくなっていたので修正
・EpgTimerNWに休止とスタンバイのボタンを追加
　（※ローカルマシンではなく、接続先のサーバーに対する動作になります）
・EpgTimerNWの接続時にIPの変わりにドメイン名を指定できるように変更
10/10/17　人柱版１０．１
・録画後バッチのマクロに「$Drops$」、「$Scrambles$」、「$Result$」を追加
・自動予約登録時にイベントグループ考慮するように変更
・textにいくつか追記
10/10/08　人柱版１０
・NOTキーワードはあいまい検索を行わないように変更
・textは一部まだ書きかけ


■動作確認環境■
OS    　　：Windows7 Ultimate 64bit版
CPU   　　：Intel Core i7 860
メモリ　　：4GB
VGA       ：ATI Radeon HD 4670
チューナー：PT1×1、PT2×1
