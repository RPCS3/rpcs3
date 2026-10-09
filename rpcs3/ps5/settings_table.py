#!/usr/bin/env python3
"""PS5: the launcher's settings, as the desktop's settings dialog has them.

Reads rpcs3qt/settings_dialog.cpp (which setting each widget edits, and its
tooltip), rpcs3qt/settings_dialog.ui (the widget's tab and label) and
rpcs3qt/tooltips.h (the tooltips' text), and writes ps5_settings_table.inc:
one row per setting, in the dialog's tab order, that ps5_launcher.cpp turns
into its Settings tab and each game's settings. Run it after merging
upstream, and commit what it writes.

    python3 rpcs3/ps5/settings_table.py
"""

import re
import xml.etree.ElementTree as ET
from pathlib import Path

HERE = Path(__file__).resolve().parent
QT = HERE.parent / "rpcs3qt"
TARGET = HERE / "ps5_settings_table.inc"

# The dialog's tabs that hold emulator settings, and the names they get here
TABS = {"coreTab": "CPU", "gpuTab": "GPU", "audioTab": "Audio", "inputTab": "I/O", "systemTab": "System",
        "networkTab": "Network", "advancedTab": "Advanced", "emulatorTab": "Emulator", "debugTab": "Debug"}

# Not offered on the console: what has one choice there or none (renderers,
# audio and input devices, windows), what would end or stall the title
# (exiting on stop, not starting on boot, no video output), and what the
# launcher depends on (the native interface, Big Picture Mode at boot)
EXCLUDED = {
    "Renderer", "AudioRenderer", "MicrophoneType", "KeyboardHandler", "MouseHandler", "CameraType", "Camera",
    "CameraFlip", "MusicHandler", "Move", "Buzz", "Turntable", "GHLtar", "BackgroundInput", "ShowMoveCursor",
    "MouseBasedGyro", "LockOvlIptToP1", "ExclusiveFullscreenMode", "PauseOnFocusLoss", "StartGameFullscreen",
    "PreventDisplaySleep", "UseNativeInterface", "EnableGamemode", "ExitRPCS3OnFinish", "StartBigPictureModeOnBoot",
    "StartOnBoot", "DisableVideoOutput", "RenderdocCompatibility", "DebugOutput", "DisableMSLFastMath",
    "EnableHostRoot", "DumpToFile", "RecordWithOverlays", "ShowCaptureHints", "UseRecursiveScan",
    "ShowMouseAndKeyboardToggleHint", "MouseDebugOverlay", "ShowRpcnPopups", "PerfOverlayUseWindowSpace",
}

# Settings the dialog edits through its own code rather than one widget:
# (after this one, tab, setting, label, tooltip)
EXTRA = [
    ("ShaderPrecisionQuality", "GPU", "RelaxedZCULL", "Relaxed ZCULL sync", "zcull_operation_mode"),
    ("RelaxedZCULL", "GPU", "PreciseZCULL", "Precise ZCULL stats", "zcull_operation_mode"),
]

# The PS5 port's own settings, which the desktop dialog lacks:
# (after this one, tab, setting, label, help)
PS5_EXTRA = [
    ("PreferredSPUThreads", "CPU", "SPUOwnCore", "Own core for the busy SPU",
     "Runs the kernel of a SPURS instance of one SPU alone on one of the console's CPU cores, and keeps "
     "every other emulator thread off that core. In GTA IV it is the busiest thread of all in busy streets "
     "(its sound and more), and the game waits for it. Off, the console's system places every thread, and "
     "two busy ones can share a core. Takes effect when the game starts."),
    ("OutputScalingMode", "GPU", "FrameGeneration", "Frame generation",
     "Shows frames made between the game's own when the game runs slower than the display's 60 frames a "
     "second: motion looks smoother. The game itself runs no faster, each of its frames reaches the "
     "screen a refresh (17 ms) later for each frame made before it, and fast motion, text and the HUD "
     "can shimmer or smear."),
    ("FrameGeneration", "GPU", "FrameGenerationFrames", "Frames made between",
     "The most frames made between two of the game's. They go up one a refresh, only as many as the time "
     "between the game's frames has room for beside what the screen still has waiting, and a frame that "
     "would have to wait for the screen is dropped, so the game is not held back. At 24 fps, 1 shows about "
     "48 frames a second and Auto (up to 3) nearly 60. Each frame made puts the game's own a refresh (17 ms) "
     "later."),
    ("FrameGenerationFrames", "GPU", "FrameGenerationQuality", "Frame generation quality",
     "How hard frame generation looks for motion: Fast follows it in a copy of the picture 480 pixels "
     "across, Balanced 640, Quality 960. Each follows moves of up to about a fifth of the screen between "
     "two of the game's frames; higher follows the edges of moving things more closely and takes more of "
     "the GPU's time."),
    ("TimeStretchingThreshold", "Audio", "DisableSamplingSkip", "Disable sampling skip",
     "When a game is late with its sound, the emulator waits for it a little (about 11 ms) before skipping "
     "that bit of the sound. On, it never waits: the sound keeps strict time, and every late moment is "
     "skipped, heard as stutter. GTA IV on the console skipped 8 to 17 bits every 10 s with it on, 0 to 3 "
     "with it off. Some games that slow down waiting for their sound run better with it on."),
    ("DisableSamplingSkip", "Audio", "LateAudioWait", "Wait for late game sound",
     "How much longer than the emulator's own wait (about 11 ms) to wait for a game that is late with its "
     "sound, before skipping that bit of it (heard as a click or stutter). The sound already buffered "
     "plays on meanwhile, so a wait shorter than the audio buffer should not be heard. Off is RPCS3's own "
     "behaviour. Has no effect while Disable sampling skip is on."),
    ("LateAudioWait", "Audio", "MenuSoundVolume", "Menu sounds",
     "How loud the menus' sounds are: moving, choosing, going back, messages and trophies, in the library, "
     "the pause menu and RPCS3's own dialogs. Off for none. To use sounds of your own, put WAV files named "
     "snd_cursor, snd_decide, snd_cancel, snd_system_ok, snd_system_ng, snd_oskenter, snd_oskcancel and "
     "snd_trophy (.wav) in the title's rpcs3/sounds folder over FTP; any you leave out keep the title's own."),
]

