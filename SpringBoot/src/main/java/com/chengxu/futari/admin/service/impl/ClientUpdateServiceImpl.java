package com.chengxu.futari.admin.service.impl;

import com.chengxu.futari.admin.dto.ClientUpdateInfoResponse;
import com.chengxu.futari.admin.service.ClientUpdateService;
import com.chengxu.futari.config.ClientUpdateProperties;
import com.fasterxml.jackson.databind.JsonNode;
import com.fasterxml.jackson.databind.ObjectMapper;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.net.URI;
import java.net.URLEncoder;
import java.net.http.HttpClient;
import java.net.http.HttpRequest;
import java.net.http.HttpResponse;
import java.nio.charset.StandardCharsets;
import java.nio.file.AtomicMoveNotSupportedException;
import java.nio.file.Files;
import java.nio.file.Path;
import java.nio.file.StandardCopyOption;
import java.security.DigestInputStream;
import java.security.MessageDigest;
import java.security.NoSuchAlgorithmException;
import java.time.Duration;
import java.util.ArrayList;
import java.util.Comparator;
import java.util.HexFormat;
import java.util.List;
import java.util.Map;
import java.util.Optional;
import java.util.concurrent.ConcurrentHashMap;
import java.util.regex.Matcher;
import java.util.regex.Pattern;
import lombok.extern.slf4j.Slf4j;
import org.springframework.beans.factory.ObjectProvider;
import org.springframework.beans.factory.annotation.Value;
import org.springframework.boot.info.BuildProperties;
import org.springframework.scheduling.annotation.Scheduled;
import org.springframework.stereotype.Service;

/** 从 GitHub 发行页筛选当前服务端兼容链，并缓存 Windows 与 Ubuntu 安装包。 @author Futari */
@Slf4j
@Service
public class ClientUpdateServiceImpl implements ClientUpdateService {
    private static final int RELEASE_PAGE_SIZE = 100;
    private static final int MAXIMUM_RELEASE_PAGES = 10;
    private static final long MAXIMUM_ASSET_SIZE = 2L * 1024 * 1024 * 1024;
    private static final Pattern SERVER_VERSION_PATTERN =
            Pattern.compile("^(\\d+)\\.(\\d+)\\.(\\d+)(?:\\.([A-Za-z][A-Za-z0-9.]*))?$");
    private static final Pattern SERVER_COMPATIBILITY_PATTERN = Pattern.compile("^(\\d+)\\.(\\d+)$");
    private static final Pattern CLIENT_VERSION_PATTERN =
            Pattern.compile("^(\\d+)\\.(\\d+)\\.(\\d+)(?:-([A-Za-z0-9.-]+))?$");

    private final ClientUpdateProperties properties;
    private final ObjectMapper objectMapper;
    private final ObjectProvider<BuildProperties> buildProperties;
    private final String fallbackServerVersion;
    private final HttpClient httpClient = HttpClient.newBuilder()
            .connectTimeout(Duration.ofSeconds(15))
            .followRedirects(HttpClient.Redirect.NORMAL)
            .build();
    // Published snapshots are replaced per platform only after a complete verified download.
    private final Map<String, CachedUpdate> latestByPlatform = new ConcurrentHashMap<>();

    public ClientUpdateServiceImpl(ClientUpdateProperties properties, ObjectMapper objectMapper,
                                   ObjectProvider<BuildProperties> buildProperties,
                                   @Value("${futari.server-version:0.1.0}") String fallbackServerVersion) {
        this.properties = properties;
        this.objectMapper = objectMapper;
        this.buildProperties = buildProperties;
        this.fallbackServerVersion = fallbackServerVersion;
    }

    @Override
    public ClientUpdateInfoResponse updateInfo(String platformName) {
        Platform platform = Platform.fromName(platformName);
        ServerVersion server = parseServerVersion(serverVersion()).orElse(null);
        if (platform == null || server == null) return emptyInfo(server);

        CachedUpdate update = latestByPlatform.get(platform.key);
        if (update == null || !update.serverVersion.compatibilityLine().equals(server.compatibilityLine())
                || !Files.isRegularFile(update.filePath)) return emptyInfo(server);

        String encodedTag = URLEncoder.encode(update.releaseTag, StandardCharsets.UTF_8);
        String downloadUrl = "api/updates/download?platform=" + platform.key + "&releaseTag=" + encodedTag;
        return new ClientUpdateInfoResponse(serverVersion(), server.compatibilityLine(),
                update.clientVersion.toString(), update.releaseTag, update.fileName,
                update.fileSize, update.sha256, downloadUrl);
    }

