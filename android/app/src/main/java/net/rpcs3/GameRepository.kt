package net.rpcs3

import android.content.res.Resources.NotFoundException
import androidx.annotation.Keep
import androidx.compose.runtime.MutableIntState
import androidx.compose.runtime.MutableState
import androidx.compose.runtime.mutableIntStateOf
import androidx.compose.runtime.mutableStateListOf
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.snapshots.SnapshotStateList
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.withContext
import kotlinx.serialization.Serializable
import kotlinx.serialization.json.Json
import java.io.File
import java.security.InvalidParameterException

enum class GameFlag {
    Locked,
    Trial
}

@Serializable
data class GameInfo @Keep constructor(
    val path: String,
    var name: String? = null,
    var iconPath: String? = null,
    var gameFlags: Int = 0
)

data class GameInfoStore(
    val path: String,
    val name: MutableState<String?> = mutableStateOf(null),
    val iconPath: MutableState<String?> = mutableStateOf(null),
    val gameFlags: MutableIntState = mutableIntStateOf(0)
)

enum class GameProgressType {
    Install,
    Compile,
    Remove,
}

data class GameProgress(val id: Long, val type: GameProgressType)

data class Game(
    val info: GameInfoStore,
    val progressList: SnapshotStateList<GameProgress> = mutableStateListOf()
) {
    fun addProgress(progress: GameProgress) {
        if (findProgress(progress.type) != null) {
            throw InvalidParameterException()
        }

        progressList += progress
    }

    fun findProgress(type: GameProgressType) =
        progressList.filter { elem -> elem.type == type }.ifEmpty { null }

    fun findProgress(types: Array<GameProgressType>) =
        progressList.filter { elem -> types.contains(elem.type) }.ifEmpty { null }

    fun removeProgress(type: GameProgressType) =
        progressList.removeIf { progress -> progress.type == type }

    fun hasFlag(flag: GameFlag) = (info.gameFlags.intValue and (1 shl flag.ordinal)) != 0
}

internal fun scanRootPrefixes(roots: List<String>, canonical: (String) -> String?) =
    roots.mapNotNull { root -> canonical(root)?.trimEnd('/')?.plus("/") }

internal fun isStaleAfterScan(
    path: String,
    seen: Set<String>,
    ownedRoots: List<String>,
    exists: (String) -> Boolean
): Boolean {
    if (path == "$" || seen.contains(path)) {
        return false
    }

    if (ownedRoots.none { root -> path.startsWith(root) }) {
        return false
    }

    return !exists(path)
}

private fun toStore(info: GameInfo) =
    GameInfoStore(
        info.path,
        mutableStateOf(info.name),
        mutableStateOf(info.iconPath),
        mutableIntStateOf(info.gameFlags)
    )

private fun toInfo(store: GameInfoStore) =
    GameInfo(store.path, store.name.value, store.iconPath.value, store.gameFlags.intValue)

class GameRepository {
    private val games = mutableStateListOf<Game>()

