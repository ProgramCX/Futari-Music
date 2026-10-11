package com.chengxu.futari.song.service.impl;

import com.chengxu.futari.common.error.BizException;
import com.chengxu.futari.common.error.ErrorCode;
import java.nio.ByteBuffer;
import java.nio.charset.CharacterCodingException;
import java.nio.charset.CodingErrorAction;
import java.nio.charset.StandardCharsets;
import java.util.Locale;
import java.util.Set;
import java.util.regex.Pattern;
import org.springframework.web.multipart.MultipartFile;

/** 统一校验歌词上传的 UTF-8 编码、文件大小和歌词类型。 @author Futari */
public final class LyricsFiles {
    private static final Set<String> FORMATS = Set.of("lrc", "txt");
    private static final Pattern KARAOKE_TIMESTAMP =
            Pattern.compile("<\\d+:\\d{1,2}(?:\\.\\d+)?>");
    private static final int MAX_BYTES = 256 * 1024;

    private LyricsFiles() {}

    public static String readLyrics(MultipartFile file) {
        if (file == null || file.isEmpty() || !FORMATS.contains(extension(file)))
            throw new BizException(ErrorCode.SONG_LYRICS_FORMAT);
        if (file.getSize() > MAX_BYTES) throw new BizException(ErrorCode.SONG_LYRICS_SIZE);
        try {
            byte[] bytes = file.getBytes();
            if (bytes.length > MAX_BYTES) {
                throw new BizException(ErrorCode.SONG_LYRICS_SIZE);
            }
            String decoded = StandardCharsets.UTF_8.newDecoder()
                    .onMalformedInput(CodingErrorAction.REPORT)
                    .onUnmappableCharacter(CodingErrorAction.REPORT)
                    .decode(ByteBuffer.wrap(bytes)).toString();
            String text = decoded.startsWith("\uFEFF") ? decoded.substring(1) : decoded;
            return text;
        } catch (CharacterCodingException ex) {
            throw new BizException(ErrorCode.SONG_LYRICS_FORMAT);
        } catch (java.io.IOException ex) {
            throw new BizException(ErrorCode.SONG_STORAGE);
        }
    }

    public static boolean hasKaraokeTimestamps(String text) {
        return KARAOKE_TIMESTAMP.matcher(text).find();
    }

    private static String extension(MultipartFile file) {
        String name = file.getOriginalFilename();
        int dot = name == null ? -1 : name.lastIndexOf('.');
        return dot < 0 ? "" : name.substring(dot + 1).toLowerCase(Locale.ROOT);
    }
}
