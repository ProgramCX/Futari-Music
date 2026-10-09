package com.chengxu.futari.admin.service.impl;

import org.junit.jupiter.api.Test;

import static org.junit.jupiter.api.Assertions.assertEquals;
import static org.junit.jupiter.api.Assertions.assertFalse;
import static org.junit.jupiter.api.Assertions.assertTrue;

/** Release tag examples lock down the client version chain and patch compatibility rule. */
class ClientUpdateServiceImplTest {
    @Test
    void tagParserReadsShortCompatibilityPrefixAndSplitsClientVersion() {
        var release = ClientUpdateServiceImpl.parseReleaseTag("0.1-0.0.1").orElseThrow();

        assertEquals("0.1.0", release.serverVersion().toVersionString());
        assertEquals("0.0.1", release.clientVersion().toString());
        assertEquals("0.1", release.serverVersion().compatibilityLine());
    }

    @Test
    void tagParserContinuesToAcceptPreviouslyPublishedFullServerVersions() {
        var release = ClientUpdateServiceImpl.parseReleaseTag("0.1.0.alpha-0.0.2").orElseThrow();

        assertEquals("0.1.0.alpha", release.serverVersion().toVersionString());
        assertEquals("0.1", release.serverVersion().compatibilityLine());
    }

    @Test
    void serverPatchVersionsShareAClientChainButMinorVersionsDoNot() {
        var release = ClientUpdateServiceImpl.parseReleaseTag("1.1-0.1.0").orElseThrow();
        var sameChain = ClientUpdateServiceImpl.parseServerVersion("1.1.1").orElseThrow();
        var newChain = ClientUpdateServiceImpl.parseServerVersion("1.2.0").orElseThrow();

        assertEquals(release.serverVersion().compatibilityLine(), sameChain.compatibilityLine());
        assertFalse(release.serverVersion().compatibilityLine().equals(newChain.compatibilityLine()));
    }

    @Test
    void clientVersionsAreOrderedNumericallyAcrossTheCompatibilityChain() {
        var first = ClientUpdateServiceImpl.parseReleaseTag("0.1-0.0.1").orElseThrow();
        var second = ClientUpdateServiceImpl.parseReleaseTag("0.1-0.0.2").orElseThrow();
        var nextMinor = ClientUpdateServiceImpl.parseReleaseTag("0.1-0.1.0").orElseThrow();

        assertTrue(first.clientVersion().compareTo(second.clientVersion()) < 0);
        assertTrue(second.clientVersion().compareTo(nextMinor.clientVersion()) < 0);
        assertFalse(ClientUpdateServiceImpl.parseReleaseTag("1-0.1.0").isPresent());
    }
}
