// Uses TagLib to Parse!
// this is just a simple wrapper for JNI Bridge
// git->https://github.com/taglib/taglib
#include <unistd.h>
#include <sys/syscall.h>
#include <cstdint>
#include <fcntl.h>
#include <jni.h>
#include <ctime>
#include <sys/time.h>
#include <android/log.h>
#include <sys/stat.h>
#include <cstring>
#include <cstdlib>
#include <csignal>
#include <cstdio>
#include <taglib/fileref.h>
#include <taglib/tag.h>
#include <taglib/tbytevector.h>
#include <taglib/mpegfile.h>
#include <taglib/id3v2tag.h>
#include <taglib/id3v2frame.h>
#include <taglib/attachedpictureframe.h>
#include <taglib/unsynchronizedlyricsframe.h>
#include <taglib/flacfile.h>
#include <taglib/xiphcomment.h>

#define LOG_TAG "System"
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

extern "C" JNIEXPORT jobjectArray JNICALL
Java_love_qz_music_controller_storage_AudioTag_readMediaTags(JNIEnv *env, jobject thiz, jint fd) {
    if (fd < 0) {
        LOGE("Invalid file descriptor passed: %d", fd);
        return nullptr;
    }

    char fd_path[64];
    snprintf(fd_path, sizeof(fd_path), "/proc/self/fd/%d", fd);

    TagLib::FileRef f(fd_path);

    if (f.isNull() || !f.tag() || !f.file()) {
        return nullptr;
    }

    auto* mp3File = dynamic_cast<TagLib::MPEG::File*>(f.file());
    auto* flacFile = dynamic_cast<TagLib::FLAC::File*>(f.file());

    if (!mp3File && !flacFile) {
        return nullptr;
    }

    TagLib::Tag *tag = f.tag();
    std::string name = tag->title().to8Bit(true);
    std::string artist = tag->artist().to8Bit(true);
    std::string album = tag->album().to8Bit(true);

    std::string lyric;
    TagLib::ByteVector picData;

    if (mp3File && mp3File->ID3v2Tag()) {
        TagLib::ID3v2::Tag* id3v2 = mp3File->ID3v2Tag();

        TagLib::ID3v2::FrameList picFrames = id3v2->frameList("APIC");
        if (!picFrames.isEmpty()) {
            auto* picFrame = dynamic_cast<TagLib::ID3v2::AttachedPictureFrame*>(picFrames.front());
            if (picFrame) picData = picFrame->picture();
        }

        TagLib::ID3v2::FrameList lyricFrames = id3v2->frameList("USLT");
        if (!lyricFrames.isEmpty()) {
            auto* lyricFrame = dynamic_cast<TagLib::ID3v2::UnsynchronizedLyricsFrame*>(lyricFrames.front());
            if (lyricFrame) lyric = lyricFrame->text().to8Bit(true);
        }
    }
    else if (flacFile) {
        const TagLib::List<TagLib::FLAC::Picture*>& picList = flacFile->pictureList();
        if (!picList.isEmpty()) {
            picData = picList.front()->data();
        }

        if (flacFile->xiphComment()) {
            TagLib::Ogg::FieldListMap fields = flacFile->xiphComment()->fieldListMap();
            if (fields.contains("LYRICS")) {
                lyric = fields["LYRICS"].front().to8Bit(true);
            }
        }
    }

    jclass objClass = env->FindClass("java/lang/Object");
    jobjectArray resultArray = env->NewObjectArray(5, objClass, nullptr);

    env->SetObjectArrayElement(resultArray, 0, env->NewStringUTF(name.c_str()));
    env->SetObjectArrayElement(resultArray, 1, env->NewStringUTF(artist.c_str()));
    env->SetObjectArrayElement(resultArray, 2, env->NewStringUTF(album.c_str()));
    env->SetObjectArrayElement(resultArray, 3, env->NewStringUTF(lyric.c_str()));

    if (!picData.isEmpty()) {
        jbyteArray jPicData = env->NewByteArray(picData.size());
        env->SetByteArrayRegion(jPicData, 0, picData.size(), reinterpret_cast<const jbyte*>(picData.data()));
        env->SetObjectArrayElement(resultArray, 4, jPicData);
        env->DeleteLocalRef(jPicData);
    }

    return resultArray;
}

