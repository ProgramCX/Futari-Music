package com.chengxu.futari.playlist.personal.service.impl;

import com.baomidou.mybatisplus.core.conditions.query.LambdaQueryWrapper;
import com.chengxu.futari.common.error.BizException;
import com.chengxu.futari.common.error.ErrorCode;
import com.chengxu.futari.playlist.personal.dto.*;
import com.chengxu.futari.playlist.personal.entity.*;
import com.chengxu.futari.playlist.personal.mapper.*;
import com.chengxu.futari.playlist.personal.service.UserPlaylistService;
import com.chengxu.futari.song.service.SongService;
import com.chengxu.futari.song.dto.SongResponse;
import java.time.ZoneId;
import java.util.HashSet;
import java.util.List;
import java.util.Set;
import lombok.RequiredArgsConstructor;
import org.springframework.stereotype.Service;
import org.springframework.transaction.annotation.Transactional;

/** 只允许歌单所有者读写的个人歌单实现。 @author Futari */
@Service @RequiredArgsConstructor
public class UserPlaylistServiceImpl implements UserPlaylistService {
    private final UserPlaylistMapper playlists;
    private final UserPlaylistSongMapper entries;
    private final SongService songs;
    private UserPlaylist owned(Long userId, Long id) {
        UserPlaylist row = playlists.selectOne(new LambdaQueryWrapper<UserPlaylist>().eq(UserPlaylist::getId, id).eq(UserPlaylist::getUserId, userId));
        if (row == null) throw new BizException(ErrorCode.PLAYLIST_NOT_FOUND);
        return row;
    }
    private List<UserPlaylistSong> entries(Long id) {
        return entries.selectList(new LambdaQueryWrapper<UserPlaylistSong>().eq(UserPlaylistSong::getPlaylistId, id).orderByAsc(UserPlaylistSong::getSortOrder).orderByAsc(UserPlaylistSong::getId));
    }
    private PlaylistResponse response(UserPlaylist row) {
        Long created = row.getCreatedAt() == null ? null : row.getCreatedAt().atZone(ZoneId.systemDefault()).toInstant().toEpochMilli();
        return new PlaylistResponse(row.getId(), row.getName(), row.getDescription(), row.getCoverUrl(), row.getUserId(), created,
                songs.findAvailableByIds(entries(row.getId()).stream().map(UserPlaylistSong::getSongId).toList()));
    }
    public List<PlaylistResponse> list(Long userId) {
        return playlists.selectList(new LambdaQueryWrapper<UserPlaylist>().eq(UserPlaylist::getUserId, userId).orderByDesc(UserPlaylist::getId)).stream().map(this::response).toList();
    }
    public PlaylistResponse get(Long userId, Long id) { return response(owned(userId, id)); }
    public PlaylistResponse create(Long userId, PlaylistCreateRequest request) {
        UserPlaylist row = new UserPlaylist(); row.setUserId(userId); row.setName(request.getName().trim()); row.setDescription(request.getDescription()); row.setCoverUrl(request.getCoverUrl());
        playlists.insert(row); return response(row);
    }
    public PlaylistResponse update(Long userId, Long id, PlaylistUpdateRequest request) {
        UserPlaylist row = owned(userId, id); row.setName(request.getName().trim()); row.setDescription(request.getDescription()); row.setCoverUrl(request.getCoverUrl());
        playlists.updateById(row); return response(row);
    }
    public void delete(Long userId, Long id) { owned(userId, id); playlists.deleteById(id); }
    @Transactional(rollbackFor = Exception.class)
    public AddSongsResponse addSongs(Long userId, Long id, List<Long> songIds) {
        owned(userId, id);
        if (songIds == null || songIds.size() > 500 || songIds.stream().anyMatch(songId -> songId == null || songId < 1)) throw new BizException(ErrorCode.PARAM);
        int added = 0;
        int order = entries(id).size();
        for (Long songId : songIds) { songs.require(songId); added += entries.insertIgnore(id, songId, order++); }
        return new AddSongsResponse(added, songIds.size() - added);
    }
    public void removeSong(Long userId, Long id, Long songId) { owned(userId, id); entries.removeSong(id, songId); }
    @Transactional(rollbackFor = Exception.class)
    public void reorder(Long userId, Long id, List<Long> songIds) {
        owned(userId, id);
        if (songIds == null || songIds.size() > 500) throw new BizException(ErrorCode.PARAM);
        Set<Long> present = new HashSet<>(songs.findAvailableByIds(entries(id).stream().map(UserPlaylistSong::getSongId).toList())
                .stream().map(SongResponse::getId).toList());
        if (present.size() != songIds.size() || !present.equals(new HashSet<>(songIds))) throw new BizException(ErrorCode.PLAYLIST_ORDER);
        for (int i = 0; i < songIds.size(); i++) entries.reorder(id, songIds.get(i), i);
    }
}
