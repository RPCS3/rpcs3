#pragma once

#include <rtmidi_c.h>
#include <memory>

inline constexpr auto midi_deleter = [](RtMidiWrapper* ptr) { if (ptr) rtmidi_in_free(ptr); };
using midi_ptr = std::unique_ptr<RtMidiWrapper, decltype(midi_deleter)>;
