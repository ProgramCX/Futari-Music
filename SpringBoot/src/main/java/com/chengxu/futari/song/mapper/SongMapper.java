package com.chengxu.futari.song.mapper;

import com.baomidou.mybatisplus.core.mapper.BaseMapper;
import com.baomidou.mybatisplus.core.metadata.IPage;
import com.baomidou.mybatisplus.extension.plugins.pagination.Page;
import com.chengxu.futari.song.dto.CoverCandidateResponse;
import com.chengxu.futari.song.dto.LyricsCandidateResponse;
import com.chengxu.futari.song.entity.Song;
import org.apache.ibatis.annotations.Mapper;
import org.apache.ibatis.annotations.Select;
import org.apache.ibatis.annotations.Param;
import org.apache.ibatis.annotations.Update;

/** 歌曲数据访问。 @author Futari */
@Mapper public interface SongMapper extends BaseMapper<Song> {
    @Select("SELECT s.id, s.title, s.artist, s.album_id, a.name AS album_name, "
            + "(s.cover_hash IS NULL OR s.cover_hash = '') AS song_missing, "
            + "(a.id IS NOT NULL AND (a.cover_hash IS NULL OR a.cover_hash = '')) AS album_missing "
            + "FROM song s LEFT JOIN album a ON a.id = s.album_id AND a.deleted = 0 "
            + "WHERE s.deleted = 0 AND "
            + "(s.cover_hash IS NULL OR s.cover_hash = '' OR "
            + "(a.id IS NOT NULL AND (a.cover_hash IS NULL OR a.cover_hash = ''))) "
            + "ORDER BY s.id")
    IPage<CoverCandidateResponse> selectCoverCandidates(Page<CoverCandidateResponse> page);

    @Select("SELECT id, title, artist FROM song WHERE deleted = 0 "
            + "AND (lyrics IS NULL OR TRIM(lyrics) = '') ORDER BY id")
    IPage<LyricsCandidateResponse> selectSongsMissingLineLyrics(
            Page<LyricsCandidateResponse> page);

    @Select("SELECT id, title, artist FROM song WHERE deleted = 0 "
            + "AND (karaoke_lyrics IS NULL OR TRIM(karaoke_lyrics) = '') ORDER BY id")
    IPage<LyricsCandidateResponse> selectSongsMissingKaraokeLyrics(
            Page<LyricsCandidateResponse> page);

    @Select("SELECT lyrics FROM song WHERE id = #{id} AND deleted = 0")
    String selectLyrics(@Param("id") Long id);

    @Select("SELECT karaoke_lyrics FROM song WHERE id = #{id} AND deleted = 0")
    String selectKaraokeLyrics(@Param("id") Long id);

    @Update("UPDATE song SET lyrics = #{lyrics,jdbcType=LONGVARCHAR}, "
            + "karaoke_lyrics = #{karaokeLyrics,jdbcType=LONGVARCHAR}, "
            + "has_lyrics = #{hasLyrics} WHERE id = #{id} AND deleted = 0")
    int updateLyrics(@Param("id") Long id, @Param("lyrics") String lyrics,
                     @Param("karaokeLyrics") String karaokeLyrics,
                     @Param("hasLyrics") boolean hasLyrics);

    @Update("UPDATE song SET lyrics = #{lyrics,jdbcType=LONGVARCHAR}, has_lyrics = TRUE "
            + "WHERE id = #{id} AND deleted = 0 "
            + "AND (lyrics IS NULL OR TRIM(lyrics) = '')")
    int completeLineLyrics(@Param("id") Long id, @Param("lyrics") String lyrics);

    @Update("UPDATE song SET karaoke_lyrics = #{lyrics,jdbcType=LONGVARCHAR}, has_lyrics = TRUE "
            + "WHERE id = #{id} AND deleted = 0 "
            + "AND (karaoke_lyrics IS NULL OR TRIM(karaoke_lyrics) = '')")
    int completeKaraokeLyrics(@Param("id") Long id, @Param("lyrics") String lyrics);

    @Select("SELECT id, hash FROM song WHERE hash = #{hash} AND deleted = 1")
    Song selectDeletedByHash(@Param("hash") String hash);

    // 显式重新上传允许恢复软删除记录；所有可选字段也必须覆盖，避免沿用旧歌词或封面。
    @Update("UPDATE song SET title = #{song.title}, artist = #{song.artist}, album_id = #{song.albumId}, "
            + "album = #{song.album}, lyrics = #{song.lyrics}, karaoke_lyrics = #{song.karaokeLyrics}, "
            + "has_lyrics = #{song.hasLyrics}, "
            + "cover_hash = #{song.coverHash}, cover_format = #{song.coverFormat}, duration_ms = #{song.durationMs}, "
            + "file_size = #{song.fileSize}, format = #{song.format}, uploader_id = #{song.uploaderId}, "
            + "created_at = NOW(), updated_at = NOW(), deleted = 0 "
            + "WHERE id = #{song.id} AND hash = #{song.hash} AND deleted = 1")
    int restoreDeleted(@Param("song") Song song);

    @Update("UPDATE song SET album = #{name} WHERE album_id = #{albumId} AND deleted = 0")
    int updateAlbumName(@Param("albumId") Long albumId, @Param("name") String name);
}