    companion object {
        private val instance = GameRepository()
        private var scanSink: MutableSet<String>? = null

        private fun storeFile() = File(RPCS3.rootDirectory + "games.json")

        private fun backupFile() = File(RPCS3.rootDirectory + "games.json.bak")

        fun save() {
            try {
                val payload = Json.encodeToString(instance.games.map { game ->
                    toInfo(
                        game.info
                    )
                }.filter { info -> info.path != "$" })

                val target = storeFile()
                target.parentFile?.mkdirs()
                val staging = File(target.parentFile, "games.json.tmp")
                staging.writeText(payload)

                if (target.isFile) {
                    target.copyTo(backupFile(), overwrite = true)
                }

                if (!staging.renameTo(target)) {
                    staging.copyTo(target, overwrite = true)
                    staging.delete()
                }
            } catch (e: Exception) {
                e.printStackTrace()
            }
        }

        private fun readStore(file: File): Array<GameInfo>? = try {
            if (file.isFile) Json.decodeFromString<Array<GameInfo>>(file.readText()) else null
        } catch (_: NotFoundException) {
            null
        } catch (e: Exception) {
            e.printStackTrace()
            null
        }

        suspend fun load() {
            withContext(Dispatchers.IO) {
                val stored = readStore(storeFile()) ?: readStore(backupFile()) ?: return@withContext

                synchronized(instance) {
                    val unsaved = instance.games.filter { game ->
                        stored.none { info -> info.path == game.info.path }
                    }

                    instance.games.clear()
                    instance.games += stored.map { info -> Game(toStore(info)) }
                    instance.games += unsaved
                }
            }
        }

        fun beginScan() {
            synchronized(instance) {
                scanSink = mutableSetOf()
            }
        }

        fun endScan(roots: List<String>) {
            synchronized(instance) {
                val seen = scanSink ?: return
                scanSink = null

                val owned = scanRootPrefixes(roots) { root ->
                    runCatching { File(root).canonicalPath }.getOrNull()
                }

                instance.games.removeIf { game ->
                    isStaleAfterScan(game.info.path, seen, owned) { path -> File(path).exists() }
                }

                save()
            }
        }

        @Keep
        @JvmStatic
        fun add(gameInfos: Array<GameInfo>, progressId: Long) {
            synchronized(instance) {
                if (progressId >= 0) {
                    val progressEntry =
                        instance.games.filter { game -> game.info.path == "$" }.find { game ->
                            val progress = game.findProgress(GameProgressType.Install)
                                ?.find { progress -> progress.id == progressId }
                            progress != null
                        }

                    if (progressEntry != null) {
                        instance.games.remove(progressEntry)
                    }
                }

                gameInfos.forEach { info ->
                    scanSink?.add(info.path)
                    val existsGame = instance.games.find { x -> x.info.path == info.path }
                    if (existsGame == null) {
                        val newGame = Game(toStore(info))
                        if (progressId >= 0) {
                            newGame.addProgress(GameProgress(progressId, GameProgressType.Install))
                        }
                        instance.games.add(0, newGame)
                    } else {
                        existsGame.info.name.value = info.name ?: existsGame.info.name.value
                        existsGame.info.iconPath.value =
                            info.iconPath ?: existsGame.info.iconPath.value
                        existsGame.info.gameFlags.intValue = info.gameFlags
                        if (progressId >= 0) {
                            existsGame.addProgress(
                                GameProgress(
                                    progressId,
                                    GameProgressType.Install
                                )
                            )
                        }
                    }
                }

                if (scanSink == null) {
                    save()
                }
            }
        }

        fun addPreview(gameInfos: Array<GameInfo>) {
            synchronized(instance) {
                instance.games += gameInfos.map { info -> Game(toStore(info)) }
            }
        }

        fun onBoot(game: Game) {
            synchronized(instance) {
                if (instance.games.firstOrNull() != game) {
                    instance.games.remove(game)
                    instance.games.add(0, game)
                    save()
                }
            }
        }

        fun createGameInstallEntry(progressId: Long) {
            synchronized(instance) {
                val game = Game(GameInfoStore("$"))
                game.addProgress(GameProgress(progressId, GameProgressType.Install))
                instance.games.add(0, game)
            }
        }

        fun clearProgress(progressId: Long) {
            synchronized(instance) {
                instance.games.forEach { game -> game.progressList.removeIf { progress -> progress.id == progressId } }
                instance.games.removeIf { game -> game.info.path == "$" && game.progressList.isEmpty() }
            }
        }

        fun remove(game: Game) {
            synchronized(instance) {
                instance.games -= game
                save()
            }
        }

        fun find(path: String): Game? {
            synchronized(instance) {
                return instance.games.find { game -> game.info.path == path }
            }
        }

        fun list() = instance.games
    }
}

fun gameTitleId(path: String) = path.trimEnd('/').substringAfterLast('/')