    @Override
    public Optional<Path> updateFile(String platformName, String releaseTag) {
        Platform platform = Platform.fromName(platformName);
        if (platform == null) return Optional.empty();
        CachedUpdate update = latestByPlatform.get(platform.key);
        if (update == null || !update.releaseTag.equals(releaseTag)
                || !Files.isRegularFile(update.filePath)) return Optional.empty();
        return Optional.of(update.filePath);
    }

    @Override
    @Scheduled(scheduler = "futariUpdateTaskScheduler",
            initialDelayString = "${futari.updates.poll-initial-delay-ms:0}",
            fixedDelayString = "${futari.updates.poll-interval-ms:600000}")
    public void refreshReleases() {
        if (!properties.isPollEnabled()) return;
        ServerVersion server = parseServerVersion(serverVersion()).orElse(null);
        if (server == null) {
            log.warn("Skipping client update scan because the configured server version is invalid");
            return;
        }
        try {
            Map<Platform, ReleaseAsset> candidates = selectCompatibleAssets(fetchReleases(), server);
            for (Map.Entry<Platform, ReleaseAsset> entry : candidates.entrySet()) {
                cacheAsset(entry.getKey(), entry.getValue());
            }
        } catch (InterruptedException exception) {
            Thread.currentThread().interrupt();
            log.warn("GitHub client update scan was interrupted");
        } catch (IOException | RuntimeException exception) {
            log.warn("Unable to refresh client updates from GitHub: {}", exception.getMessage());
        }
    }

    private String serverVersion() {
        BuildProperties build = buildProperties.getIfAvailable();
        return build == null ? fallbackServerVersion : build.getVersion();
    }

    private ClientUpdateInfoResponse emptyInfo(ServerVersion server) {
        String version = serverVersion();
        String line = server == null ? "" : server.compatibilityLine();
        return new ClientUpdateInfoResponse(version, line, "", "", "", 0, "", "");
    }

    private List<JsonNode> fetchReleases() throws IOException, InterruptedException {
        List<JsonNode> releases = new ArrayList<>();
        for (int page = 1; page <= MAXIMUM_RELEASE_PAGES; page++) {
            JsonNode response = getJson(releasesUri(page));
            if (!response.isArray()) throw new IOException("GitHub release response was not a list");
            response.forEach(releases::add);
            if (response.size() < RELEASE_PAGE_SIZE) break;
        }
        return releases;
    }

    private URI releasesUri(int page) {
        String repository = properties.getGithubRepository();
        if (repository == null || !repository.matches("[A-Za-z0-9_.-]+/[A-Za-z0-9_.-]+")) {
            throw new IllegalArgumentException("invalid GitHub repository setting");
        }
        return URI.create("https://api.github.com/repos/" + repository
                + "/releases?per_page=" + RELEASE_PAGE_SIZE + "&page=" + page);
    }

    private JsonNode getJson(URI uri) throws IOException, InterruptedException {
        HttpRequest request = HttpRequest.newBuilder(uri)
                .timeout(Duration.ofSeconds(30))
                .header("Accept", "application/vnd.github+json")
                .header("X-GitHub-Api-Version", "2022-11-28")
                .header("User-Agent", "Futari-Music-Update-Service")
                .GET().build();
        HttpResponse<String> response = httpClient.send(request,
                HttpResponse.BodyHandlers.ofString(StandardCharsets.UTF_8));
        if (response.statusCode() != 200) {
            throw new IOException("GitHub API returned HTTP " + response.statusCode());
        }
        return objectMapper.readTree(response.body());
    }

    private Map<Platform, ReleaseAsset> selectCompatibleAssets(List<JsonNode> releases,
                                                               ServerVersion server) {
        Map<Platform, ReleaseAsset> selected = new ConcurrentHashMap<>();
        for (JsonNode release : releases) {
            if (release.path("draft").asBoolean() || release.path("prerelease").asBoolean()) continue;
            Optional<ReleaseTag> tag = parseReleaseTag(release.path("tag_name").asText());
            if (tag.isEmpty() || !tag.get().serverVersion.compatibilityLine()
                    .equals(server.compatibilityLine())) continue;
            selectAssetsForRelease(release.path("assets"), tag.get(), selected);
        }
        return selected;
    }

