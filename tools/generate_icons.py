from pathlib import Path
from PIL import Image, ImageDraw, ImageFont
import math

ROOT = Path(__file__).resolve().parents[1]
SVG_DIR = ROOT / "assets" / "icons" / "svg"
PNG_DIR = ROOT / "assets" / "icons" / "png" / "16"
PREVIEW_PATH = ROOT / "assets" / "icons" / "preview.png"

BLUE = "#42a5f5"

UI_ICONS = [
    "app", "file", "folder", "chat", "trash", "search", "chevron-up",
    "chevron-down", "close", "copy", "refresh", "branch", "edit", "save",
    "cancel", "image", "code", "key", "model", "pin", "unpin", "warning",
    "info", "database", "terminal", "globe", "settings", "profiles",
    "clear-chat", "theme", "quit", "new-chat", "send", "stop", "export",
    "advanced", "attach", "history", "download", "upload", "play", "pause",
]

TILE_ICONS = {
    "document": ("DOC", "#64b5f6"),
    "markdown": ("MD", "#4dd0e1"),
    "json": ("{}", "#fdd835"),
    "yaml": ("YML", "#ffb74d"),
    "css": ("CSS", "#29b6f6"),
    "javascript": ("JS", "#fdd835"),
    "typescript": ("TS", "#42a5f5"),
    "html": ("<>", "#ff7043"),
    "cpp": ("C++", "#7e57c2"),
    "csharp": ("C#", "#66bb6a"),
    "python": ("PY", "#ffd54f"),
    "powershell": (">_", "#42a5f5"),
    "image-file": ("IMG", "#64b5f6"),
    "audio": ("AUD", "#26c6da"),
    "video": ("VID", "#5c6bc0"),
    "archive": ("ZIP", "#ffca28"),
    "pdf": ("PDF", "#ef5350"),
    "word": ("W", "#42a5f5"),
    "excel": ("X", "#66bb6a"),
    "powerpoint": ("P", "#ff7043"),
    "package": ("PKG", "#ffca28"),
    "config": ("CFG", "#90a4ae"),
    "lock": ("LCK", "#ffd54f"),
    "test": ("TST", "#81c784"),
    "database-file": ("DB", "#fdd835"),
    "server": ("SRV", "#26c6da"),
    "cloud": ("CLD", "#4fc3f7"),
    "git": ("GIT", "#ff7043"),
    "github": ("GH", "#cfd8dc"),
    "docker": ("DOC", "#29b6f6"),
    "cmake": ("CM", "#ef5350"),
    "npm": ("NPM", "#ef5350"),
    "node": ("ND", "#66bb6a"),
    "rust": ("RS", "#ff8a65"),
    "go": ("GO", "#4dd0e1"),
    "java": ("JV", "#ff7043"),
    "php": ("PHP", "#9575cd"),
    "ruby": ("RB", "#ef5350"),
    "swift": ("SW", "#ff8a65"),
    "sql": ("SQL", "#fdd835"),
}


def font(size, bold=False):
    names = [
        "C:/Windows/Fonts/segoeuib.ttf" if bold else "C:/Windows/Fonts/segoeui.ttf",
        "C:/Windows/Fonts/arialbd.ttf" if bold else "C:/Windows/Fonts/arial.ttf",
    ]
    for name in names:
        if Path(name).exists():
            return ImageFont.truetype(name, size)
    return ImageFont.load_default()


def svg_shell(content):
    return (
        '<svg xmlns="http://www.w3.org/2000/svg" width="24" height="24" '
        'viewBox="0 0 24 24" fill="none">\n'
        f"{content}\n</svg>\n"
    )


def line_svg(color, body, width=1.8):
    return svg_shell(
        f'<g stroke="{color}" stroke-width="{width}" stroke-linecap="round" '
        f'stroke-linejoin="round">{body}</g>'
    )


def tile_svg(label, color):
    return svg_shell(
        f'<rect x="3" y="3" width="18" height="18" rx="4" fill="{color}"/>\n'
        f'<text x="12" y="14.8" fill="#1b1f24" font-family="Segoe UI, Arial, sans-serif" '
        f'font-size="{7 if len(label) > 2 else 8}" font-weight="700" '
        f'text-anchor="middle">{label}</text>'
    )


