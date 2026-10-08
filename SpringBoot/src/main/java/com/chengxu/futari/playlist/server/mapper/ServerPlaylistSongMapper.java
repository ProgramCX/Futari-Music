package com.chengxu.futari.playlist.server.mapper;

import com.baomidou.mybatisplus.core.mapper.BaseMapper;
import com.chengxu.futari.playlist.server.entity.ServerPlaylistSong;
import org.apache.ibatis.annotations.Delete;
import org.apache.ibatis.annotations.Insert;
import org.apache.ibatis.annotations.Mapper;
import org.apache.ibatis.annotations.Param;
import org.apache.ibatis.annotations.Update;

/** 公共歌单歌曲数据访问。 @author Futari */
@Mapper
public interface ServerPlaylistSongMapper extends BaseMapper<ServerPlaylistSong> {
    @Insert("INSERT IGNORE INTO server_playlist_song (playlist_id, song_id, sort_order, created_at, updated_at, deleted) VALUES (#{playlistId}, #{songId}, #{sortOrder}, NOW(), NOW(), 0)")
    int insertIgnore(@Param("playlistId") Long playlistId, @Param("songId") Long songId, @Param("sortOrder") int sortOrder);
    @Delete("DELETE FROM server_playlist_song WHERE playlist_id = #{playlistId} AND song_id = #{songId}")
    int removeSong(@Param("playlistId") Long playlistId, @Param("songId") Long songId);
    @Update("UPDATE server_playlist_song SET sort_order = #{sortOrder}, updated_at = NOW() WHERE playlist_id = #{playlistId} AND song_id = #{songId} AND deleted = 0")
    int reorder(@Param("playlistId") Long playlistId, @Param("songId") Long songId, @Param("sortOrder") int sortOrder);
}