extern "C" JNIEXPORT jboolean JNICALL
Java_love_qz_music_controller_storage_AudioTag_writeMediaTags(JNIEnv *env, jobject thiz,
                                                jint fd,
                                                jstring j_title,
                                                jstring j_artist,
                                                jstring j_album,
                                                jstring j_lyric,
                                                jbyteArray j_pic) {
    char fd_path[64];
    snprintf(fd_path, sizeof(fd_path), "/proc/self/fd/%d", fd);

    const char *title = j_title ? env->GetStringUTFChars(j_title, nullptr) : nullptr;
    const char *artist = j_artist ? env->GetStringUTFChars(j_artist, nullptr) : nullptr;
    const char *album = j_album ? env->GetStringUTFChars(j_album, nullptr) : nullptr;
    const char *lyric = j_lyric ? env->GetStringUTFChars(j_lyric, nullptr) : nullptr;

    // 解析ByteArray(PIC)
    TagLib::ByteVector picData;
    if (j_pic) {
        jsize picLen = env->GetArrayLength(j_pic);
        jbyte *picBytes = env->GetByteArrayElements(j_pic, nullptr);
        picData.setData(reinterpret_cast<const char*>(picBytes), picLen);
        env->ReleaseByteArrayElements(j_pic, picBytes, JNI_ABORT);
    }

    bool is_saved = false;
    TagLib::FileRef f(fd_path);

    if (!f.isNull() && f.file()) {
        TagLib::Tag *tag = f.tag();

        if (title) tag->setTitle(TagLib::String(title, TagLib::String::UTF8));
        if (artist) tag->setArtist(TagLib::String(artist, TagLib::String::UTF8));
        if (album) tag->setAlbum(TagLib::String(album, TagLib::String::UTF8));

        auto* mp3File = dynamic_cast<TagLib::MPEG::File*>(f.file());
        auto* flacFile = dynamic_cast<TagLib::FLAC::File*>(f.file());

        // WARNNING+DOTO("加入额外格式判断?")
        TagLib::String mimeType = "image/jpeg";
        if (picData.size() > 2 && (unsigned char)picData[0] == 0x89 && (unsigned char)picData[1] == 0x50) {
            mimeType = "image/png";
        }

        if (mp3File && mp3File->ID3v2Tag()) {
            TagLib::ID3v2::Tag* id3v2 = mp3File->ID3v2Tag();

            if (lyric) {
                id3v2->removeFrames("USLT");
                if (strlen(lyric) > 0) {
                    auto* frame = new TagLib::ID3v2::UnsynchronizedLyricsFrame(TagLib::String::UTF8);
                    frame->setLanguage("eng"); // 默认语言标记
                    frame->setText(TagLib::String(lyric, TagLib::String::UTF8));
                    id3v2->addFrame(frame);
                }
            }

            if (j_pic) {
                id3v2->removeFrames("APIC");
                if (!picData.isEmpty()) {
                    auto* frame = new TagLib::ID3v2::AttachedPictureFrame();
                    frame->setMimeType(mimeType);
                    frame->setType(TagLib::ID3v2::AttachedPictureFrame::FrontCover);
                    frame->setPicture(picData);
                    id3v2->addFrame(frame);
                }
            }
        }
        
        else if (flacFile) {
            if (lyric && flacFile->xiphComment()) {
                if (strlen(lyric) > 0) {
                    flacFile->xiphComment()->addField("LYRICS", TagLib::String(lyric, TagLib::String::UTF8), true);
                } else {
                    flacFile->xiphComment()->removeFields("LYRICS");
                }
            }

            if (j_pic) {
                flacFile->removePictures();
                if (!picData.isEmpty()) {
                    auto* pic = new TagLib::FLAC::Picture();
                    pic->setMimeType(mimeType);
                    pic->setType(TagLib::FLAC::Picture::FrontCover);
                    pic->setData(picData);
                    flacFile->addPicture(pic);
                }
            }
        }

        is_saved = f.save();
    }

    if (title) env->ReleaseStringUTFChars(j_title, title);
    if (artist) env->ReleaseStringUTFChars(j_artist, artist);
    if (album) env->ReleaseStringUTFChars(j_album, album);
    if (lyric) env->ReleaseStringUTFChars(j_lyric, lyric);

    return is_saved ? JNI_TRUE : JNI_FALSE;
}
