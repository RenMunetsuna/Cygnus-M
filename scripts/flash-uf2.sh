#!/usr/bin/env bash
set -euo pipefail

usage() {
  cat <<'USAGE'
Usage:
  scripts/flash-uf2.sh --side right --run latest
  scripts/flash-uf2.sh --side left --run 26284225454
  scripts/flash-uf2.sh --firmware path/to/Cygnus_R.uf2

Options:
  --side right|left|reset    Select firmware from a downloaded/build artifact.
  --run RUN_ID|latest        Download artifacts from a GitHub Actions run.
  --repo OWNER/REPO          GitHub repo. Defaults to origin or RenMunetsuna/Cygnus-M.
  --dir DIR                  Artifact directory. Defaults to artifacts/run-RUN_ID.
  --firmware FILE            Use an explicit UF2 file instead of selecting by side/run.
  --wait SECONDS             Wait for a UF2 bootloader volume. Default: 60.
  -h, --help                 Show this help.
USAGE
}

repo_from_git() {
  local url
  url="$(git config --get remote.origin.url 2>/dev/null || true)"
  case "$url" in
    git@github.com:*)
      url="${url#git@github.com:}"
      printf '%s\n' "${url%.git}"
      ;;
    https://github.com/*)
      url="${url#https://github.com/}"
      printf '%s\n' "${url%.git}"
      ;;
    *)
      printf '%s\n' "RenMunetsuna/Cygnus-M"
      ;;
  esac
}

firmware_pattern_for_side() {
  case "$1" in
    right|r)
      printf '%s\n' 'Cygnus_R*.uf2'
      ;;
    left|l)
      printf '%s\n' 'Cygnus_L*.uf2'
      ;;
    reset|settings_reset)
      printf '%s\n' 'settings_reset*.uf2'
      ;;
    *)
      printf 'Unknown side: %s\n' "$1" >&2
      exit 2
      ;;
  esac
}

find_uf2_volumes() {
  local volume
  for volume in /Volumes/*; do
    [ -d "$volume" ] || continue
    if [ -e "$volume/INFO_UF2.TXT" ] || [ -e "$volume/INDEX.HTM" ]; then
      printf '%s\n' "$volume"
    fi
  done
}

wait_for_uf2_volume() {
  local wait_seconds="$1"
  local deadline=$((SECONDS + wait_seconds))
  local volumes
  local count

  while [ "$SECONDS" -le "$deadline" ]; do
    volumes="$(find_uf2_volumes || true)"
    count="$(printf '%s\n' "$volumes" | sed '/^$/d' | wc -l | tr -d ' ')"
    if [ "$count" = "1" ]; then
      printf '%s\n' "$volumes" | sed -n '1p'
      return 0
    fi
    if [ "$count" -gt 1 ]; then
      printf 'Multiple UF2 volumes found. Unmount extras and retry:\n%s\n' "$volumes" >&2
      return 1
    fi
    sleep 1
  done

  printf 'No UF2 bootloader volume found under /Volumes within %ss.\n' "$wait_seconds" >&2
  return 1
}

repo="$(repo_from_git)"
side=""
run_id=""
artifact_dir=""
firmware=""
wait_seconds=60

while [ "$#" -gt 0 ]; do
  case "$1" in
    --side)
      side="${2:-}"
      shift 2
      ;;
    --run)
      run_id="${2:-}"
      shift 2
      ;;
    --repo)
      repo="${2:-}"
      shift 2
      ;;
    --dir)
      artifact_dir="${2:-}"
      shift 2
      ;;
    --firmware)
      firmware="${2:-}"
      shift 2
      ;;
    --wait)
      wait_seconds="${2:-}"
      shift 2
      ;;
    -h|--help)
      usage
      exit 0
      ;;
    *)
      printf 'Unknown option: %s\n' "$1" >&2
      usage >&2
      exit 2
      ;;
  esac
done

if [ -z "$firmware" ]; then
  if [ -z "$side" ]; then
    printf 'Missing --side when --firmware is not specified.\n' >&2
    usage >&2
    exit 2
  fi

  if [ -z "$run_id" ] || [ "$run_id" = "latest" ]; then
    run_id="$(gh run list --repo "$repo" --workflow build.yml --branch main --status success \
      --limit 1 --json databaseId --jq '.[0].databaseId')"
    if [ -z "$run_id" ] || [ "$run_id" = "null" ]; then
      printf 'Could not find a successful build.yml run for %s.\n' "$repo" >&2
      exit 1
    fi
  fi

  artifact_dir="${artifact_dir:-artifacts/run-$run_id}"
  mkdir -p "$artifact_dir"

  if ! find "$artifact_dir" -type f -name '*.uf2' | grep -q .; then
    gh run download "$run_id" --repo "$repo" --dir "$artifact_dir"
  fi

  pattern="$(firmware_pattern_for_side "$side")"
  firmware="$(find "$artifact_dir" -type f -name "$pattern" | sort | sed -n '1p')"
  if [ -z "$firmware" ]; then
    printf 'No firmware matching %s found in %s.\n' "$pattern" "$artifact_dir" >&2
    exit 1
  fi
fi

if [ ! -f "$firmware" ]; then
  printf 'Firmware file does not exist: %s\n' "$firmware" >&2
  exit 1
fi

printf 'Firmware: %s\n' "$firmware"
printf 'Put the target half into bootloader mode now.\n'
volume="$(wait_for_uf2_volume "$wait_seconds")"

printf 'Copying to %s...\n' "$volume"
cp -X "$firmware" "$volume/"
sync
printf 'Done.\n'
