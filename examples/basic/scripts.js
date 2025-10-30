(function(){
  const statusEl = document.getElementById('status');
  const btn400 = document.getElementById('btn400');
  const btnClear = document.getElementById('btnClear');

  function log(level, text){
    const d = document.createElement('div');
    d.className = 'item';
    d.innerHTML = '<span class="tag">' + (level||'info') + '</span> ' + escapeHtml(text);
    statusEl.prepend(d);
  }

  function escapeHtml(s){
    return s.replace(/[&<>"]/g, c => ({'&':'&amp;','<':'&lt;','>':'&gt;','"':'&quot;'}[c]));
  }

  btnClear.addEventListener('click', () => {
    statusEl.innerHTML = '<div class="item"><span class="tag">info</span>Status log cleared.</div>';
  });

  btn400.addEventListener('click', async () => {
    log('run', 'Starting malformed-request attempts... (watch server logs)');

    // 1) Invalid percent-encoding
    try {
      const badPath = '/%';
      log('try', 'fetch to "' + badPath + '"');
      const r1 = await fetch(badPath, { method: 'GET', cache: 'no-store' });
      log('result', `fetch ${badPath} → status: ${r1.status} ${r1.statusText}`);
    } catch (e) {
      log('error', 'fetch "/%" failed: ' + e.message);
    }

    // 2) Invalid percent hex
    try {
      const badPath2 = '/%zz';
      log('try', 'fetch to "' + badPath2 + '"');
      const r2 = await fetch(badPath2, { method: 'GET', cache: 'no-store' });
      log('result', `fetch ${badPath2} → status: ${r2.status} ${r2.statusText}`);
    } catch (e) {
      log('error', 'fetch "/%zz" failed: ' + e.message);
    }

    // 3) Null byte in path
    try {
      const nullPath = '/\0-null-test';
      log('try', 'fetch to path with null char');
      const r3 = await fetch(nullPath, { method: 'GET', cache: 'no-store' });
      log('result', `fetch ${encodeURI(nullPath)} → status: ${r3.status} ${r3.statusText}`);
    } catch (e) {
      log('error', 'fetch with null char failed: ' + e.message);
    }

    // 4) XHR illegal header
    try {
      log('try', 'XMLHttpRequest with CRLF in header (will likely throw)');
      const xhr = new XMLHttpRequest();
      xhr.open('GET', '/', true);
      try {
        xhr.setRequestHeader('X-Bad', 'bad\r\nvalue');
        xhr.onreadystatechange = function(){ if(xhr.readyState===4) log('result','XHR status: ' + xhr.status); };
        xhr.send();
      } catch (innerErr) {
        log('error','XHR setRequestHeader threw: ' + innerErr.message);
      }
    } catch (e) {
      log('error','XHR failed: ' + e.message);
    }

    // 5) Image load with weird URL
    try {
      const imgBad = new Image();
      const random = Math.floor(Math.random()*999999);
      const srcBad = location.origin + '/%?bad=' + random;
      log('try', 'Image load to "' + srcBad + '"');
      imgBad.onload = () => log('result', 'Image loaded (unexpected)');
      imgBad.onerror = () => log('error', 'Image load failed (network error or server error)');
      imgBad.src = srcBad;
    } catch (e) {
      log('error', 'Image trick failed: ' + e.message);
    }

    // 6) Invalid hostname
    try {
      const illegal = 'http://%zz/';
      log('try', 'window.open illegal URL "' + illegal + '"');
      const w = window.open(illegal, '_blank');
      if(!w) log('note','window.open blocked or refused the URL.');
      else log('note','window.open returned window — check server logs.');
    } catch (e) {
      log('error','window.open failed: ' + e.message);
    }

    log('done', 'Malformed-request attempts finished. Check your SSFHS logs.');
  });
})();