# Labels clearer alone than under the dialog's shared group boxes
LABELS = {
    "PerfOverlayDetailLevel": "Performance overlay detail level",
    "PerfOverlayPosition": "Performance overlay position",
    "PerfOverlayCenterX": "Centre the performance overlay across",
    "PerfOverlayCenterY": "Centre the performance overlay down",
    "PerfOverlayMarginX": "Performance overlay margin across (%)",
    "PerfOverlayMarginY": "Performance overlay margin down (%)",
    "PerfOverlayUpdateInterval": "Performance overlay update interval",
    "PerfOverlayFontSize": "Performance overlay font size",
    "PerfOverlayOpacity": "Performance overlay opacity",
    "PerfOverlayFramerateDatapoints": "Framerate graph datapoints",
    "PerfOverlayFrametimeDatapoints": "Frametime graph datapoints",
    "FsrSharpeningStrength": "FidelityFX CAS sharpening strength",
    "MasterVolume": "Master volume",
    "ResolutionScale": "Resolution Scale",
    "MaximumCacheSize": "Maximum cache size (MB)",
    "ShaderLoadBgEnabled": "Shader loading: allow a custom background",
    "ShaderLoadBgDarkening": "Shader loading: background darkening",
    "ShaderLoadBgBlur": "Shader loading: background blur",
}

# Tooltips the dialog chooses in code
TOOLTIPS = {"ThreadSchedulerMode": "enable_thread_scheduler"}

# Radio button groups: their label and tooltip
RADIO = {
    "PPUDecoder": ("PPU decoder", "ppu_llvm"),
    "SPUDecoder": ("SPU decoder", "spu_llvm"),
    "ShaderMode": ("Shader mode", "async_shader_recompiler"),
    "EnterButtonAssignment": ("Enter button assignment", "enter_button_assignment"),
}


def read_tooltips():
    text = (QT / "tooltips.h").read_text(encoding="utf-8")
    block = text[text.index("const struct settings"):]
    block = block[:block.index("} settings;")]
    tips = {}
    for name, body in re.findall(r'const QString\s+(\w+)\s*=\s*tr\("((?:[^"\\]|\\.)*)"\)', block):
        body = body.encode("utf-8").decode("unicode_escape").encode("latin-1").decode("utf-8")
        body = re.sub(r"<br\s*/?>", "\n", body)
        body = re.sub(r"<[^>]+>", "", body)
        tips[name] = body.strip()
    return tips


def read_ui():
    root = ET.parse(QT / "settings_dialog.ui").getroot()
    parent = {child: node for node in root.iter() for child in node}
    widgets = {w.get("name"): w for w in root.iter("widget")}

    def prop(widget, name):
        for p in widget.findall("property"):
            if p.get("name") == name and p.find("string") is not None:
                return (p.find("string").text or "").replace("&", "").strip()
        return ""

    def tab_of(name):
        node = widgets.get(name)
        while node is not None:
            if node.tag == "widget" and node.get("name") in TABS:
                return TABS[node.get("name")]
            node = parent.get(node)
        return None

    def group_title(name):
        node = parent.get(widgets[name]) if name in widgets else None
        while node is not None:
            if node.tag == "widget" and node.get("class") == "QGroupBox" and prop(node, "title"):
                return prop(node, "title")
            node = parent.get(node)
        return ""

    def ancestors(name):
        names, node = [], widgets.get(name)
        while node is not None:
            if node.tag == "widget" and node.get("name"):
                names.append(node.get("name"))
            node = parent.get(node)
        return names

    return widgets, prop, tab_of, group_title, ancestors


