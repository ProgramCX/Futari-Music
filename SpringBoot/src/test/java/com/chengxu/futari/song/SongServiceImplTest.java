package com.chengxu.futari.song;

import com.chengxu.futari.common.error.BizException;
import com.chengxu.futari.common.error.ErrorCode;
import com.chengxu.futari.song.dto.SongUploadRequest;
import com.chengxu.futari.song.dto.SongUpdateRequest;
import com.chengxu.futari.song.entity.Song;
import com.chengxu.futari.song.entity.Album;
import com.chengxu.futari.song.mapper.SongMapper;
import com.chengxu.futari.song.service.AlbumService;
import com.chengxu.futari.song.service.impl.SongServiceImpl;
import java.nio.file.Path;
import org.junit.jupiter.api.Test;
import org.junit.jupiter.api.io.TempDir;
import org.springframework.mock.web.MockMultipartFile;
import org.springframework.test.util.ReflectionTestUtils;
import org.springframework.util.unit.DataSize;
import org.mockito.ArgumentCaptor;
import java.nio.file.Files;
import java.util.Collection;
import java.util.List;
import java.util.concurrent.atomic.AtomicReference;
import org.springframework.web.multipart.MultipartFile;

import static org.junit.jupiter.api.Assertions.*;
import static org.mockito.ArgumentMatchers.any;
import static org.mockito.ArgumentMatchers.anyCollection;
import static org.mockito.Mockito.*;

/** 歌曲去重和封面格式校验。 @author Futari */
class SongServiceImplTest {
    @TempDir Path storage;
    @Test void playlistLookupSkipsMissingSongsAndPreservesOrder() {
        SongMapper mapper = mock(SongMapper.class);
        AlbumService albums = mock(AlbumService.class);
        Song first = new Song(); first.setId(1L); first.setTitle("第一首");
        Song third = new Song(); third.setId(3L); third.setTitle("第三首");
        when(mapper.selectBatchIds(anyCollection())).thenReturn(List.of(first, third));
        when(albums.findByIds(anyCollection())).thenReturn(List.of());
        SongServiceImpl service = new SongServiceImpl(mapper, albums);

        assertEquals(List.of(3L, 1L), service.findAvailableByIds(List.of(3L, 2L, 1L)).stream().map(song -> song.getId()).toList());
        verify(mapper, never()).selectById(any());
        clearInvocations(mapper, albums);
        assertTrue(service.findAvailableByIds(List.of()).isEmpty());
        verifyNoInteractions(mapper, albums);
    }
    @Test void duplicateAudioKeepsOriginalMetadata() {
        SongMapper mapper = mock(SongMapper.class);
        Song original = new Song(); original.setId(12L); original.setHash("a".repeat(64)); original.setTitle("原歌名"); original.setArtist("原歌手"); original.setAlbum("原专辑"); original.setHasLyrics(false);
        when(mapper.selectOne(any())).thenReturn(original);
        SongServiceImpl service = new SongServiceImpl(mapper, mock(AlbumService.class));
        setDefaultUploadLimit(service);
        ReflectionTestUtils.setField(service, "storageDirectory", storage.toString());
        SongUploadRequest request = new SongUploadRequest();
        request.setFile(new MockMultipartFile("file", "test.mp3", "audio/mpeg", new byte[]{1, 2, 3}));
        request.setTitle("新歌名"); request.setArtist("新歌手"); request.setLyricsText("[00:00.00]新歌词");
        var response = service.upload(1L, request);
        assertEquals("原歌手", response.getArtist());
        assertEquals("原专辑", response.getAlbum());
        assertFalse(response.getHasLyrics());
        verify(mapper, never()).insert(any(Song.class));
    }
    @Test void coverExtensionMustMatchImageSignature() {
        SongMapper mapper = mock(SongMapper.class);
        SongServiceImpl service = new SongServiceImpl(mapper, mock(AlbumService.class));
        setDefaultUploadLimit(service);
        ReflectionTestUtils.setField(service, "storageDirectory", storage.toString());
        SongUploadRequest request = new SongUploadRequest();
        request.setFile(new MockMultipartFile("file", "test.mp3", "audio/mpeg", new byte[]{1}));
        request.setTitle("歌曲");
        request.setCoverFile(new MockMultipartFile("coverFile", "cover.jpg", "image/jpeg", new byte[]{1, 2, 3}));
        BizException ex = assertThrows(BizException.class, () -> service.upload(1L, request));
        assertEquals(ErrorCode.SONG_COVER_FORMAT, ex.getErrorCode());
        verifyNoInteractions(mapper);
    }
    @Test void reuploadRestoresDeletedAudioWithOnlyCurrentMetadata() {
        SongMapper mapper = mock(SongMapper.class);
        Song deleted = new Song(); deleted.setId(12L);
        when(mapper.selectDeletedByHash(any())).thenReturn(deleted);
        when(mapper.restoreDeleted(any())).thenReturn(1);
        SongServiceImpl service = new SongServiceImpl(mapper, mock(AlbumService.class));
        setDefaultUploadLimit(service);
        ReflectionTestUtils.setField(service, "storageDirectory", storage.toString());
        SongUploadRequest request = new SongUploadRequest();
        request.setFile(new MockMultipartFile("file", "test.flac", "audio/flac", new byte[]{1, 2, 3}));
        request.setTitle("重新上传"); request.setArtist("当前歌手");

        var response = service.upload(1L, request);

        assertEquals(12L, response.getId());
        ArgumentCaptor<Song> restored = ArgumentCaptor.forClass(Song.class);
        verify(mapper).restoreDeleted(restored.capture());
        assertEquals("重新上传", restored.getValue().getTitle());
        assertNull(restored.getValue().getLyrics());
        assertFalse(restored.getValue().getHasLyrics());
        assertNull(restored.getValue().getCoverHash());
        assertNull(restored.getValue().getAlbumId());
        assertEquals(0, restored.getValue().getDeleted());
        verify(mapper, never()).insert(any(Song.class));
    }

