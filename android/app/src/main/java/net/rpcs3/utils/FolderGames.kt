package net.rpcs3.utils

import android.system.Os
import android.system.OsConstants
import net.rpcs3.GameInfo
import net.rpcs3.GameRepository
import net.rpcs3.RPCS3
import java.io.File

object FolderGames {
    private const val LINK_DIR = "directboot"
    private val linkName = Regex("^[A-Za-z0-9_-]{4,32}$")

    private fun linkRoot(): File? {
        val base = RPCS3.internalDirectory
        return if (base.isEmpty()) null else File(base, LINK_DIR)
    }

    private fun isLink(file: File) = runCatching {
        OsConstants.S_ISLNK(Os.lstat(file.absolutePath).st_mode)
    }.getOrDefault(false)

    private fun normalize(path: String) = path.trimEnd('/')

    private fun entries(): List<Pair<File, String>> {
        val children = linkRoot()?.listFiles() ?: return emptyList()

        return children.filter { isLink(it) }.mapNotNull { link ->
            val target = runCatching { Os.readlink(link.absolutePath) }.getOrNull()
            if (target == null) null else link to normalize(target)
        }
    }

    private fun candidatePaths(link: String): List<String> {
        val candidates = mutableListOf(link)

        fun reroot(prefix: String, dropFirstSegment: Boolean) {
            if (!link.startsWith(prefix)) {
                return
            }

            var rest = link.substring(prefix.length)

            if (dropFirstSegment) {
                val slash = rest.indexOf('/')
                if (slash < 0) {
                    return
                }
                rest = rest.substring(slash + 1)
            }

            candidates += "/storage/$rest"
        }

        reroot("/mnt/user/", true)
        reroot("/mnt/runtime/", true)
        reroot("/mnt/androidwritable/", true)
        reroot("/mnt/media_rw/", false)

        return candidates
    }

    fun resolveDescriptorPath(fd: Int): String? {
        val link = runCatching { Os.readlink("/proc/self/fd/$fd") }.getOrNull() ?: return null
        return candidatePaths(link).firstOrNull { File(it).isFile }
    }

    fun gameRootOf(paramSfoPath: String): String {
        val parent = File(paramSfoPath).parentFile ?: return normalize(paramSfoPath)

        if (parent.name == "PS3_GAME") {
            return normalize(parent.parent ?: parent.absolutePath)
        }

        return normalize(parent.absolutePath)
    }

    fun iconPathOf(gameRoot: String): String? {
        listOf("PS3_GAME/ICON0.PNG", "ICON0.PNG").forEach { relative ->
            val candidate = File(gameRoot, relative)
            if (candidate.isFile) {
                return candidate.absolutePath
            }
        }

        return null
    }

    fun linksTo(gamePath: String): List<File> {
        val wanted = normalize(gamePath)
        return entries().filter { it.second == wanted }.map { it.first }
    }

    fun isDirectBoot(gamePath: String) = linksTo(gamePath).isNotEmpty()

    fun bootPath(gamePath: String) = linksTo(gamePath).firstOrNull()?.absolutePath ?: gamePath

    fun link(gameRoot: String, titleId: String): File? {
        if (!linkName.matches(titleId)) {
            return null
        }

        val root = linkRoot() ?: return null

        if (!root.isDirectory && !root.mkdirs()) {
            return null
        }

        val link = File(root, titleId)

        if (isLink(link)) {
            if (runCatching { Os.remove(link.absolutePath) }.isFailure) {
                return null
            }
        } else if (link.exists() && !link.delete()) {
            return null
        }

        return runCatching {
            Os.symlink(normalize(gameRoot), link.absolutePath)
            link
        }.getOrNull()
    }

    fun removeLinks(gamePath: String): Int {
        var removed = 0

        linksTo(gamePath).forEach { link ->
            if (runCatching { Os.remove(link.absolutePath) }.isSuccess) {
                removed++
            }
        }

        return removed
    }

    fun restore() {
        entries().forEach { (link, target) ->
            if (!File(target).isDirectory || GameRepository.find(target) != null) {
                return@forEach
            }

            val details = GameDetailsReader.read(link.absolutePath)

            GameRepository.add(
                arrayOf(
                    GameInfo(
                        target,
                        details.title.ifEmpty { details.titleId },
                        iconPathOf(target),
                        0
                    )
                ),
                -1L
            )
        }
    }
}
