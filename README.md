<div>
<a href="https://sunos.saita.ma/mod_cgidso.html">https://sunos.saita.ma/mod_cgidso.html</a> から引用<hr>
<h1>Apache 2.x モジュール mod_cgidso</h1>
<h2>特徴</h2>
    <div>
        <p>普通の CGI は実行ファイルを fork(), exec() して呼び出しますが，
            このモジュールでは共有オブジェクトを dlopen(), dlsym() して呼び出します．
            構造的には，Apache モジュールのうちコンテンツ出力部分を分離して別ファイルとし，
            httpd との接続部分をこのモジュールが受け持つ形になっています．</p>
        <ul>
            <li>普通の CGI のようなプロセス生成のオーバヘッドが発生せず，サーバ負荷軽減につながる．</li>
            <li>CGI をそのまま Apache モジュールに置き換えた場合，プログラムの
                追加や変更の度に httpd の再起動が必要となるが，このモジュールを用いれば
                httpd の再起動なしに手軽にプログラムの追加・変更が可能となる．</li>
        </ul>
        <p>といったメリットがあります．</p>
        <p><small># 本来の意味での CGI とは異なるので，モジュール名に "cgi" と入っているのは
            おかしいと言えばおかしいのですが，そこはあまり深く考えないように......</small></p>
        <p>このモジュールはマルチスレッド MPM に対応しています．</p>
        <p><i>★ <a href="2ch-cgi-lw.html" title="&lt;!--#flastmod virtual='2ch-cgi-lw.html' --&gt;">2ch CGI 軽量化</a> や
            <a href="http://maido3.com/server/option/ccgi.html" target="_blank">レンタルサーバ BIG-server.com オプションサービス</a>
            で利用して頂いています ★</i></p>
    </div>

<h2>ハンドラ</h2>
    <dl>
        <dt>dso-script</dt>
        <dd>
            <p>このハンドラが割り当てられたファイルがリクエストされると，
                そのファイルはこのモジュールによって処理されます．</p>
            <p>使用例:</p>
<blockquote><p><code>
&lt;IfModule mod_cgidso.c&gt;<br>
&nbsp; &nbsp; AddHandler dso-script so<br>
&lt;/IfModule&gt;
</code></p></blockquote>
            <p>このモジュールから呼び出される共有オブジェクトのソースの例:
                <a href="dso-example.c" title="&lt;!--#flastmod virtual='dso-example.c' --&gt;">dso-example.c</a></p>
            <p>共有オブジェクトのビルドには apxs を使うのが最も簡単です．</p>
<blockquote><p><code>
apxs -c source.c [source2.c source3.c ...]
</code></p></blockquote>
            <p>により .libs/source.so が生成されます．</p>
            <p><b style="text-decoration:underline;">このモジュールから呼び出される共有オブジェクトに関する注意点</b></p>
            <ul>
                <li>普通の CGI と異なり，httpd プロセスと同一メモリ空間で実行され，
                    処理終了時にプロセスは終了しません．標準 C 関数などを使用する場合
                    明示的にリソース解放を行わないと，どんどんリークしていきます．
                    できるだけ APR 関数の使用をお勧めします．APR 関数を使用すれば
                    処理終了時のリソース解放が自動的に行われます．もし APR 関数のことを
                    よく知らないなら，<a href="http://apr.apache.org/docs/apr/" target="_blank">Apache Portable Runtime Documentation</a>
                    を参照して下さい．あるいは，Apache のヘッダファイルもヒントになるでしょう．
<blockquote><p><code>
apxs -q INCLUDEDIR
</code></p></blockquote>
                    によりヘッダファイルのあるディレクトリを調べられます．</li>
                <li>exit() は使用しないで下さい．使用すると httpd プロセスごと終了してしまいます．</li>
                <li>マルチスレッド MPM で使用する場合，共有オブジェクトも thread-safe でなければなりません．
                    thread safety に関しては以下を参照して下さい．
                    <ul>
                        <li><a href="http://httpd.apache.org/docs-2.1/developer/thread_safety.html" target="_blank">
                            Apache 2.0 Thread Safety Issues</a></li>
                        <li><a href="http://docs.sun.com/app/docs/doc/819-0390/6n2qp46ft?a=view" target="_blank">
                            プログラミング上の指針 - マルチスレッドのプログラミング</a></li>
                    </ul></li>
            </ul>
        </dd>
    </dl>

<h2>サーバ負荷対策 Tips</h2>
    <div>
        <p><a href="dccache.html" title="&lt;!--#flastmod virtual='dccache.html' --&gt;">
            CGI 出力キャッシュ</a>で述べているように，mod_mem_cache の利用は負荷軽減にはかなり効果的です．</p>
        <p>Apache 2.2 では mod_proxy_balancer というロードバランサ・負荷分散モジュールも実装されています．
            また，mod_cache / mod_proxy なども 2.0 までのものよりかなり改良されています．
            さらに，Event MPM という Keep Alive 時の負荷軽減を図った MPM も実装されています．</p>
    </div>

<h2>ダウンロード</h2>
    <dl>
        <!--<dt><a href="mod_cgidso.c">mod_cgidso.c</a>
            <small><i style="font-weight:normal;">size: #fsize virtual="mod_cgidso.c" bytes
            #flastmod virtual="mod_cgidso.c" </i></small></dt>-->
        <dd>このモジュールは <a href="http://apache.org/licenses/LICENSE-2.0" target="_blank">Apache License, Version 2.0</a> に基づき提供します．</dd>
    </dl>
</div>