    @Test void simultaneousRestoreReturnsAlreadyRestoredSong() {
        SongMapper mapper = mock(SongMapper.class);
        Song deleted = new Song(); deleted.setId(12L);
        Song active = new Song(); active.setId(12L); active.setTitle("另一个上传已完成");
        when(mapper.selectOne(any())).thenReturn(null, active);
        when(mapper.selectDeletedByHash(any())).thenReturn(deleted);
        when(mapper.restoreDeleted(any())).thenReturn(0);
        SongServiceImpl service = new SongServiceImpl(mapper, mock(AlbumService.class));
        setDefaultUploadLimit(service);
        ReflectionTestUtils.setField(service, "storageDirectory", storage.toString());
        SongUploadRequest request = new SongUploadRequest();
        request.setFile(new MockMultipartFile("file", "test.flac", "audio/flac", new byte[]{1, 2, 3}));
        request.setTitle("歌曲");

        assertEquals("另一个上传已完成", service.upload(1L, request).getTitle());
        verify(mapper, never()).insert(any(Song.class));
    }
    @Test void uploadRejectsAudioAboveConfiguredLimitWithSpecificError() {
        SongMapper mapper = mock(SongMapper.class);
        SongServiceImpl service = new SongServiceImpl(mapper, mock(AlbumService.class));
        SongUploadRequest request = new SongUploadRequest();
        MultipartFile file = mock(MultipartFile.class);
        when(file.isEmpty()).thenReturn(false);
        when(file.getSize()).thenReturn(1025L);
        request.setFile(file);
        ReflectionTestUtils.setField(service, "maxAudioSize", DataSize.ofBytes(1024));

        BizException exception = assertThrows(BizException.class, () -> service.upload(1L, request));

        assertEquals(ErrorCode.SONG_UPLOAD_SIZE, exception.getErrorCode());
        verifyNoInteractions(mapper);
    }
    @Test void fileMetadataResolvesStoredAudioByValidatedHash() throws Exception {
        SongMapper mapper = mock(SongMapper.class);
        String hash = "c".repeat(64);
        Song song = new Song(); song.setId(19L); song.setHash(hash); song.setFormat("mp3");
        when(mapper.selectOne(any())).thenReturn(song);
        SongServiceImpl service = new SongServiceImpl(mapper, mock(AlbumService.class));
        ReflectionTestUtils.setField(service, "storageDirectory", storage.toString());
        byte[] audio = {1, 2, 3, 4};
        Files.write(storage.resolve(hash), audio);

        var file = service.file(hash);
        assertEquals("audio/mpeg", file.getContentType());
        assertEquals(audio.length, file.getFileSize());
        assertEquals(storage.resolve(hash).toString(), file.getFilePath());
    }
    @Test void newSongStoresLyricsAndCover() {
        SongMapper mapper = mock(SongMapper.class);
        when(mapper.insert(any(Song.class))).thenAnswer(call -> { ((Song) call.getArgument(0)).setId(7L); return 1; });
        AlbumService albums = mock(AlbumService.class);
        AtomicReference<Album> savedAlbum = new AtomicReference<>();
        when(albums.resolveForUpload(any(), any(), any(), any(), any())).thenAnswer(call -> {
            String name = call.getArgument(1);
            if (name == null) return null;
            Album album = new Album(); album.setId(31L); album.setName(name); album.setArtist(call.getArgument(2));
            album.setCoverHash(call.getArgument(3)); album.setCoverFormat(call.getArgument(4));
            savedAlbum.set(album); return album;
        });
        when(albums.findByIds(anyCollection())).thenAnswer(call -> {
            Collection<Long> ids = call.getArgument(0);
            return savedAlbum.get() != null && ids.contains(savedAlbum.get().getId()) ? List.of(savedAlbum.get()) : List.of();
        });
        SongServiceImpl service = new SongServiceImpl(mapper, albums);
        setDefaultUploadLimit(service);
        ReflectionTestUtils.setField(service, "storageDirectory", storage.toString());
        SongUploadRequest request = new SongUploadRequest();
        request.setFile(new MockMultipartFile("file", "test.mp3", "audio/mpeg", new byte[]{1, 2, 3}));
        request.setTitle("歌名"); request.setArtist("歌手"); request.setAlbum("专辑");
        request.setLyricsFile(new MockMultipartFile("lyricsFile", "test.lrc", "text/plain", "[00:01.00]歌词".getBytes(java.nio.charset.StandardCharsets.UTF_8)));
        byte[] png = new byte[]{(byte) 0x89, 0x50, 0x4E, 0x47, 0x0D, 0x0A, 0x1A, 0x0A, 1};
        request.setCoverFile(new MockMultipartFile("coverFile", "cover.png", "image/png", png));
        var response = service.upload(1L, request);
        ArgumentCaptor<Song> saved = ArgumentCaptor.forClass(Song.class);
        verify(mapper).insert(saved.capture());
        assertEquals("[00:01.00]歌词", saved.getValue().getLyrics());
        assertEquals("专辑", response.getAlbum());
        assertTrue(response.getHasLyrics());
        assertEquals(31L, response.getAlbumId());
        assertEquals("/api/albums/31/cover", response.getCoverUrl());
        assertTrue(Files.exists(storage.resolve("covers").resolve(savedAlbum.get().getCoverHash() + ".png")));
    }

