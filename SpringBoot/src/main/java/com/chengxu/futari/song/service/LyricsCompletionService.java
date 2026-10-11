package com.chengxu.futari.song.service;

import com.chengxu.futari.common.result.PageResult;
import com.chengxu.futari.song.dto.LyricsCandidateResponse;
import com.chengxu.futari.song.dto.LyricsCompletionRequest;
import com.chengxu.futari.song.dto.LyricsCompletionResponse;

/** 管理员补全空歌词字段的业务接口。 @author Futari */
public interface LyricsCompletionService {
    PageResult<LyricsCandidateResponse> candidates(String type, int pageNum, int pageSize);

    LyricsCompletionResponse complete(Long songId, String type,
                                      LyricsCompletionRequest request);
}