def main():
    tips = read_tooltips()
    widgets, prop, tab_of, group_title, ancestors = read_ui()
    source = (QT / "settings_dialog.cpp").read_text(encoding="utf-8")

    subscribed = dict(re.findall(r"SubscribeTooltip\(ui->(\w+),\s*tooltips\.settings\.(\w+)\)", source))

    rows = []  # (position, tab, setting, label, tooltip)

    def add(position, setting, widget, tooltip, group=None, label=None):
        if setting in EXCLUDED:
            return
        tab = tab_of(widget) if widget else None
        if tab is None and group:
            tab = tab_of(group)
        if tab is None:
            return
        if label is None:
            label = prop(widgets[widget], "text") if widget in widgets else ""
            if not label and group in widgets:
                label = prop(widgets[group], "title")
            if not label and widget:
                label = group_title(widget)
        label = LABELS.get(setting, label)
        # The tooltip given, else one on the widget or the boxes around it
        tooltip = tooltip or TOOLTIPS.get(setting)
        for name in ([group] if group else []) + (ancestors(widget) if widget else []):
            tooltip = tooltip or subscribed.get(name)
        rows.append((position, tab, setting, label, tips.get(tooltip, "")))

    for m in re.finditer(r"(?<!->)EnhanceCheckBox\(emu_settings_type::(\w+),\s*ui->(\w+),\s*(?:tooltips\.settings\.(\w+)|\{\})", source):
        add(m.start(), m.group(1), m.group(2), m.group(3))
    for m in re.finditer(r"(?<!->)EnhanceComboBox\(emu_settings_type::(\w+),\s*ui->(\w+),\s*(?:tooltips\.settings\.(\w+)|\{\})(?:,\s*ui->(\w+))?", source):
        add(m.start(), m.group(1), m.group(2), m.group(3), m.group(4))
    for m in re.finditer(r'(?<!->)EnhanceSlider\(emu_settings_type::(\w+),\s*ui->(\w+),\s*ui->(\w+),\s*tr\((?:reinterpret_cast<const char\*>\()?u?8?"([^"]*)"', source):
        label = m.group(4).split(":")[0].strip() if ":" in m.group(4) else group_title(m.group(2))
        add(m.start(), m.group(1), m.group(2), None, label=label)
    for m in re.finditer(r"m_emu_settings->Enhance(CheckBox|ComboBox|Slider|SpinBox|DoubleSpinBox)\(ui->(\w+),\s*emu_settings_type::(\w+)", source):
        label = None if m.group(1) == "CheckBox" else group_title(m.group(2))
        add(m.start(), m.group(3), m.group(2), None, label=label)
    for m in re.finditer(r"m_emu_settings->EnhanceRadioButton\((\w+),\s*emu_settings_type::(\w+)\)", source):
        if m.group(2) in RADIO:
            label, tooltip = RADIO[m.group(2)]
            button = re.search(m.group(1) + r"->addButton\(ui->(\w+)", source).group(1)
            add(m.start(), m.group(2), button, tooltip, label=label)

    # Once each (the dialog has a few twice, one per platform), the extras after theirs
    seen, ordered = set(), []
    for row in sorted(rows):
        if row[2] not in seen:
            seen.add(row[2])
            ordered.append(row)
    for after, tab, setting, label, tooltip in EXTRA:
        at = next(i for i, row in enumerate(ordered) if row[2] == after)
        ordered.insert(at + 1, (ordered[at][0], tab, setting, label, tips.get(tooltip, "")))
    for after, tab, setting, label, text in PS5_EXTRA:
        at = next(i for i, row in enumerate(ordered) if row[2] == after)
        ordered.insert(at + 1, (ordered[at][0], tab, setting, label, text))

    tab_order = list(TABS.values())
    ordered = [row for tab in tab_order for row in ordered if row[1] == tab]

    def literal(text):
        return '"' + text.replace("\\", "\\\\").replace('"', '\\"').replace("\n", "\\n") + '"'

    lines = ["// Generated by settings_table.py from rpcs3qt/settings_dialog.cpp, settings_dialog.ui",
             "// and tooltips.h: the desktop dialog's settings, by tab. Do not edit; run it again"]
    for _, tab, setting, label, tooltip in ordered:
        lines.append(f"{{{literal(tab)}, emu_settings_type::{setting}, {literal(label)}, {literal(tooltip)}}},")
    TARGET.write_text("\n".join(lines) + "\n", encoding="utf-8")
    counts = {tab: sum(1 for row in ordered if row[1] == tab) for tab in tab_order}
    print(f"wrote {TARGET.name}: {len(ordered)} settings ({', '.join(f'{t} {n}' for t, n in counts.items())})")


if __name__ == "__main__":
    main()
