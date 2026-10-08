package com.chengxu.futari.playlist.personal;

import com.chengxu.futari.playlist.personal.entity.UserPlaylist;
import com.chengxu.futari.playlist.personal.mapper.UserPlaylistMapper;
import com.chengxu.futari.playlist.personal.mapper.UserPlaylistSongMapper;
import com.chengxu.futari.playlist.personal.service.impl.UserPlaylistServiceImpl;
import com.chengxu.futari.song.service.SongService;
import java.util.List;
import org.junit.jupiter.api.Test;

import static org.junit.jupiter.api.Assertions.*;
import static org.mockito.ArgumentMatchers.*;
import static org.mockito.Mockito.*;

/** 验证批量加歌的新增与重复计数。 @author Futari */
class UserPlaylistServiceImplTest {
    @Test void repeatedSongsAreReportedAsDuplicates() {
        UserPlaylistMapper playlists = mock(UserPlaylistMapper.class);
        UserPlaylistSongMapper entries = mock(UserPlaylistSongMapper.class);
        SongService songs = mock(SongService.class);
        UserPlaylist row = new UserPlaylist(); row.setId(7L); row.setUserId(3L);
        when(playlists.selectOne(any())).thenReturn(row);
        when(entries.selectList(any())).thenReturn(List.of());
        when(entries.insertIgnore(7L, 10L, 0)).thenReturn(1);
        when(entries.insertIgnore(7L, 10L, 1)).thenReturn(0);
        var result = new UserPlaylistServiceImpl(playlists, entries, songs).addSongs(3L, 7L, List.of(10L, 10L));
        assertEquals(1, result.getAdded());
        assertEquals(1, result.getDuplicated());
    }
}
