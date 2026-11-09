<?xml version="1.0" encoding="utf-8"?>
<!DOCTYPE TS>
<TS version="2.1" language="ja_JP">
<context>
    <name>ImportDialog</name>
    <message>
        <location filename="../src/importdialog.cpp" line="17"/>
        <source>Import curl Command</source>
        <translation>curlコマンドをインポート</translation>
    </message>
    <message>
        <location filename="../src/importdialog.cpp" line="24"/>
        <source>Paste a curl command, for example from your browser&apos;s developer tools (Network tab → right-click a request → Copy → Copy as cURL). Both bash and Windows cmd formats are supported.</source>
        <translation>curlコマンドを貼り付けてください。たとえばブラウザーの開発者ツールで、「ネットワーク」タブのリクエストを右クリック →「コピー」→「cURL としてコピー」で取得できます。bash 形式と Windows の cmd 形式の両方に対応しています。</translation>
    </message>
    <message>
        <location filename="../src/importdialog.cpp" line="45"/>
        <source>Import</source>
        <translation>インポート</translation>
    </message>
    <message>
        <location filename="../src/importdialog.cpp" line="46"/>
        <source>Cancel</source>
        <translation>キャンセル</translation>
    </message>
    <message>
        <location filename="../src/importdialog.cpp" line="72"/>
        <source>Paste a curl command first.</source>
        <translation>先にcurlコマンドを貼り付けてください。</translation>
    </message>
    <message>
        <location filename="../src/importdialog.cpp" line="75"/>
        <source>A quote is never closed. Make sure you copied the whole command.</source>
        <translation>閉じられていない引用符があります。コマンド全体をコピーしたか確認してください。</translation>
    </message>
    <message>
        <location filename="../src/importdialog.cpp" line="78"/>
        <source>This doesn&apos;t look like a curl command. It should start with &quot;curl&quot;.</source>
        <translation>curlコマンドではないようです。「curl」で始まる必要があります。</translation>
    </message>
    <message>
        <location filename="../src/importdialog.cpp" line="81"/>
        <source>No URL was found in the command.</source>
        <translation>コマンドにURLが見つかりません。</translation>
    </message>
