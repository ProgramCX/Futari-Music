package com.chengxu.futari.playlist.personal;

import com.chengxu.futari.playlist.personal.entity.UserPlaylist;
import com.chengxu.futari.playlist.personal.entity.UserPlaylistSong;
import com.chengxu.futari.playlist.personal.mapper.UserPlaylistMapper;
import com.chengxu.futari.playlist.personal.mapper.UserPlaylistSongMapper;
import com.chengxu.futari.playlist.personal.service.impl.UserPlaylistServiceImpl;
import com.chengxu.futari.playlist.server.entity.ServerPlaylist;
import com.chengxu.futari.playlist.server.entity.ServerPlaylistSong;
import com.chengxu.futari.playlist.server.mapper.ServerPlaylistMapper;
import com.chengxu.futari.playlist.server.mapper.ServerPlaylistSongMapper;
import com.chengxu.futari.playlist.server.service.impl.ServerPlaylistServiceImpl;
import com.chengxu.futari.song.entity.Song;
import com.chengxu.futari.song.mapper.SongMapper;
import com.chengxu.futari.song.service.AlbumService;
import com.chengxu.futari.song.service.SongService;
import com.chengxu.futari.song.service.impl.SongServiceImpl;
import java.util.Collection;
import java.util.List;
import org.junit.jupiter.api.Test;
import static org.junit.jupiter.api.Assertions.*;
import static org.mockito.ArgumentMatchers.*;
import static org.mockito.Mockito.*;

/** 实际歌单与歌曲 Service 协作，失效关联不能阻塞列表和排序。 @author Futari */
class PlaylistAvailabilityTest {
    private SongService songs() {
        SongMapper mapper = mock(SongMapper.class);
        Song active = new Song(); active.setId(10L); active.setTitle("有效歌曲");
        when(mapper.selectBatchIds(anyCollection())).thenAnswer(call -> {
            Collection<Long> ids = call.getArgument(0);
            return ids.contains(10L) ? List.of(active) : List.of();
        });
        AlbumService albums = mock(AlbumService.class);
        when(albums.findByIds(anyCollection())).thenReturn(List.of());
        return new SongServiceImpl(mapper, albums);
    }
    @Test void personalPlaylistWithDeletedSongStillListsGetsAndReorders() {
        UserPlaylistMapper playlists = mock(UserPlaylistMapper.class);
        UserPlaylist row = new UserPlaylist(); row.setId(7L); row.setUserId(3L); row.setName("我喜欢");
        UserPlaylist empty = new UserPlaylist(); empty.setId(8L); empty.setUserId(3L); empty.setName("空歌单");
        when(playlists.selectList(any())).thenReturn(List.of(row, empty));
        when(playlists.selectOne(any())).thenReturn(row);
        UserPlaylistSong removed = new UserPlaylistSong(); removed.setSongId(99L);
        UserPlaylistSong active = new UserPlaylistSong(); active.setSongId(10L);
        UserPlaylistSongMapper entries = mock(UserPlaylistSongMapper.class);
        when(entries.selectList(any())).thenReturn(List.of(removed, active));
        UserPlaylistServiceImpl service = new UserPlaylistServiceImpl(playlists, entries, songs());

        assertEquals(2, service.list(3L).size());
        assertEquals(List.of(10L), service.get(3L, 7L).getSongs().stream().map(song -> song.getId()).toList());
        service.reorder(3L, 7L, List.of(10L));

        verify(entries).reorder(7L, 10L, 0);
        verify(entries, never()).reorder(eq(7L), eq(99L), anyInt());
    }
    @Test void serverPlaylistWithDeletedSongStillListsGetsAndReorders() {
        ServerPlaylistMapper playlists = mock(ServerPlaylistMapper.class);
        ServerPlaylist row = new ServerPlaylist(); row.setId(7L); row.setCreatorId(3L); row.setName("公共歌单");
        when(playlists.selectList(any())).thenReturn(List.of(row));
        when(playlists.selectById(7L)).thenReturn(row);
        ServerPlaylistSong removed = new ServerPlaylistSong(); removed.setSongId(99L);
        ServerPlaylistSong active = new ServerPlaylistSong(); active.setSongId(10L);
        ServerPlaylistSongMapper entries = mock(ServerPlaylistSongMapper.class);
        when(entries.selectList(any())).thenReturn(List.of(removed, active));
        ServerPlaylistServiceImpl service = new ServerPlaylistServiceImpl(playlists, entries, songs());

        assertEquals(1, service.list().size());
        assertEquals(List.of(10L), service.get(7L).getSongs().stream().map(song -> song.getId()).toList());
        service.reorder(7L, List.of(10L));

        verify(entries).reorder(7L, 10L, 0);
        verify(entries, never()).reorder(eq(7L), eq(99L), anyInt());
    }
}
