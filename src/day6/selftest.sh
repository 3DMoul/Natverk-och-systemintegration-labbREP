#!/usr/bin/env bash
# Full acceptance check; assumes no existing lab with these names.
set -euo pipefail
cd -- "$(dirname -- "${BASH_SOURCE[0]}")"
bash lab.sh up
trap 'bash lab.sh down' EXIT
bash lab.sh test
bash lab.sh backup
sed 's/tcp dport 1883/tcp dport 8080/' rules.nft > .state/student.nft
ip netns exec iot25-d6-router nft -c -f .state/student.nft
ip netns exec iot25-d6-router nft -f .state/student.nft
set +e
bash lab.sh test > .state/student-test.log
result=$?
set -e
cat .state/student-test.log
[[ $result == 1 ]]
[[ $(grep -c '^FAIL' .state/student-test.log) == 2 ]]
grep -q '^FAIL iot -> 1883 BLOCK expected ALLOW' .state/student-test.log
grep -q '^FAIL iot -> 8080 ALLOW expected BLOCK' .state/student-test.log
bash lab.sh restore
bash lab.sh test
bash lab.sh backup
bash lab.sh break
bash lab.sh test-broken
bash lab.sh restore
bash lab.sh test
echo 'PASS: baseline, student rule change, failure injection and restore'
