$ErrorActionPreference = 'Stop'
Set-Location 'C:\Users\hhhel.DESKTOP-3QIFGGI\Documents\RPCS3XBOX360'
git show 'FETCH_HEAD:LICENSE' | Set-Content -Encoding utf8 'LICENSE'
git add -A
$tree = (git write-tree).Trim()
$parent = (git rev-parse FETCH_HEAD).Trim()
$commit = (git -c user.name=heltindev -c user.email=heltindev@users.noreply.github.com commit-tree $tree -p $parent -m 'Organize Xbox 360 experimental prototype').Trim()
git update-ref refs/heads/xbox360-experimental $commit
git symbolic-ref HEAD refs/heads/xbox360-experimental
Write-Output "COMMIT=$commit"
git status --short
git push -u origin xbox360-experimental