def ui_svg(name):
    c = {
        "trash": "#ef5350", "close": "#ef5350", "cancel": "#ef5350", "stop": "#ef5350",
        "edit": "#ffb74d", "save": "#4ade80", "refresh": "#81c784", "branch": "#ce93d8",
        "key": "#ffd54f", "warning": "#ffb300", "database": "#fdd835", "terminal": "#90a4ae",
        "globe": "#29b6f6", "settings": BLUE, "advanced": BLUE, "folder": BLUE,
        "chat": "#4fc3f7", "image": "#64b5f6", "code": "#4dd0e1", "model": "#26c6da",
        "pin": "#f06292", "unpin": "#f06292", "theme": "#8ab4f8",
    }.get(name, BLUE)

    if name == "app":
        return svg_shell(
            '<path d="M5 7l5-3.5 9 2.5v12l-9 2.5L5 17l6-5z" fill="#007acc"/>\n'
            '<path d="M10.5 7.5L15.5 12l-5 4.5" stroke="#e8f3ff" stroke-width="1.6" '
            'stroke-linecap="round" stroke-linejoin="round"/>'
        )
    if name in {"file", "attach"}:
        return line_svg(c, '<path d="M6 3.5h7l5 5v12H6z"/><path d="M13 3.5v5h5"/>')
    if name == "folder":
        return svg_shell(
            '<path d="M3.5 7.5H9l1.8-2h9.7v13h-17z" fill="#42a5f52d" '
            'stroke="#42a5f5" stroke-width="1.8" stroke-linejoin="round"/>'
        )
    if name == "chat":
        return line_svg(c, '<rect x="4" y="5" width="16" height="12" rx="3"/>'
                           '<path d="M8 20l3-3M8 9.5h8M8 13h6"/>')
    if name in {"new-chat", "clear-chat"}:
        mark = '<path d="M12 8v6M9 11h6"/>' if name == "new-chat" else '<path d="M9 9l6 6M15 9l-6 6"/>'
        return line_svg(c, f'<rect x="4" y="5" width="16" height="12" rx="3"/><path d="M8 20l3-3"/>{mark}')
    if name == "trash":
        return line_svg(c, '<path d="M7 7h10M10 5h4M8 8h8v11H8zM10.5 10.5v6M13.5 10.5v6"/>')
    if name == "search":
        return line_svg("#9ca3af", '<circle cx="10.5" cy="10.5" r="5"/><path d="M14.5 14.5L19 19"/>', 1.9)
    if name == "chevron-up":
        return line_svg("#cbd5e1", '<path d="M7 14l5-5 5 5"/>', 2)
    if name == "chevron-down":
        return line_svg("#cbd5e1", '<path d="M7 10l5 5 5-5"/>', 2)
    if name in {"close", "cancel"}:
        return line_svg(c, '<path d="M8 8l8 8M16 8l-8 8"/>', 2)
    if name == "copy":
        return line_svg("#90caf9", '<rect x="8" y="7" width="10" height="12" rx="2"/><rect x="5" y="4" width="10" height="12" rx="2"/>')
    if name == "refresh":
        return line_svg(c, '<path d="M18.8 5.8l-.7 2.4M18.8 5.8l-2-.2"/><path d="M18.4 7A7 7 0 1 0 19 12"/>')
    if name == "branch":
        return line_svg(c, '<path d="M8 6v12M8 12l8-4"/><circle cx="8" cy="6" r="2"/><circle cx="8" cy="18" r="2"/><circle cx="16" cy="8" r="2"/>')
    if name == "edit":
        return line_svg(c, '<path d="M7 17l9.5-9.5M14 5l5 5M6 18l4-1"/>')
    if name == "save":
        return line_svg(c, '<rect x="5" y="4" width="14" height="16" rx="2"/><path d="M8 4v5M16 4v5"/><rect x="8" y="13" width="8" height="5" rx="1"/>')
    if name == "image":
        return line_svg(c, '<rect x="4" y="5" width="16" height="14" rx="2"/><circle cx="15.5" cy="9" r="1.5"/><path d="M6.5 16.5l3.5-4 3 2.5 3-3.5 3 5"/>')
    if name == "code":
        return line_svg(c, '<path d="M9 8l-4 4 4 4M15 8l4 4-4 4M13 7l-2 10"/>')
    if name == "key":
        return line_svg(c, '<circle cx="8" cy="10" r="3"/><path d="M11 10h9M16 10v3M19 10v2"/>')
    if name in {"settings", "advanced"}:
        extra = '<path d="M6 19.5h12" stroke="#8ab4f8"/>' if name == "advanced" else ""
        teeth = "".join(
            f'<path d="M{12+math.cos(i*math.pi/4)*6:.2f} {12+math.sin(i*math.pi/4)*6:.2f}'
            f'L{12+math.cos(i*math.pi/4)*8.5:.2f} {12+math.sin(i*math.pi/4)*8.5:.2f}"/>'
            for i in range(8)
        )
        return line_svg(c, f'<circle cx="12" cy="12" r="3"/>{teeth}{extra}')
    if name == "profiles":
        return line_svg(c, '<circle cx="10" cy="8" r="3.2"/><path d="M5 19a5 5 0 0 1 10 0"/><circle cx="16.5" cy="9.5" r="2.3"/><path d="M13 18a3.5 3.5 0 0 1 7 0"/>')
    if name == "theme":
        return svg_shell('<circle cx="12" cy="12" r="7" fill="#007acc2d" stroke="#007acc" stroke-width="1.8"/>'
                         '<path d="M14 5a7 7 0 0 0 0 14" stroke="#8ab4f8" stroke-width="1.5"/>')
    if name == "quit":
        return line_svg(c, '<path d="M12 4v8"/><path d="M7 7a7 7 0 1 0 10 0"/>')
    if name == "send":
        return svg_shell('<path d="M5 4l15 8-15 8 3-8z" fill="#007acc"/>')
    if name == "stop":
        return svg_shell('<rect x="7" y="7" width="10" height="10" rx="2" fill="#ef4444"/>')
    if name == "export":
        return line_svg(c, '<rect x="5" y="5" width="14" height="15" rx="2"/><path d="M12 15V3M8 7l4-4 4 4"/>')
    if name == "model":
        return line_svg(c, '<rect x="5" y="5" width="14" height="14" rx="3"/><path d="M9 9h6M9 12h6M9 15h4"/>')
    if name in {"pin", "unpin"}:
        slash = '<path d="M6 18L18 6"/>' if name == "unpin" else ""
        return line_svg(c, f'<path d="M12 13v8M8 5h8M10 5l-1 8h6l-1-8"/>{slash}')
    if name == "warning":
        return line_svg(c, '<path d="M12 4l9 15H3zM12 9v5M12 16.5h.01"/>')
    if name == "info":
        return line_svg(c, '<circle cx="12" cy="12" r="8"/><path d="M12 11v5M12 8h.01"/>')
    if name == "database":
        return line_svg(c, '<ellipse cx="12" cy="6.5" rx="6.5" ry="2.5"/><path d="M5.5 6.5v10.5M18.5 6.5v10.5"/><ellipse cx="12" cy="17" rx="6.5" ry="2.5"/><path d="M5.5 12a6.5 2.5 0 0 0 13 0"/>')
    if name == "terminal":
        return line_svg(c, '<rect x="4" y="5" width="16" height="14" rx="2"/><path d="M7 9l3 3-3 3M12 15h5"/>')
    if name == "globe":
        return line_svg(c, '<circle cx="12" cy="12" r="8"/><path d="M4 12h16"/><path d="M12 4a10 10 0 0 0 0 16M12 4a10 10 0 0 1 0 16"/>')
    if name == "history":
        return line_svg("#ab47bc", '<path d="M7 7a7 7 0 1 1-1.7 7"/><path d="M4 7h3V4M12 8v5l3 2"/>')
    if name in {"download", "upload"}:
        arrow = '<path d="M12 4v11M8 11l4 4 4-4"/>' if name == "download" else '<path d="M12 15V4M8 8l4-4 4 4"/>'
        return line_svg("#4fc3f7", f'{arrow}<path d="M5 19h14"/>')
    if name in {"play", "pause"}:
        return svg_shell('<path d="M8 5v14l11-7z" fill="#66bb6a"/>' if name == "play" else '<path d="M7 5h4v14H7zM13 5h4v14h-4z" fill="#ffca28"/>')
    return line_svg(c, '<circle cx="12" cy="12" r="8"/>')