</context>
<context>
    <name>MainWindow</name>
    <message>
        <location filename="../src/mainwindow.cpp" line="377"/>
        <source>Kurly – curl command builder</source>
        <translation>Kurly – curlコマンドビルダー</translation>
    </message>
    <message>
        <location filename="../src/mainwindow.cpp" line="379"/>
        <source>Import curl command…</source>
        <translation>curlコマンドをインポート…</translation>
    </message>
    <message>
        <location filename="../src/mainwindow.cpp" line="380"/>
        <source>Fill in the form from an existing curl command, such as one copied from a browser&apos;s developer tools (%1)</source>
        <translation>既存のcurlコマンド(ブラウザーの開発者ツールからコピーしたものなど)からフォームに入力します(%1)</translation>
    </message>
    <message>
        <location filename="../src/mainwindow.cpp" line="384"/>
        <source>URL:</source>
        <translation>URL:</translation>
    </message>
    <message>
        <location filename="../src/mainwindow.cpp" line="385"/>
        <source>https://example.com/api</source>
        <translation>https://example.com/api</translation>
    </message>
    <message>
        <location filename="../src/mainwindow.cpp" line="386"/>
        <source>Method:</source>
        <translation>メソッド:</translation>
    </message>
    <message>
        <location filename="../src/mainwindow.cpp" line="388"/>
        <source>Headers</source>
        <translation>ヘッダー</translation>
    </message>
    <message>
        <location filename="../src/mainwindow.cpp" line="389"/>
        <source>Name</source>
        <translation>名前</translation>
    </message>
    <message>
        <location filename="../src/mainwindow.cpp" line="389"/>
        <source>Value</source>
        <translation>値</translation>
    </message>
    <message>
        <location filename="../src/mainwindow.cpp" line="390"/>
        <source>Add header</source>
        <translation>ヘッダーを追加</translation>
    </message>
    <message>
        <location filename="../src/mainwindow.cpp" line="391"/>
        <source>Remove selected</source>
        <translation>選択を削除</translation>
    </message>
    <message>
        <location filename="../src/mainwindow.cpp" line="393"/>
        <source>Request body</source>
        <translation>リクエストボディ</translation>
    </message>
    <message>
        <location filename="../src/mainwindow.cpp" line="394"/>
        <source>Data to send (e.g. JSON)</source>
        <translation>送信するデータ(例: JSON)</translation>
    </message>
    <message>
        <location filename="../src/mainwindow.cpp" line="396"/>
        <source>Authentication and other</source>
        <translation>認証・その他</translation>
    </message>
    <message>
        <location filename="../src/mainwindow.cpp" line="397"/>
        <source>User (user:password):</source>
        <translation>ユーザー(ユーザー:パスワード):</translation>
    </message>
    <message>
        <location filename="../src/mainwindow.cpp" line="398"/>
        <source>User agent:</source>
        <translation>ユーザーエージェント:</translation>
    </message>
    <message>
        <location filename="../src/mainwindow.cpp" line="399"/>
        <source>Timeout:</source>
        <translation>タイムアウト:</translation>
    </message>
    <message>
        <location filename="../src/mainwindow.cpp" line="400"/>
        <source>None</source>
        <translation>なし</translation>
    </message>
    <message>
        <location filename="../src/mainwindow.cpp" line="401"/>
        <source> s</source>
        <translation> 秒</translation>
    </message>
    <message>
        <location filename="../src/mainwindow.cpp" line="402"/>
        <source>Save output to file:</source>
        <translation>出力をファイルに保存:</translation>
    </message>
    <message>
        <location filename="../src/mainwindow.cpp" line="403"/>
        <source>When run from Kurly, relative paths are saved in your home folder.</source>
        <translation>Kurlyから実行した場合、相対パスはホームフォルダーを基準に保存されます。</translation>
    </message>
    <message>
        <location filename="../src/mainwindow.cpp" line="405"/>
        <source>Options</source>
        <translation>オプション</translation>
    </message>
    <message>
        <location filename="../src/mainwindow.cpp" line="406"/>
        <source>Follow redirects</source>
        <translation>リダイレクトに従う</translation>
    </message>
    <message>
        <location filename="../src/mainwindow.cpp" line="407"/>
        <source>Skip TLS certificate verification</source>
        <translation>TLS証明書の検証をスキップ</translation>
    </message>
    <message>
        <location filename="../src/mainwindow.cpp" line="408"/>
        <source>Verbose output</source>
        <translation>詳細出力</translation>
    </message>
    <message>
        <location filename="../src/mainwindow.cpp" line="409"/>
        <source>Silent mode</source>
        <translation>サイレントモード</translation>
    </message>
    <message>
        <location filename="../src/mainwindow.cpp" line="410"/>
        <source>Include response headers</source>
        <translation>レスポンスヘッダーを含める</translation>
    </message>
    <message>
        <location filename="../src/mainwindow.cpp" line="411"/>
        <source>Fail on HTTP errors</source>
        <translation>HTTPエラー時に失敗とする</translation>
    </message>
    <message>
        <location filename="../src/mainwindow.cpp" line="412"/>
        <source>Request compressed response</source>
        <translation>圧縮レスポンスを要求</translation>
    </message>
    <message>
        <location filename="../src/mainwindow.cpp" line="414"/>
        <source>Language:</source>
        <translation>言語:</translation>
    </message>
    <message>
        <location filename="../src/mainwindow.cpp" line="415"/>
        <source>Command:</source>
        <translation>コマンド:</translation>
    </message>
    <message>
        <location filename="../src/mainwindow.cpp" line="416"/>
        <source>Enter a URL to generate the command.</source>
        <translation>URLを入力するとコマンドが生成されます。</translation>
    </message>
    <message>
        <location filename="../src/mainwindow.cpp" line="417"/>
        <source>Use long option names</source>
        <translation>長いオプション名を使う</translation>
    </message>
    <message>
        <location filename="../src/mainwindow.cpp" line="418"/>
        <source>Split across multiple lines</source>
        <translation>複数行に分割</translation>
    </message>
    <message>
        <location filename="../src/mainwindow.cpp" line="419"/>
        <location filename="../src/mainwindow.cpp" line="516"/>
        <source>Copy command</source>
        <translation>コマンドをコピー</translation>
    </message>
    <message>
        <location filename="../src/mainwindow.cpp" line="420"/>
        <source>Response:</source>
        <translation>レスポンス:</translation>
    </message>
    <message>
        <location filename="../src/mainwindow.cpp" line="421"/>
        <source>Format JSON/XML</source>
        <translation>JSON/XMLを整形</translation>
    </message>
    <message>
        <location filename="../src/mainwindow.cpp" line="422"/>
        <source>Show JSON and XML responses indented. Available once a request finishes with a complete JSON or XML response.</source>
        <translation>JSONやXMLのレスポンスをインデントして表示します。リクエストが完了し、完全なJSONまたはXMLのレスポンスを受け取ると使用できます。</translation>
    </message>
    <message>
        <location filename="../src/mainwindow.cpp" line="424"/>
        <source>Press Run to send the request. curl&apos;s output appears here.</source>
        <translation>「実行」を押すとリクエストを送信します。curlの出力がここに表示されます。</translation>
    </message>
    <message>
        <location filename="../src/mainwindow.cpp" line="515"/>
        <source>Copied!</source>
        <translation>コピーしました!</translation>
    </message>
    <message>
        <location filename="../src/mainwindow.cpp" line="608"/>
        <source>[Output truncated]</source>
        <translation>[出力が長すぎるため省略しました]</translation>
    </message>
    <message>
        <location filename="../src/mainwindow.cpp" line="645"/>
        <source>malformed URL</source>
        <translation>URLの形式が不正です</translation>
    </message>
    <message>
        <location filename="../src/mainwindow.cpp" line="646"/>
        <source>could not resolve host</source>
        <translation>ホスト名を解決できません</translation>
    </message>
    <message>
        <location filename="../src/mainwindow.cpp" line="647"/>
        <source>could not connect to server</source>
        <translation>サーバーに接続できません</translation>
    </message>
    <message>
        <location filename="../src/mainwindow.cpp" line="648"/>
        <source>HTTP error returned</source>
        <translation>HTTPエラーが返されました</translation>
    </message>
    <message>
        <location filename="../src/mainwindow.cpp" line="649"/>
        <source>timed out</source>
        <translation>タイムアウトしました</translation>
    </message>
    <message>
        <location filename="../src/mainwindow.cpp" line="650"/>
        <source>TLS/SSL connection error</source>
        <translation>TLS/SSL接続エラー</translation>
    </message>
    <message>
        <location filename="../src/mainwindow.cpp" line="651"/>
        <source>too many redirects</source>
        <translation>リダイレクトが多すぎます</translation>
    </message>
    <message>
        <location filename="../src/mainwindow.cpp" line="652"/>
        <source>server returned nothing</source>
        <translation>サーバーから応答がありません</translation>
    </message>
    <message>
        <location filename="../src/mainwindow.cpp" line="653"/>
        <source>TLS certificate problem</source>
        <translation>TLS証明書の問題</translation>
    </message>
    <message>
        <location filename="../src/mainwindow.cpp" line="664"/>
        <source>Stop</source>
        <translation>停止</translation>
    </message>
    <message>
        <location filename="../src/mainwindow.cpp" line="664"/>
        <source>Run</source>
        <translation>実行</translation>
    </message>
    <message>
        <location filename="../src/mainwindow.cpp" line="673"/>
        <source>Running…</source>
        <translation>実行中…</translation>
    </message>
    <message>
        <location filename="../src/mainwindow.cpp" line="676"/>
        <source>%1 ms</source>
        <translation>%1 ミリ秒</translation>
    </message>
    <message>
        <location filename="../src/mainwindow.cpp" line="678"/>
        <source>Done · %1</source>
        <translation>完了 · %1</translation>
    </message>
    <message>
        <location filename="../src/mainwindow.cpp" line="682"/>
        <source>curl exited with code %1 · %2</source>
        <translation>curlが終了コード %1 で終了しました · %2</translation>
    </message>
    <message>
        <location filename="../src/mainwindow.cpp" line="683"/>
        <source>curl exited with code %1 (%2) · %3</source>
        <translation>curlが終了コード %1 で終了しました(%2) · %3</translation>
    </message>
    <message>
        <location filename="../src/mainwindow.cpp" line="690"/>
        <source>Stopped</source>
        <translation>停止しました</translation>
    </message>
    <message>
        <location filename="../src/mainwindow.cpp" line="690"/>
        <source>curl stopped unexpectedly</source>
        <translation>curlが予期せず終了しました</translation>
    </message>
    <message>
        <location filename="../src/mainwindow.cpp" line="694"/>
        <source>Could not start curl. Is it installed and on your PATH?</source>
        <translation>curlを起動できません。インストールされていて PATH に含まれているか確認してください。</translation>
    </message>
    <message>
        <location filename="../src/mainwindow.cpp" line="714"/>
        <source>Import curl Command</source>
        <translation>curlコマンドをインポート</translation>
    </message>
    <message>
        <location filename="../src/mainwindow.cpp" line="715"/>
        <source>The command was imported, but Kurly doesn&apos;t support some of its options, so they were skipped:</source>
        <translation>コマンドをインポートしましたが、Kurlyが対応していないオプションはスキップしました:</translation>
    </message>
</context>
</TS>
