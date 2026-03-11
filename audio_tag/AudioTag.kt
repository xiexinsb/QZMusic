package love.qz.music.controller.storage

import android.os.ParcelFileDescriptor
import love.qz.music.controller.sign.DecryptUtil
import love.qz.music.models.music.MediaTagInfo
import java.io.File

object AudioTag {
    init {
        System.loadLibrary("lib.so")
    }
    private external fun writeMediaTags(
        fd: Int,
        title: String?,
        artist: String?,
        album: String?,
        lyric: String?,
        pic: ByteArray?
    ): Boolean
    private external fun readMediaTags(fd: Int): Array<Any?>?
    fun read(path: String): MediaTagInfo? {
        val file = File(path)
        val pfd = ParcelFileDescriptor.open(file, ParcelFileDescriptor.MODE_READ_WRITE)
        pfd.use { descriptor ->
            var rawArray = readMediaTags(descriptor.fd)
            if (rawArray == null || rawArray.size < 5) {
                rawArray = arrayOf("未知歌曲", "未知歌手", "未知专辑", "", null)
            }

            val name = rawArray[0] as? String ?: "未知歌曲"
            val artist = rawArray[1] as? String ?: "未知歌手"
            val album = rawArray[2] as? String ?: "未知专辑"
            val lyric = rawArray[3] as? String ?: ""
            val picData = rawArray[4] as? ByteArray
            return MediaTagInfo(
                name = name,
                artist = artist,
                album = album,
                lyric = lyric,
                picData = picData
            )
        }
    }
}
