package com.chengxu.futari.song;

import com.chengxu.futari.common.annotation.RequirePermission;
import com.chengxu.futari.common.error.BizException;
import com.chengxu.futari.song.controller.LyricsCompletionController;
import com.chengxu.futari.song.dto.LyricsCompletionRequest;
import com.chengxu.futari.song.entity.Song;
import com.chengxu.futari.song.mapper.SongMapper;
import com.chengxu.futari.song.service.impl.LyricsCompletionServiceImpl;
import java.nio.charset.StandardCharsets;
import org.junit.jupiter.api.Test;
import org.springframework.mock.web.MockMultipartFile;

import static org.junit.jupiter.api.Assertions.assertEquals;
import static org.junit.jupiter.api.Assertions.assertFalse;
import static org.junit.jupiter.api.Assertions.assertNotNull;
import static org.junit.jupiter.api.Assertions.assertThrows;
import static org.junit.jupiter.api.Assertions.assertTrue;
import static org.mockito.Mockito.mock;
import static org.mockito.Mockito.never;
import static org.mockito.Mockito.verify;
import static org.mockito.Mockito.when;

/** 歌词补全的类型校验、防覆盖行为和管理权限。 @author Futari */
class LyricsCompletionTest {
    @Test
    void concurrentLineLyricsAreSkippedWithoutReplacingExistingContent() {
        SongMapper songs = mock(SongMapper.class);
        when(songs.selectById(7L)).thenReturn(new Song());
        when(songs.completeLineLyrics(7L, "[00:00.00]逐行歌词")).thenReturn(0);
        LyricsCompletionRequest request = request("line.lrc", "[00:00.00]逐行歌词");

        var result = new LyricsCompletionServiceImpl(songs).complete(7L, "line", request);

        assertFalse(result.applied());
        verify(songs).completeLineLyrics(7L, "[00:00.00]逐行歌词");
        verify(songs, never()).completeKaraokeLyrics(7L, "[00:00.00]逐行歌词");
    }

    @Test
    void karaokeLyricsRequireInlineCharacterTimestamps() {
        SongMapper songs = mock(SongMapper.class);
        when(songs.selectById(8L)).thenReturn(new Song());
        LyricsCompletionRequest request = request("line.lrc", "[00:00.00]只有逐行时间标签");

        assertThrows(BizException.class,
                     () -> new LyricsCompletionServiceImpl(songs).complete(8L, "karaoke", request));

        verify(songs, never()).completeKaraokeLyrics(8L, "[00:00.00]只有逐行时间标签");
    }

    @Test
    void validKaraokeLyricsFillOnlyTheKaraokeField() {
        SongMapper songs = mock(SongMapper.class);
        when(songs.selectById(9L)).thenReturn(new Song());
        String lyrics = "[00:00.00]<00:00.00>逐<00:00.20>字";
        when(songs.completeKaraokeLyrics(9L, lyrics)).thenReturn(1);
        LyricsCompletionRequest request = request("karaoke.lrc", lyrics);

        var result = new LyricsCompletionServiceImpl(songs).complete(9L, "karaoke", request);

        assertTrue(result.applied());
        verify(songs).completeKaraokeLyrics(9L, lyrics);
        verify(songs, never()).completeLineLyrics(9L, lyrics);
    }

    @Test
    void everyEndpointRequiresAdministratorPermission() {
        for (var method : LyricsCompletionController.class.getDeclaredMethods()) {
            RequirePermission permission = method.getAnnotation(RequirePermission.class);
            assertNotNull(permission);
            assertEquals("admin", permission.value());
        }
    }

    private LyricsCompletionRequest request(String name, String content) {
        var request = new LyricsCompletionRequest();
        request.setFile(new MockMultipartFile("file", name, "text/plain",
                                             content.getBytes(StandardCharsets.UTF_8)));
        return request;
    }
}
