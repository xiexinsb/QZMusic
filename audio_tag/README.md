# QZMusic SIMPLE Tablib Wrapper(just a sample here,DO NOT use for produce directly)

本组件是 [TagLib](https://github.com/taglib/taglib) 的一个简单 JNI（Java Native Interface）封装。它允许你在 Java 或 Kotlin 应用程序中轻松地读取和写入音频文件的元数据。用于QZMusic的本地音乐标签读写功能实现

## 简介

TagLib 是一个用于读取和编辑多种音频文件格式（如 MP3、FLAC 等）元数据的库。本项目通过 JNI 将 TagLib 的功能暴露给 Java/Kotlin 代码，使得在 Android 或其他 Java 平台上处理音频标签变得简单和高效。

## 功能特性

- 支持多种音频格式（MP3,FLAC, WAV......）
- 读取和修改音频文件的元数据（标题、艺术家、专辑、年份、内嵌歌词、封面等）
- 高性能的 C++ 实现，通过 JNI 高效调用
- 使用FD访问文件避免Native层权限问题
- 适用于 Android 和其他 Java 平台

## 文件结构

```
.
├── AudioTag.kt          # Kotlin 接口，提供音频标签操作的 API
├── jni.cpp              # JNI 实现，桥接 C++ TagLib 和 Java/Kotlin
└── taglib_arm64v8a_prebuilt.7z  # 预编译的 TagLib 库（ARM64-v8a 架构）
```

## 依赖

本项目依赖于开源组件库 **TagLib**：

- **仓库地址**: [https://github.com/taglib/taglib](https://github.com/taglib/taglib)
- **使用协议**: Apache License 2.0

## 许可证

本组件使用的 TagLib 库遵循 **Apache License 2.0**。  
请遵守相关许可证条款。

## 致谢

感谢 [TagLib 团队](https://github.com/taglib/taglib) 提供的优秀开源库。

---

*本示例仅作为 JNI 封装的简单演示，生产环境建议自行封装避免意外BUG和不兼容问题*
*仅为测试版本,不保证可用性和稳定性*