def draw_tile(draw, label, color, scale):
    draw.rounded_rectangle([3*scale, 3*scale, 21*scale, 21*scale], radius=4*scale, fill=color)
    f = font(max(6, int((7 if len(label) > 2 else 8) * scale)), True)
    box = draw.textbbox((0, 0), label, font=f)
    draw.text(((24*scale - (box[2]-box[0]))/2, (24*scale - (box[3]-box[1]))/2 - 1*scale), label, fill="#1b1f24", font=f)


def draw_from_svgish(name, size=16):
    scale = size / 24
    img = Image.new("RGBA", (size, size), (0, 0, 0, 0))
    draw = ImageDraw.Draw(img)
    if name in TILE_ICONS:
        label, color = TILE_ICONS[name]
        draw_tile(draw, label, color, scale)
        return img
    color = {
        "trash": "#ef5350", "close": "#ef5350", "cancel": "#ef5350", "stop": "#ef5350",
        "edit": "#ffb74d", "save": "#4ade80", "refresh": "#81c784", "branch": "#ce93d8",
        "key": "#ffd54f", "warning": "#ffb300", "database": "#fdd835", "terminal": "#90a4ae",
        "globe": "#29b6f6", "settings": BLUE, "advanced": BLUE, "folder": BLUE,
        "chat": "#4fc3f7", "image": "#64b5f6", "code": "#4dd0e1", "model": "#26c6da",
        "pin": "#f06292", "unpin": "#f06292", "theme": "#8ab4f8",
    }.get(name, BLUE)
    w = max(1, int(2 * scale))
    def p(points): return [(x*scale, y*scale) for x, y in points]
    def line(points, fill=color): draw.line(p(points), fill=fill, width=w, joint="curve")
    def rect(x1, y1, x2, y2, r=2, outline=color, fill=None): draw.rounded_rectangle([x1*scale, y1*scale, x2*scale, y2*scale], radius=r*scale, outline=outline, width=w, fill=fill)
    def ellipse(x1, y1, x2, y2, outline=color, fill=None): draw.ellipse([x1*scale, y1*scale, x2*scale, y2*scale], outline=outline, width=w, fill=fill)

    if name == "app":
        draw.polygon(p([(5, 7), (10, 3.5), (19, 6), (19, 18), (10, 20.5), (5, 17), (11, 12)]), fill="#007acc")
        line([(10.5, 7.5), (15.5, 12), (10.5, 16.5)], "#e8f3ff")
    elif name == "folder":
        draw.polygon(p([(3.5, 7.5), (9, 7.5), (10.8, 5.5), (20.5, 5.5), (20.5, 18.5), (3.5, 18.5)]), outline=color, fill=(66, 165, 245, 45))
    elif name in {"file", "attach"}:
        line([(6, 3.5), (13, 3.5), (18, 8.5), (18, 20.5), (6, 20.5), (6, 3.5)])
        line([(13, 3.5), (13, 8.5), (18, 8.5)])
    elif name in {"chat", "new-chat", "clear-chat"}:
        rect(4, 5, 20, 17, 3)
        line([(8, 20), (11, 17)])
        if name == "new-chat":
            line([(12, 8), (12, 14)])
            line([(9, 11), (15, 11)])
        elif name == "clear-chat":
            line([(9, 9), (15, 15)], "#ef5350")
            line([(15, 9), (9, 15)], "#ef5350")
        else:
            line([(8, 9.5), (16, 9.5)])
            line([(8, 13), (14, 13)])
    elif name == "trash":
        line([(7, 7), (17, 7)])
        line([(10, 5), (14, 5)])
        rect(8, 8, 16, 19, 1.5)
        line([(10.5, 10.5), (10.5, 16.5)])
        line([(13.5, 10.5), (13.5, 16.5)])
    elif name == "search":
        ellipse(5.5, 5.5, 15.5, 15.5, "#9ca3af")
        line([(14.5, 14.5), (19, 19)], "#9ca3af")
    elif name == "chevron-up":
        line([(7, 14), (12, 9), (17, 14)], "#cbd5e1")
    elif name == "chevron-down":
        line([(7, 10), (12, 15), (17, 10)], "#cbd5e1")
    elif name in {"close", "cancel"}:
        line([(8, 8), (16, 16)], "#ef5350")
        line([(16, 8), (8, 16)], "#ef5350")
    elif name == "send":
        draw.polygon(p([(5, 4), (20, 12), (5, 20), (8, 12)]), fill="#007acc")
    elif name == "stop":
        rect(7, 7, 17, 17, 2, outline="#ef5350", fill="#ef5350")
    elif name in {"copy", "save", "model", "terminal", "image"}:
        label = {"copy": "CP", "save": "SV", "model": "AI", "terminal": ">_", "image": "IMG"}[name]
        draw_tile(draw, label, color, scale)
    elif name in {"settings", "advanced"}:
        ellipse(9, 9, 15, 15)
        for i in range(8):
            a = i * math.pi / 4
            line([(12 + math.cos(a)*6, 12 + math.sin(a)*6), (12 + math.cos(a)*8.5, 12 + math.sin(a)*8.5)])
        if name == "advanced":
            line([(6, 19.5), (18, 19.5)], "#8ab4f8")
    elif name in {"pin", "unpin"}:
        line([(12, 13), (12, 21)])
        line([(8, 5), (16, 5)])
        line([(10, 5), (9, 13), (15, 13), (14, 5)])
        if name == "unpin":
            line([(6, 18), (18, 6)], "#ef5350")
    elif name in {"play", "pause"}:
        if name == "play":
            draw.polygon(p([(8, 5), (8, 19), (19, 12)]), fill="#66bb6a")
        else:
            rect(7, 5, 11, 19, 0, outline="#ffca28", fill="#ffca28")
            rect(13, 5, 17, 19, 0, outline="#ffca28", fill="#ffca28")
    else:
        text = {
            "refresh": "R", "branch": "BR", "edit": "ED", "code": "<>", "key": "KEY",
            "warning": "!", "info": "i", "database": "DB", "globe": "GL", "profiles": "PR",
            "theme": "TH", "quit": "Q", "export": "EX", "history": "H", "download": "DL",
            "upload": "UP",
        }.get(name, name[:2].upper())
        draw_tile(draw, text, color, scale)
    return img


