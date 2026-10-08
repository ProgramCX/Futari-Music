package com.chengxu.futari.playlist.personal.dto;

import jakarta.validation.constraints.NotNull;
import jakarta.validation.constraints.Size;
import java.util.List;
import lombok.Getter;
import lombok.Setter;

/** 批量加歌或全量排序。 @author Futari */
@Getter @Setter
public class PlaylistSongsRequest { @NotNull @Size(max = 500) private List<@NotNull Long> songIds; }