    private void selectAssetsForRelease(JsonNode assets, ReleaseTag tag,
                                        Map<Platform, ReleaseAsset> selected) {
        for (Platform platform : Platform.values()) {
            findPlatformAsset(assets, platform).ifPresent(asset -> {
                ReleaseAsset candidate = new ReleaseAsset(tag, platform, asset);
                selected.compute(platform, (ignored, current) ->
                        current == null || candidate.isNewerThan(current) ? candidate : current);
            });
        }
    }

    private Optional<JsonNode> findPlatformAsset(JsonNode assets, Platform platform) {
        if (!assets.isArray()) return Optional.empty();
        for (JsonNode asset : assets) {
            String name = asset.path("name").asText();
            if ("uploaded".equals(asset.path("state").asText())
                    && name.endsWith(platform.assetSuffix)
                    && asset.path("size").asLong() > 0
                    && asset.path("size").asLong() <= MAXIMUM_ASSET_SIZE) return Optional.of(asset);
        }
        return Optional.empty();
    }

    static Optional<ReleaseTag> parseReleaseTag(String tag) {
        if (tag == null) return Optional.empty();
        String[] versions = tag.split("-", 2);
        if (versions.length != 2) return Optional.empty();
        Optional<ServerVersion> server = parseReleaseServerVersion(versions[0]);
        Optional<ClientVersion> client = parseClientVersion(versions[1]);
        if (server.isEmpty() || client.isEmpty()) return Optional.empty();
        return Optional.of(new ReleaseTag(tag, server.get(), client.get()));
    }

    private static Optional<ServerVersion> parseReleaseServerVersion(String version) {
        Matcher compatibilityMatcher =
                SERVER_COMPATIBILITY_PATTERN.matcher(version == null ? "" : version);
        if (compatibilityMatcher.matches()) {
            return Optional.of(new ServerVersion(Integer.parseInt(compatibilityMatcher.group(1)),
                    Integer.parseInt(compatibilityMatcher.group(2)), 0, null));
        }
        // Accept previously published full-version tags while new releases use the shorter chain prefix.
        return parseServerVersion(version);
    }

    static Optional<ServerVersion> parseServerVersion(String version) {
        Matcher matcher = SERVER_VERSION_PATTERN.matcher(version == null ? "" : version);
        if (!matcher.matches()) return Optional.empty();
        return Optional.of(new ServerVersion(Integer.parseInt(matcher.group(1)),
                Integer.parseInt(matcher.group(2)), Integer.parseInt(matcher.group(3)),
                matcher.group(4)));
    }

    static Optional<ClientVersion> parseClientVersion(String version) {
        Matcher matcher = CLIENT_VERSION_PATTERN.matcher(version == null ? "" : version);
        if (!matcher.matches()) return Optional.empty();
        return Optional.of(new ClientVersion(Integer.parseInt(matcher.group(1)),
                Integer.parseInt(matcher.group(2)), Integer.parseInt(matcher.group(3)),
                matcher.group(4)));
    }

    private void cacheAsset(Platform platform, ReleaseAsset asset) throws IOException, InterruptedException {
        String fileName = asset.fileName();
        if (!isSafeAssetName(fileName) || !asset.downloadUrl().startsWith("https://github.com/")) {
            throw new IOException("GitHub release contains an invalid client asset");
        }
        Path platformDirectory = Path.of(properties.getDirectory()).toAbsolutePath().normalize()
                .resolve(platform.key).normalize();
        Files.createDirectories(platformDirectory);
        Path target = platformDirectory.resolve(fileName).normalize();
        if (!target.startsWith(platformDirectory)) throw new IOException("client asset path escaped update directory");
        CachedUpdate current = latestByPlatform.get(platform.key);
        if (isCachedFileCurrent(current, asset, target)) return;

        CachedUpdate diskCache = restoreCachedAsset(asset, target);
        if (diskCache != null) {
            latestByPlatform.put(platform.key, diskCache);
            log.info("Restored cached client release {} for {}", asset.tag.releaseTag, platform.key);
            return;
        }

        CachedUpdate cached = downloadAsset(asset, target);
        latestByPlatform.put(platform.key, cached);
        log.info("Cached client release {} for {}", asset.tag.releaseTag, platform.key);
    }