    @Test void adminUpdateRenamesSharedAlbumAndCanClearLyrics() {
        SongMapper mapper = mock(SongMapper.class);
        AlbumService albums = mock(AlbumService.class);
        Song song = new Song(); song.setId(8L); song.setHash("a".repeat(64)); song.setTitle("旧歌名");
        song.setArtist("旧歌手"); song.setAlbumId(11L); song.setAlbum("旧专辑"); song.setLyrics("旧歌词"); song.setHasLyrics(true);
        Album album = new Album(); album.setId(11L); album.setName("新专辑"); album.setCoverHash("b".repeat(64)); album.setCoverFormat("png");
        when(mapper.selectById(8L)).thenReturn(song);
        when(albums.require(11L)).thenReturn(album);
        when(albums.updateMetadata(11L, "新专辑", null, null, null)).thenReturn(album);
        when(albums.findByIds(anyCollection())).thenReturn(List.of(album));
        SongServiceImpl service = new SongServiceImpl(mapper, albums);
        ReflectionTestUtils.setField(service, "storageDirectory", storage.toString());

        SongUpdateRequest request = new SongUpdateRequest();
        request.setTitle("新歌名"); request.setArtist("新歌手"); request.setAlbumId(11L);
        request.setAlbumName("新专辑"); request.setLyricsText("");
        var response = service.update(8L, request);

        assertEquals("新歌名", response.getTitle());
        assertEquals("新歌手", response.getArtist());
        assertEquals(11L, response.getAlbumId());
        assertFalse(response.getHasLyrics());
        assertEquals("新专辑", song.getAlbum());
        verify(mapper).updateAlbumName(11L, "新专辑");
        verify(mapper).updateById(song);
    }

    @Test void replacingWithAnotherSongsAudioFailsAndRemovesTemporaryFile() throws Exception {
        SongMapper mapper = mock(SongMapper.class);
        AlbumService albums = mock(AlbumService.class);
        Song original = new Song(); original.setId(8L); original.setTitle("歌曲");
        Song duplicate = new Song(); duplicate.setId(9L);
        when(mapper.selectById(8L)).thenReturn(original);
        when(mapper.selectOne(any())).thenReturn(duplicate);
        SongServiceImpl service = new SongServiceImpl(mapper, albums);
        setDefaultUploadLimit(service);
        ReflectionTestUtils.setField(service, "storageDirectory", storage.toString());
        SongUpdateRequest request = new SongUpdateRequest();
        request.setTitle("歌曲");
        request.setFile(new MockMultipartFile("file", "other.mp3", "audio/mpeg", new byte[]{1, 2, 3}));

        BizException ex = assertThrows(BizException.class, () -> service.update(8L, request));

        assertEquals(ErrorCode.SONG_DUPLICATE, ex.getErrorCode());
        try (var files = Files.list(storage)) { assertEquals(0, files.count()); }
        verify(mapper, never()).updateById(any(Song.class));
    }

    private static void setDefaultUploadLimit(SongServiceImpl service) {
        ReflectionTestUtils.setField(service, "maxAudioSize", DataSize.ofMegabytes(64));
    }
}