def main():
    SVG_DIR.mkdir(parents=True, exist_ok=True)
    PNG_DIR.mkdir(parents=True, exist_ok=True)

    icons = {name: ui_svg(name) for name in UI_ICONS}
    icons.update({name: tile_svg(label, color) for name, (label, color) in TILE_ICONS.items()})

    for name, svg in sorted(icons.items()):
        (SVG_DIR / f"{name}.svg").write_text(svg, encoding="utf-8", newline="\n")
        draw_from_svgish(name, 16).save(PNG_DIR / f"{name}.png")

    names = sorted(icons)
    cell_w, cell_h, cols = 150, 34, 4
    rows = math.ceil(len(names) / cols)
    preview = Image.new("RGBA", (cols * cell_w, rows * cell_h), "#202020")
    canvas = ImageDraw.Draw(preview)
    label_font = font(11)
    for idx, name in enumerate(names):
        col = idx % cols
        row = idx // cols
        x = col * cell_w
        y = row * cell_h
        if row % 2 == 0:
            canvas.rectangle([x, y, x + cell_w, y + cell_h], fill="#242424")
        icon = draw_from_svgish(name, 16)
        preview.alpha_composite(icon, (x + 12, y + 9))
        canvas.text((x + 36, y + 9), name, fill="#d4d4d4", font=label_font)
    PREVIEW_PATH.parent.mkdir(parents=True, exist_ok=True)
    preview.save(PREVIEW_PATH)

    print(f"Generated {len(icons)} SVG icons in {SVG_DIR}")
    print(f"Generated {len(icons)} 16px PNG icons in {PNG_DIR}")
    print(f"Generated preview sheet at {PREVIEW_PATH}")


if __name__ == "__main__":
    main()
