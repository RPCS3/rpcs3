package net.rpcs3

import android.content.Context
import android.os.Bundle
import android.os.Handler
import android.os.Looper
import android.os.Message
import androidx.annotation.Keep
import androidx.compose.runtime.MutableLongState
import androidx.compose.runtime.MutableState
import androidx.compose.runtime.mutableLongStateOf
import androidx.compose.runtime.mutableStateOf
import androidx.core.app.NotificationCompat
import androidx.core.app.NotificationManagerCompat
import net.rpcs3.dialogs.AlertDialogQueue
import java.util.concurrent.ConcurrentHashMap
import java.util.concurrent.CopyOnWriteArrayList

data class ProgressEntry(
    val value: MutableLongState = mutableLongStateOf(0),
    val max: MutableLongState = mutableLongStateOf(0),
    val message: MutableState<String?> = mutableStateOf(null)
) {
    fun isComplete() = value.longValue == max.longValue && !isIndeterminate()
    fun isFailed() = value.longValue < 0
    fun isFinished() = isFailed() || isComplete()
    fun isIndeterminate() = max.longValue == 0L
}

data class ProgressUpdateEntry(val value: Long, val max: Long, val message: String?) {
    fun isComplete() = value == max && !isIndeterminate()
    fun isFailed() = value < 0
    fun isFinished() = isFailed() || isComplete()
    fun isIndeterminate() = max == 0L
}

private data class ProgressWithHandler(
    var handler: (ProgressUpdateEntry) -> Unit,
    val progressEntry: MutableState<ProgressEntry>,
    val listeners: CopyOnWriteArrayList<(ProgressUpdateEntry) -> Unit> = CopyOnWriteArrayList()
)

class ProgressRepository {
    private var progressHandlers = ConcurrentHashMap<Long, ProgressWithHandler>()
    private var nextRequestId = 1L

    companion object {
        private val instance = ProgressRepository()

        fun getItem(id: Long?) =
            if (id != null) instance.progressHandlers[id]?.progressEntry else null

        fun exists(id: Long?) = id != null && instance.progressHandlers.containsKey(id)

        fun addListener(id: Long, listener: (ProgressUpdateEntry) -> Unit) {
            instance.progressHandlers[id]?.listeners?.add(listener)
        }

        @Keep
        @JvmStatic
        fun onProgressEvent(id: Long, value: Long, max: Long, message: String? = null): Boolean {
            val item = instance.progressHandlers[id] ?: return false

            val previous = item.progressEntry.value
            item.progressEntry.value = ProgressEntry(
                mutableLongStateOf(value),
                mutableLongStateOf(max),
                mutableStateOf(message ?: previous.message.value)
            )

            item.handler(ProgressUpdateEntry(value, max, item.progressEntry.value.message.value))

            if (item.progressEntry.value.isFinished()) {
                cancel(id)
            }

            return true
        }

        fun cancel(id: Long) {
            instance.progressHandlers.remove(id)
            GameRepository.clearProgress(id)
        }

        fun create(
            context: Context,
            title: String,
            silent: Boolean = false,
            handler: (ProgressUpdateEntry) -> Unit = { _ -> }
        ): Long {
            var requestId: Long
            val entry = ProgressWithHandler(handler, mutableStateOf(ProgressEntry()))
            while (true) {
                requestId = instance.nextRequestId++
                if (instance.progressHandlers.put(requestId, entry) == null) {
                    break
                }
            }

            val hasPermission = !silent && Permission.PostNotifications.checkPermission(context)

            val builder = NotificationCompat.Builder(context, "rpcs3-progress").apply {
                setContentTitle(title)
                setSmallIcon(R.drawable.ic_stat_ps3native)
                setCategory(NotificationCompat.CATEGORY_SERVICE)
                setPriority(NotificationCompat.PRIORITY_DEFAULT)
                setProgress(0, 0, true)
                setSilent(true)
            }

            if (hasPermission) {
                with(NotificationManagerCompat.from(context)) {
                    notify(requestId.toInt(), builder.build())
                }
            }

            val asyncHandler = Handler.createAsync(Looper.getMainLooper()) { message ->
                val value = message.data.getLong("value")
                val max = message.data.getLong("max")
                val text = message.data.getString("message")

                if (hasPermission) {
                    val notificationManager = NotificationManagerCompat.from(context)

                    if (text != null) {
                        builder.setContentText(text)
                    }

                    if (value >= 0 && max > 0) {
                        if (value == max) {
                            notificationManager.cancel(requestId.toInt())
                        } else {
                            builder.setProgress(max.toInt(), value.toInt(), false)
                            notificationManager.notify(requestId.toInt(), builder.build())
                        }
                    } else if (value < 0) {
                        val contentText = text ?: "Installation failed"
                        builder.setContentText(contentText)
                        AlertDialogQueue.showDialog(title, contentText)
                        notificationManager.notify(requestId.toInt(), builder.build())
                    } else {
                        builder.setProgress(max.toInt(), value.toInt(), true)
                        notificationManager.notify(requestId.toInt(), builder.build())
                    }
                }

                val update = ProgressUpdateEntry(value, max, text)
                handler(update)
                entry.listeners.forEach { it(update) }
                true
            }

            entry.handler = { progress: ProgressUpdateEntry ->
                val message = Message()
                val data = Bundle()
                data.putLong("value", progress.value)
                data.putLong("max", progress.max)
                data.putString("message", progress.message)
                message.data = data
                asyncHandler.sendMessage(message)
            }

            return requestId
        }
    }
}
