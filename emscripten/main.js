// SDLPAL Web - main.js

// Progress UI helpers
var progressFill, progressPct, statusText, progressDetail;
var loadingScreen, mainMenu;

function initUI() {
    progressFill = document.getElementById('progressFill');
    progressPct = document.getElementById('progressPct');
    statusText = document.getElementById('statusText');
    progressDetail = document.getElementById('progressDetail');
    loadingScreen = document.getElementById('loadingScreen');
    mainMenu = document.getElementById('mainMenu');
}

function setProgress(pct, text, detail) {
    if (progressFill) progressFill.style.width = Math.min(pct, 100) + '%';
    if (progressPct) progressPct.textContent = Math.round(pct) + '%';
    if (statusText && text) statusText.textContent = text;
    if (progressDetail && detail) progressDetail.textContent = detail;
}

// Called when user clicks "开始游戏"
function startGame() {
    initUI();
    if (!mainMenu || !loadingScreen) return;
    mainMenu.style.display = 'none';
    loadingScreen.classList.add('active');
    setProgress(0, '正在加载游戏引擎...', '');

    // Dynamically load sdlpal.js (Emscripten WASM loader)
    var script = document.createElement('script');
    script.src = 'sdlpal.js';
    script.onerror = function() { setProgress(0, '引擎加载失败，请刷新页面重试', ''); };
    script.onload = function() { console.log('sdlpal.js loaded'); };
    document.body.appendChild(script);
}

// Data loading after WASM is ready
function runDataLoading() {
    setProgress(30, '正在检查缓存...', '');
    try { FS.mkdir('/data'); } catch(e) {}
    FS.mount(IDBFS, {}, '/data');

    // Sync from IndexedDB cache first
    FS.syncfs(true, function(err) {
        // Check if data is already cached
        var hasData = false;
        try { if (FS.stat('/data/fbp.mkf').size > 0) hasData = true; } catch(e) {}

        if (hasData) {
            setProgress(60, '使用缓存数据...', '');
            setTimeout(function() {
                setProgress(95, '正在同步...', '');
                FS.syncfs(false, function() {
                    setProgress(100, '正在启动游戏...', '');
                    setTimeout(function() { launchGame(); }, 200);
                });
            }, 300);
        } else {
            // No cache - download data.zip
            doDownloadAndExtract();
        }
    });
}

function doDownloadAndExtract() {
    setProgress(35, '正在下载游戏数据...', '');

    fetch('data.zip').then(function(r) {
        if (!r.ok) throw new Error('HTTP ' + r.status);
        var total = parseInt(r.headers.get('Content-Length') || '0');
        var loaded = 0;
        if (r.body) {
            var reader = r.body.getReader();
            var chunks = [];
            function pump() {
                return reader.read().then(function(result) {
                    if (result.done) return new Blob(chunks);
                    chunks.push(result.value);
                    loaded += result.value.length;
                    var pct = 35 + (loaded / total) * 20;
                    setProgress(pct, '正在下载游戏数据...', (loaded / 1048576).toFixed(1) + 'MB / ' + (total / 1048576).toFixed(1) + 'MB');
                    return pump();
                });
            }
            return pump();
        }
        return r.blob();
    }).then(function(blob) {
        setProgress(57, '正在解压游戏资源...', '');
        var zip = new JSZip();
        return zip.loadAsync(blob).then(function(z) {
            var entries = [];
            z.forEach(function(path, entry) {
                if (!entry.dir && !path.includes('._')) entries.push({path:path, entry:entry});
            });
            var done = 0;
            function extractNext() {
                if (done >= entries.length) return Promise.resolve();
                var item = entries[done];
                return item.entry.async('uint8array').then(function(arr) {
                    try { FS.writeFile('/data/' + item.path.toLowerCase(), arr, {encoding:'binary'}); } catch(e) {}
                    done++;
                    var pct = 60 + (done / entries.length) * 33;
                    setProgress(pct, '正在解压 ' + item.path.toLowerCase(), done + '/' + entries.length);
                    return extractNext();
                });
            }
            return extractNext();
        });
    }).then(function() {
        setProgress(95, '正在写入缓存...', '');
        return new Promise(function(resolve) { FS.syncfs(false, function() { resolve(); }); });
    }).then(function() {
        setProgress(100, '正在启动游戏...', '');
        setTimeout(function() { launchGame(); }, 300);
    }).catch(function(e) {
        setProgress(0, '加载失败: ' + e.message, '');
        console.error('Data load error:', e);
    });
}

// The Module object for Emscripten
window.Module = {
    preRun: [],
    postRun: [],
    print: function(text) { console.log(text); },
    printErr: function(text) { console.error(text); },
    canvas: (function() {
        var c = document.getElementById('canvas');
        if (c) {
            c.addEventListener("webglcontextlost", function(e) { alert('WebGL context lost. Reload the page.'); e.preventDefault(); }, false);
        }
        return c;
    })(),
    setStatus: function(text) {
        if (!text) return;
        var m = text.match(/([^(]+)\((\d+(\.\d+)?)\/(\d+)\)/);
        if (m) {
            setProgress(parseInt(m[2]) / parseInt(m[4]) * 30, m[1], m[2] + '/' + m[4]);
        }
    },
    totalDependencies: 0,
    monitorRunDependencies: function(left) {
        this.totalDependencies = Math.max(this.totalDependencies, left);
        if (left) {
            var loaded = this.totalDependencies - left;
            setProgress(loaded / this.totalDependencies * 30, '引擎加载中', loaded + '/' + this.totalDependencies);
        }
    },
    onRuntimeInitialized: function() {
        runDataLoading();
    }
};

function launchGame() {
    var ok = false;
    try { if (FS.stat('/data/fbp.mkf').size > 0) ok = true; } catch(e) {}
    if (!ok) {
        setProgress(0, '游戏数据加载失败', '');
        return;
    }
    setProgress(100, '进入游戏中...', '');
    loadingScreen.classList.remove('active');
    mainMenu.style.display = 'none';

    // Show virtual controller on touch devices
    initVirtualController();

    var func = Module.cwrap('EMSCRIPTEN_main', 'number', ['number', 'number'], {async:true});
    func(0, 0);
}

// Virtual Controller
function initVirtualController() {
    var vc = document.getElementById('vcontroller');
    if (!vc) return;

    // Only show on touch devices
    if ('ontouchstart' in window || navigator.maxTouchPoints > 0) {
        vc.classList.add('active');
    }

    // Touch event handling for each button
    var buttons = vc.querySelectorAll('.ctrl-btn');
    buttons.forEach(function(btn) {
        var key = btn.getAttribute('data-key');
        if (!key) return;

        function onStart(e) {
            e.preventDefault();
            btn.classList.add('pressed');
            dispatchKey(key, true);
        }
        function onEnd(e) {
            e.preventDefault();
            btn.classList.remove('pressed');
            dispatchKey(key, false);
        }

        btn.addEventListener('touchstart', onStart, {passive:false});
        btn.addEventListener('touchend', onEnd, {passive:false});
        btn.addEventListener('touchcancel', onEnd, {passive:false});
        btn.addEventListener('mousedown', onStart);
        btn.addEventListener('mouseup', onEnd);
        btn.addEventListener('mouseleave', onEnd);
    });
}

function dispatchKey(key, isDown) {
    var event = new KeyboardEvent(isDown ? 'keydown' : 'keyup', {
        key: key,
        code: key,
        bubbles: true,
        cancelable: true
    });
    document.dispatchEvent(event);
}
