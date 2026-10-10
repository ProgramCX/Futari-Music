package com.chengxu.futari.song.service.impl;

import com.chengxu.futari.common.error.BizException;
import com.chengxu.futari.common.error.ErrorCode;
import org.springframework.web.multipart.MultipartFile;
import java.nio.file.*;
import java.security.MessageDigest;
import java.security.NoSuchAlgorithmException;
import java.util.HexFormat;

/** 上传与补全共用的封面校验及内容寻址存储。 @author Futari */
public final class CoverFiles {
    private static final long MAX_COVER_BYTES = 5L * 1024 * 1024;
    private CoverFiles() { }
    public record CoverPayload(byte[] bytes, String hash, String format) { }
    public static CoverPayload readCover(MultipartFile file) {
        if (file == null) return null;
        if (file.isEmpty()) throw new BizException(ErrorCode.SONG_COVER_FORMAT);
        if (file.getSize() > MAX_COVER_BYTES) throw new BizException(ErrorCode.SONG_COVER_SIZE);
        try {
            byte[] bytes = file.getBytes();
            if (bytes.length > MAX_COVER_BYTES) throw new BizException(ErrorCode.SONG_COVER_SIZE);
            String name = extension(file);
            String format;
            if (("jpg".equals(name) || "jpeg".equals(name)) && bytes.length >= 3 && (bytes[0] & 255) == 255 && (bytes[1] & 255) == 216 && (bytes[2] & 255) == 255) format = "jpg";
            else if ("png".equals(name) && bytes.length >= 8 && bytes[0] == (byte) 0x89 && bytes[1] == 0x50 && bytes[2] == 0x4E && bytes[3] == 0x47 && bytes[4] == 0x0D && bytes[5] == 0x0A && bytes[6] == 0x1A && bytes[7] == 0x0A) format = "png";
            else if ("webp".equals(name) && bytes.length >= 12 && bytes[0] == 'R' && bytes[1] == 'I' && bytes[2] == 'F' && bytes[3] == 'F' && bytes[8] == 'W' && bytes[9] == 'E' && bytes[10] == 'B' && bytes[11] == 'P') format = "webp";
            else throw new BizException(ErrorCode.SONG_COVER_FORMAT);
            return new CoverPayload(bytes, HexFormat.of().formatHex(MessageDigest.getInstance("SHA-256").digest(bytes)), format);
        } catch (NoSuchAlgorithmException | java.io.IOException ex) { throw new BizException(ErrorCode.SONG_STORAGE); }
    }
    public static void storeCover(Path directory, CoverPayload cover) throws java.io.IOException {
        Path covers = directory.resolve("covers");
        Files.createDirectories(covers);
        Path target = covers.resolve(cover.hash() + "." + cover.format());
        if (Files.exists(target)) return;
        Path temporary = Files.createTempFile(covers, "cover-", ".tmp");
        try {
            Files.write(temporary, cover.bytes());
            try { Files.move(temporary, target, StandardCopyOption.ATOMIC_MOVE); }
            catch (FileAlreadyExistsException ex) { Files.deleteIfExists(temporary); }
        } finally { Files.deleteIfExists(temporary); }
    }
    private static String extension(MultipartFile file) {
        String name = file.getOriginalFilename();
        int dot = name == null ? -1 : name.lastIndexOf('.');
        return dot < 0 ? "" : name.substring(dot + 1).toLowerCase(java.util.Locale.ROOT);
    }
}
