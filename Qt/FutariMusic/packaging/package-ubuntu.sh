#!/usr/bin/env bash
set -euo pipefail

script_dir="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
source_dir="$(cd -- "$script_dir/.." && pwd)"
repo_dir="$(cd -- "$source_dir/../.." && pwd)"

read_cmake_default() {
    local name="$1"
    sed -nE "s/^set\\(${name} \"([^\"]+)\" CACHE.*/\\1/p" "$source_dir/CMakeLists.txt" | head -n 1
}

client_version="${1:-${FUTARI_CLIENT_VERSION:-$(read_cmake_default FUTARI_CLIENT_VERSION)}}"
server_version="${2:-${FUTARI_SERVER_COMPAT_VERSION:-$(read_cmake_default FUTARI_SERVER_COMPAT_VERSION)}}"

if [[ ! "$client_version" =~ ^[0-9]+\.[0-9]+\.[0-9]+(-[A-Za-z0-9.-]+)?$ ]]; then
    echo "Invalid client version: $client_version" >&2
    exit 2
fi
if [[ ! "$server_version" =~ ^[0-9]+\.[0-9]+(\.[0-9]+(\.[A-Za-z][A-Za-z0-9.]*)?)?$ ]]; then
    echo "Invalid server compatibility version: $server_version" >&2
    exit 2
fi

server_line="$(sed -E 's/^([0-9]+\.[0-9]+).*/\1/' <<< "$server_version")"
build_dir="$repo_dir/build/ubuntu/cmake"
output_dir="$repo_dir/build/ubuntu"
intermediate_dir="$output_dir/cpack"
output_name="FutariMusic-${server_line}-${client_version}-ubuntu-amd64.deb"

for tool in cmake cpack dpkg-deb; do
    if ! command -v "$tool" >/dev/null 2>&1; then
        echo "Required packaging tool not found: $tool" >&2
        exit 1
    fi
done

mkdir -p "$output_dir" "$intermediate_dir"
cmake -S "$source_dir" -B "$build_dir" \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_INSTALL_PREFIX=/usr \
    -DFUTARI_CLIENT_VERSION="$client_version" \
    -DFUTARI_SERVER_COMPAT_VERSION="$server_version"
cmake --build "$build_dir" --parallel "${FUTARI_BUILD_JOBS:-4}"
cpack --config "$build_dir/CPackConfig.cmake" -G DEB -B "$intermediate_dir"

debian_version="$(sed 's/-/~/g' <<< "$client_version")"
package_path="$intermediate_dir/futari-music_${debian_version}_amd64.deb"
if [[ ! -f "$package_path" ]]; then
    echo "Expected Debian package was not created: $package_path" >&2
    exit 1
fi

cp -- "$package_path" "$repo_dir/build/$output_name"
dpkg-deb --info "$repo_dir/build/$output_name" >/dev/null
echo "Created $repo_dir/build/$output_name"