    private boolean isCachedFileCurrent(CachedUpdate current, ReleaseAsset asset, Path target) {
        return current != null && current.releaseTag.equals(asset.tag.releaseTag)
                && current.filePath.equals(target) && Files.isRegularFile(target)
                && current.fileSize == asset.fileSize();
    }

    private CachedUpdate restoreCachedAsset(ReleaseAsset asset, Path target) throws IOException {
        String githubDigest = asset.githubDigest();
        if (!githubDigest.startsWith("sha256:") || !Files.isRegularFile(target)
                || Files.size(target) != asset.fileSize()) return null;
        try {
            String digest = hashFile(target);
            verifyGithubDigest(githubDigest, digest);
            return new CachedUpdate(asset.platform.key, asset.tag.releaseTag,
                    asset.tag.serverVersion, asset.tag.clientVersion,
                    target.getFileName().toString(), target, asset.fileSize(), digest);
        } catch (IOException exception) {
            log.warn("Ignoring an invalid cached client release for {}", asset.platform.key);
            return null;
        }
    }

    private CachedUpdate downloadAsset(ReleaseAsset asset, Path target)
            throws IOException, InterruptedException {
        URI uri = URI.create(asset.downloadUrl());
        if (!"https".equals(uri.getScheme()) || !"github.com".equalsIgnoreCase(uri.getHost())) {
            throw new IOException("GitHub client download URL is not trusted");
        }
        HttpRequest request = HttpRequest.newBuilder(uri).timeout(Duration.ofMinutes(20))
                .header("Accept", "application/octet-stream")
                .header("User-Agent", "Futari-Music-Update-Service")
                .GET().build();
        Path temporaryFile = Files.createTempFile(target.getParent(), ".futari-update-", ".part");
        try {
            HttpResponse<InputStream> response = httpClient.send(request,
                    HttpResponse.BodyHandlers.ofInputStream());
            if (response.statusCode() != 200) {
                response.body().close();
                throw new IOException("GitHub asset download returned HTTP " + response.statusCode());
            }

            String digest = writeAndHash(response.body(), temporaryFile);
            long size = Files.size(temporaryFile);
            if (size != asset.fileSize()) throw new IOException("downloaded client asset size did not match");
            verifyGithubDigest(asset.githubDigest(), digest);
            moveIntoPlace(temporaryFile, target);
            return new CachedUpdate(asset.platform.key, asset.tag.releaseTag,
                    asset.tag.serverVersion, asset.tag.clientVersion,
                    target.getFileName().toString(), target, size, digest);
        } finally {
            Files.deleteIfExists(temporaryFile);
        }
    }

    private String writeAndHash(InputStream input, Path temporaryFile) throws IOException {
        MessageDigest digest = sha256Digest();
        try (InputStream source = new DigestInputStream(input, digest)) {
            Files.copy(source, temporaryFile, StandardCopyOption.REPLACE_EXISTING);
        }
        return HexFormat.of().formatHex(digest.digest());
    }

    private String hashFile(Path file) throws IOException {
        MessageDigest digest = sha256Digest();
        try (InputStream source = new DigestInputStream(Files.newInputStream(file), digest)) {
            source.transferTo(OutputStream.nullOutputStream());
        }
        return HexFormat.of().formatHex(digest.digest());
    }

    private MessageDigest sha256Digest() {
        try {
            return MessageDigest.getInstance("SHA-256");
        } catch (NoSuchAlgorithmException exception) {
            throw new IllegalStateException("SHA-256 is unavailable", exception);
        }
    }

    private void verifyGithubDigest(String expectedDigest, String actualDigest) throws IOException {
        if (expectedDigest == null || !expectedDigest.startsWith("sha256:")) return;
        if (!expectedDigest.substring("sha256:".length()).equalsIgnoreCase(actualDigest)) {
            throw new IOException("downloaded client asset failed GitHub SHA-256 verification");
        }
    }

    private void moveIntoPlace(Path temporaryFile, Path target) throws IOException {
        try {
            Files.move(temporaryFile, target, StandardCopyOption.REPLACE_EXISTING,
                    StandardCopyOption.ATOMIC_MOVE);
        } catch (AtomicMoveNotSupportedException exception) {
            Files.move(temporaryFile, target, StandardCopyOption.REPLACE_EXISTING);
        }
    }

