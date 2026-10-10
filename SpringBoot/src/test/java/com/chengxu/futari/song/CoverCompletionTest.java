package com.chengxu.futari.song;

import com.baomidou.mybatisplus.core.MybatisConfiguration;
import com.baomidou.mybatisplus.core.metadata.TableInfoHelper;
import com.baomidou.mybatisplus.core.conditions.update.LambdaUpdateWrapper;
import com.chengxu.futari.common.error.BizException;
import com.chengxu.futari.song.controller.CoverCompletionController;
import com.chengxu.futari.common.annotation.RequirePermission;
import com.chengxu.futari.song.dto.CoverCompletionRequest;
import com.chengxu.futari.song.entity.Song;
import com.chengxu.futari.song.entity.Album;
import com.chengxu.futari.song.mapper.SongMapper;
import com.chengxu.futari.song.mapper.AlbumMapper;
import com.chengxu.futari.song.service.impl.CoverCompletionServiceImpl;
import java.nio.file.Path;
import org.apache.ibatis.builder.MapperBuilderAssistant;
import org.junit.jupiter.api.Test;
import org.junit.jupiter.api.io.TempDir;
import org.springframework.mock.web.MockMultipartFile;
import org.springframework.test.util.ReflectionTestUtils;
import static org.junit.jupiter.api.Assertions.*;
import static org.mockito.Mockito.*;

/** 补全接口权限、格式校验和防覆盖条件。 @author Futari */
class CoverCompletionTest {
    @TempDir Path directory;

    @Test void writesOnlyEmptySongCoverAndReportsConcurrentFill() {
        TableInfoHelper.initTableInfo(new MapperBuilderAssistant(new MybatisConfiguration(), "test"), Song.class);
        SongMapper songs = mock(SongMapper.class);
        AlbumMapper albums = mock(AlbumMapper.class);
        when(songs.selectById(1L)).thenReturn(new Song());
        when(songs.update(isNull(), any(LambdaUpdateWrapper.class))).thenAnswer(inv -> {
            LambdaUpdateWrapper<Song> query = inv.getArgument(1);
            assertTrue(query.getSqlSegment().contains("cover_hash IS NULL"));
            assertTrue(query.getSqlSegment().contains("OR cover_hash ="));
            assertTrue(query.getSqlSet().contains("cover_hash="));
            return 0;
        });
        var service = new CoverCompletionServiceImpl(songs, albums);
        ReflectionTestUtils.setField(service, "storageDirectory", directory.toString());
        var request = new CoverCompletionRequest();
        request.setFile(new MockMultipartFile("file", "cover.jpg", "image/jpeg", new byte[]{(byte)255, (byte)216, (byte)255, 0}));
        assertFalse(service.completeSong(1L, request).applied());
        verifyNoInteractions(albums);
    }

    @Test void invalidImageCannotWriteMetadata() {
        SongMapper songs = mock(SongMapper.class);
        when(songs.selectById(1L)).thenReturn(new Song());
        var service = new CoverCompletionServiceImpl(songs, mock(AlbumMapper.class));
        var request = new CoverCompletionRequest();
        request.setFile(new MockMultipartFile("file", "fake.png", "image/png", "not a png".getBytes()));
        assertThrows(BizException.class, () -> service.completeSong(1L, request));
        verify(songs, never()).update(isNull(), any(LambdaUpdateWrapper.class));
    }

    @Test void existingAlbumCoverIsSkippedWithoutWritingSongOrFile() {
        SongMapper songs = mock(SongMapper.class);
        AlbumMapper albums = mock(AlbumMapper.class);
        Album album = new Album(); album.setCoverHash("a".repeat(64));
        when(albums.selectById(9L)).thenReturn(album);
        var service = new CoverCompletionServiceImpl(songs, albums);
        assertFalse(service.completeAlbum(9L, new CoverCompletionRequest()).applied());
        verify(albums, never()).update(isNull(), any(LambdaUpdateWrapper.class));
        verifyNoInteractions(songs);
    }

    @Test void albumCompletionDoesNotWriteSongs() {
        TableInfoHelper.initTableInfo(new MapperBuilderAssistant(new MybatisConfiguration(), "test"), Album.class);
        SongMapper songs = mock(SongMapper.class);
        AlbumMapper albums = mock(AlbumMapper.class);
        when(albums.selectById(9L)).thenReturn(new Album());
        when(albums.update(isNull(), any(LambdaUpdateWrapper.class))).thenAnswer(inv -> {
            LambdaUpdateWrapper<Album> query = inv.getArgument(1);
            assertTrue(query.getSqlSegment().contains("cover_hash IS NULL"));
            return 1;
        });
        var service = new CoverCompletionServiceImpl(songs, albums);
        ReflectionTestUtils.setField(service, "storageDirectory", directory.toString());
        var request = new CoverCompletionRequest();
        request.setFile(new MockMultipartFile("file", "cover.jpg", "image/jpeg", new byte[]{(byte)255, (byte)216, (byte)255, 0}));
        assertTrue(service.completeAlbum(9L, request).applied());
        verifyNoInteractions(songs);
    }

    @Test void everyEndpointRequiresSongManagementPermission() {
        for (var method : CoverCompletionController.class.getDeclaredMethods()) {
            assertNotNull(method.getAnnotation(RequirePermission.class));
            assertEquals("admin", method.getAnnotation(RequirePermission.class).value());
        }
    }
}
