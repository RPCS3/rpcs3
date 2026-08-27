package net.rpcs3.framegen

import net.rpcs3.R
import kotlin.math.abs

enum class FrameGenPreset(
    val flowScale: Int,
    val labelRes: Int,
    val shortLabelRes: Int,
    val descriptionRes: Int
) {
    UltraPerformance(
        flowScale = 40,
        labelRes = R.string.framegen_preset_ultra_performance,
        shortLabelRes = R.string.framegen_preset_ultra_performance_short,
        descriptionRes = R.string.framegen_preset_ultra_performance_note
    ),
    Performance(
        flowScale = 50,
        labelRes = R.string.framegen_preset_performance,
        shortLabelRes = R.string.framegen_preset_performance_short,
        descriptionRes = R.string.framegen_preset_performance_note
    ),
    Balanced(
        flowScale = 70,
        labelRes = R.string.framegen_preset_balanced,
        shortLabelRes = R.string.framegen_preset_balanced_short,
        descriptionRes = R.string.framegen_preset_balanced_note
    ),
    Quality(
        flowScale = 100,
        labelRes = R.string.framegen_preset_quality,
        shortLabelRes = R.string.framegen_preset_quality_short,
        descriptionRes = R.string.framegen_preset_quality_note
    );

    companion object {
        val Default = Balanced

        fun fromFlowScale(flowScale: Int): FrameGenPreset =
            entries.minByOrNull { abs(it.flowScale - flowScale) } ?: Default

        fun atIndex(index: Int): FrameGenPreset =
            entries[index.coerceIn(0, entries.size - 1)]
    }
}
