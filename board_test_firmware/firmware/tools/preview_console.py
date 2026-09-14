"""Serve the firmware console locally for screenshots, without hardware requests."""

from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer
from pathlib import Path
from urllib.parse import urlsplit


SOURCE = Path(__file__).resolve().parents[1] / "src/web/WebConfigServer.cpp"


def preview_page():
    source = SOURCE.read_text(encoding="utf-8")
    page = source.split('static const char kPage[] PROGMEM = R"HTML(', 1)[1].split(')HTML";', 1)[0]
    page = page.replace(
        "</style>",
        "button:disabled{opacity:1;color:#202124}"
        ".primary:disabled,.danger:disabled{color:#fff}</style>",
    )
    page = page.replace(
        "window.addEventListener('beforeunload',()=>navigator.sendBeacon('/api/diagnostic','command=motor%20stop'));",
        "",
    )
    page = page.replace(
        "const requestedLanguage=new URLSearchParams(location.search).get('lang');",
        """text.en.subtitle='Local preview · Hardware is not connected';
text.zh.subtitle='本地截图预览 · 未连接硬件';
text.ru.subtitle='Локальный просмотр · Плата не подключена';
const requestedLanguage=new URLSearchParams(location.search).get('lang')||'ru';""",
    )
    page = page.replace(
        "loadMotorConfig();\nrefreshStatus();\n</script>",
        """document.querySelectorAll('button:not(#langToggle),input').forEach(element=>{
  element.disabled=true;
});
</script>""",
    )
    return page.encode("utf-8")


class PreviewHandler(BaseHTTPRequestHandler):
    def do_GET(self):
        if urlsplit(self.path).path not in ("/", "/motor"):
            self.send_error(404)
            return
        page = preview_page()
        self.send_response(200)
        self.send_header("Content-Type", "text/html; charset=utf-8")
        self.send_header("Content-Length", str(len(page)))
        self.send_header("Cache-Control", "no-store")
        self.end_headers()
        self.wfile.write(page)


if __name__ == "__main__":
    print("Console preview: http://127.0.0.1:8765/motor?lang=ru", flush=True)
    ThreadingHTTPServer(("127.0.0.1", 8765), PreviewHandler).serve_forever()
