package com.chengxu.futari.song.service.impl;

import com.baomidou.mybatisplus.extension.plugins.pagination.Page;
import com.chengxu.futari.common.error.BizException;
import com.chengxu.futari.common.error.ErrorCode;
import com.chengxu.futari.common.result.PageResult;
import com.chengxu.futari.song.dto.LyricsCandidateResponse;
import com.chengxu.futari.song.dto.LyricsCompletionRequest;
import com.chengxu.futari.song.dto.LyricsCompletionResponse;
import com.chengxu.futari.song.mapper.SongMapper;
import com.chengxu.futari.song.service.LyricsCompletionService;
import lombok.RequiredArgsConstructor;
import org.springframework.stereotype.Service;
import org.springframework.transaction.annotation.Transactional;

/** 分页读取缺失歌词，并以条件更新避免覆盖已有内容。 @author Futari */
@Service
@RequiredArgsConstructor
public class LyricsCompletionServiceImpl implements LyricsCompletionService {
    private static final int MAX_PAGE_SIZE = 100;

    private final SongMapper songs;

    @Override
    public PageResult<LyricsCandidateResponse> candidates(String type, int pageNum, int pageSize) {
        validatePage(pageNum, pageSize);
        var page = new Page<LyricsCandidateResponse>(pageNum, pageSize);
        var result = switch (parseType(type)) {
            case LINE -> songs.selectSongsMissingLineLyrics(page);
            case KARAOKE -> songs.selectSongsMissingKaraokeLyrics(page);
        };
        return new PageResult<>(result.getTotal(), result.getRecords());
    }

    @Override
    @Transactional(rollbackFor = Exception.class)
    public LyricsCompletionResponse complete(Long songId, String type,
                                              LyricsCompletionRequest request) {
        LyricsType lyricsType = parseType(type);
        if (songs.selectById(songId) == null) throw new BizException(ErrorCode.NOT_FOUND);

        String lyrics = LyricsFiles.readLyrics(request.getFile());
        boolean containsCharacterTimestamps = LyricsFiles.hasKaraokeTimestamps(lyrics);
        if (lyrics.isBlank() || containsCharacterTimestamps != (lyricsType == LyricsType.KARAOKE)) {
            throw new BizException(ErrorCode.SONG_LYRICS_FORMAT);
        }

        int changed = lyricsType == LyricsType.LINE
                ? songs.completeLineLyrics(songId, lyrics)
                : songs.completeKaraokeLyrics(songId, lyrics);
        return new LyricsCompletionResponse(changed > 0);
    }

    private LyricsType parseType(String type) {
        return switch (type) {
            case "line" -> LyricsType.LINE;
            case "karaoke" -> LyricsType.KARAOKE;
            default -> throw new BizException(ErrorCode.PARAM);
        };
    }

    private void validatePage(int pageNum, int pageSize) {
        if (pageNum < 1 || pageSize < 1 || pageSize > MAX_PAGE_SIZE) {
            throw new BizException(ErrorCode.PARAM);
        }
    }

    private enum LyricsType { LINE, KARAOKE }
}
