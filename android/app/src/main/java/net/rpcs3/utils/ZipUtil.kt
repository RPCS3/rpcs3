
package net.rpcs3.utils

import java.io.*
import java.util.zip.ZipEntry
import java.util.zip.ZipFile
import java.util.zip.ZipInputStream

object ZipUtil {
  
    @Throws(IOException::class)
    fun unzip(file : File, targetDirectory : File) {
        ZipFile(file).use { zipFile ->
            for (zipEntry in zipFile.entries()) {
                val destFile = createNewFile(targetDirectory, zipEntry)
                // If the zip entry is a file, we need to create its parent directories
                val destDirectory : File? = if (zipEntry.isDirectory) destFile else destFile.parentFile

                // Create the destination directory
                if (destDirectory == null || (!destDirectory.isDirectory && !destDirectory.mkdirs()))
                    throw FileNotFoundException("Failed to create destination directory: $destDirectory")

                // If the entry is a directory we don't need to copy anything
                if (zipEntry.isDirectory)
                    continue

                // Copy bytes to destination
                try {
                    zipFile.getInputStream(zipEntry).use { inputStream ->
                        destFile.outputStream().use { outputStream ->
                            inputStream.copyTo(outputStream)
                        }
                    }
                } catch (e : IOException) {
                    if (destFile.exists())
                        destFile.delete()
                    throw e
                }
            }
        }
    }

    @Throws(IOException::class)
    fun unzip(stream : InputStream, targetDirectory : File) {
        ZipInputStream(BufferedInputStream(stream)).use { zis ->
            do {
                // Get the next entry, break if we've reached the end
                val zipEntry = zis.nextEntry ?: break

                val destFile = createNewFile(targetDirectory, zipEntry)
                // If the zip entry is a file, we need to create its parent directories
                val destDirectory : File? = if (zipEntry.isDirectory) destFile else destFile.parentFile

                // Create the destination directory
                if (destDirectory == null || (!destDirectory.isDirectory && !destDirectory.mkdirs()))
                    throw FileNotFoundException("Failed to create destination directory: $destDirectory")

                // If the entry is a directory we don't need to copy anything
                if (zipEntry.isDirectory)
                    continue

                // Copy bytes to destination
                try {
                    BufferedOutputStream(destFile.outputStream()).use { zis.copyTo(it) }
                } catch (e : IOException) {
                    if (destFile.exists())
                        destFile.delete()
                    throw e
                }
            } while (true)
        }
    }

    @Throws(IOException::class)
    private fun createNewFile(destinationDir : File, zipEntry : ZipEntry) : File {
        val destFile = File(destinationDir, zipEntry.name)
        val destDirPath = destinationDir.canonicalPath
        val destFilePath = destFile.canonicalPath

        if (!destFilePath.startsWith(destDirPath + File.separator))
            throw IOException("Entry is outside of the target dir: " + zipEntry.name)

        return destFile
    }
}
