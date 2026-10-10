package com.chengxu.futari.song.service;

import com.chengxu.futari.song.dto.*;
import com.chengxu.futari.common.result.PageResult;

/** 管理员封面补全，读取候选与按空值条件写入。 @author Futari */
public interface CoverCompletionService {
    PageResult<CoverCandidateResponse> songs(int pageNum, int pageSize);
    PageResult<AlbumResponse> albums(int pageNum, int pageSize);
    CoverCompletionResponse completeSong(Long id, CoverCompletionRequest request);
    CoverCompletionResponse completeAlbum(Long id, CoverCompletionRequest request);
}