    private boolean isSafeAssetName(String fileName) {
        return fileName != null && fileName.matches("[A-Za-z0-9._+-]+")
                && Path.of(fileName).getFileName().toString().equals(fileName);
    }

    /** 支持更新的桌面平台及其 Release 附件后缀。 @author Futari */
    private enum Platform {
        WINDOWS("windows-x64", "-windows-x64.exe"),
        UBUNTU("ubuntu-amd64", "-ubuntu-amd64.deb");

        private final String key;
        private final String assetSuffix;

        Platform(String key, String assetSuffix) {
            this.key = key;
            this.assetSuffix = assetSuffix;
        }

        private static Platform fromName(String name) {
            for (Platform platform : values()) if (platform.key.equals(name)) return platform;
            return null;
        }
    }

    /** 服务端语义版本；主次号共同决定客户端兼容链。 @author Futari */
    record ServerVersion(int major, int minor, int patch, String suffix) {
        String compatibilityLine() { return major + "." + minor; }
        String toVersionString() {
            return major + "." + minor + "." + patch + (suffix == null ? "" : "." + suffix);
        }
        int compareTo(ServerVersion other) {
            int numeric = Comparator.comparingInt(ServerVersion::major)
                    .thenComparingInt(ServerVersion::minor)
                    .thenComparingInt(ServerVersion::patch).compare(this, other);
            return numeric == 0 ? compareSuffix(suffix, other.suffix) : numeric;
        }
    }

    /** 客户端语义版本，用于同一兼容链内挑选最高发行版。 @author Futari */
    record ClientVersion(int major, int minor, int patch, String prerelease)
            implements Comparable<ClientVersion> {
        @Override
        public int compareTo(ClientVersion other) {
            int numeric = Comparator.comparingInt(ClientVersion::major)
                    .thenComparingInt(ClientVersion::minor)
                    .thenComparingInt(ClientVersion::patch).compare(this, other);
            return numeric == 0 ? compareSuffix(prerelease, other.prerelease) : numeric;
        }
        @Override
        public String toString() {
            return major + "." + minor + "." + patch
                    + (prerelease == null ? "" : "-" + prerelease);
        }
    }

    private static int compareSuffix(String left, String right) {
        // SemVer 预发布标识中数字段按数值排序，正式版本高于预发布版本。
        if (left == null || left.isEmpty()) return right == null || right.isEmpty() ? 0 : 1;
        if (right == null || right.isEmpty()) return -1;
        String[] leftParts = left.split("\\.");
        String[] rightParts = right.split("\\.");
        for (int index = 0; index < Math.min(leftParts.length, rightParts.length); index++) {
            int part = compareVersionPart(leftParts[index], rightParts[index]);
            if (part != 0) return part;
        }
        return Integer.compare(leftParts.length, rightParts.length);
    }

    private static int compareVersionPart(String left, String right) {
        boolean leftNumeric = left.matches("\\d+");
        boolean rightNumeric = right.matches("\\d+");
        if (leftNumeric && rightNumeric) return Long.compare(Long.parseLong(left), Long.parseLong(right));
        if (leftNumeric != rightNumeric) return leftNumeric ? -1 : 1;
        return left.compareToIgnoreCase(right);
    }

    /** 发行标签中解析出的服务端和客户端版本。 @author Futari */
    record ReleaseTag(String releaseTag, ServerVersion serverVersion, ClientVersion clientVersion) {}

    /** GitHub 上待下载的发行附件。 @author Futari */
    private record ReleaseAsset(ReleaseTag tag, Platform platform, JsonNode asset) {
        private String fileName() { return asset.path("name").asText(); }
        private String downloadUrl() { return asset.path("browser_download_url").asText(); }
        private long fileSize() { return asset.path("size").asLong(); }
        private String githubDigest() { return asset.path("digest").asText(""); }
        private boolean isNewerThan(ReleaseAsset other) {
            int clientOrder = tag.clientVersion.compareTo(other.tag.clientVersion);
            return clientOrder > 0 || (clientOrder == 0
                    && tag.serverVersion.compareTo(other.tag.serverVersion) > 0);
        }
    }

    /** 已校验并缓存到本地的当前平台发行包。 @author Futari */
    private record CachedUpdate(String platform, String releaseTag, ServerVersion serverVersion,
                                ClientVersion clientVersion, String fileName, Path filePath,
                                long fileSize, String sha256) {}
}
