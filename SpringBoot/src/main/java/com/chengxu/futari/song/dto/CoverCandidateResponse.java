package com.chengxu.futari.song.dto;

/** 缺封面歌曲及所属专辑，用于客户端预览匹配。 @author Futari */
public record CoverCandidateResponse(Long id, String title, String artist, Long albumId,
        String albumName, Boolean songMissing, Boolean albumMissing) { }
