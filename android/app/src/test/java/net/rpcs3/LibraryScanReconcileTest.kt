package net.rpcs3

import org.junit.Assert.assertEquals
import org.junit.Assert.assertFalse
import org.junit.Assert.assertTrue
import org.junit.Test

private const val HDD0 = "/data/rpcs3/config/dev_hdd0/game/"
private const val GAMES = "/data/rpcs3/config/games/"

private val ROOTS = listOf(HDD0, GAMES)

private fun stale(
    path: String,
    seen: Set<String> = emptySet(),
    present: Set<String> = emptySet()
) = isStaleAfterScan(path, seen, ROOTS) { present.contains(it) }

class LibraryScanReconcileTest {
    @Test
    fun isoOutsideScanRootsSurvivesRefresh() {
        val iso = "/storage/emulated/0/Download/game.iso"

        assertFalse(stale(iso, present = setOf(iso)))
    }

    @Test
    fun isoOnUnmountedStorageSurvivesRefresh() {
        val iso = "/storage/1A2B-3C4D/PS3/game.iso"

        assertFalse(stale(iso))
    }

    @Test
    fun directBootFolderOutsideScanRootsSurvivesRefresh() {
        val folder = "/storage/emulated/0/PS3/BLUS31584"

        assertFalse(stale(folder))
    }

    @Test
    fun installedGameStillOnDiskSurvivesUnreadableParamSfo() {
        val game = HDD0 + "BLUS30127"

        assertFalse(stale(game, present = setOf(game)))
    }

    @Test
    fun installedGameReportedByScanSurvives() {
        val game = HDD0 + "BLUS30127"

        assertFalse(stale(game, seen = setOf(game)))
    }

    @Test
    fun installedGameDeletedFromDiskIsPruned() {
        val game = GAMES + "BLES00229"

        assertTrue(stale(game))
    }

    @Test
    fun installPlaceholderIsNeverPruned() {
        assertFalse(stale("$"))
    }

    @Test
    fun scanRootPrefixesCanonicalizeAndTerminate() {
        val prefixes = scanRootPrefixes(
            listOf("/data/rpcs3//config/dev_hdd0/game", "/data/rpcs3/config/games/")
        ) { it.replace("//", "/").trimEnd('/') }

        assertEquals(listOf(HDD0, GAMES), prefixes)
    }

    @Test
    fun unresolvableScanRootIsDropped() {
        val prefixes = scanRootPrefixes(listOf(HDD0, "/broken")) { root ->
            if (root == "/broken") null else root.trimEnd('/')
        }

        assertEquals(listOf(HDD0), prefixes)
    }

    @Test
    fun prefixMatchDoesNotStrayIntoSiblingDirectories() {
        assertFalse(stale("/data/rpcs3/config/games-backup/BLES00229"))
    }
}
