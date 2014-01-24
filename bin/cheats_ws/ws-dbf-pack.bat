%~d0
cd %~p0
set db=cheats_ws.dbf
del %db%

@echo off
echo Creating %db%
for %%f in (*.pnach) do (
  echo Processing %%f
  echo //IMPORT-START:%%f>> %db%
  echo [patches=%%~nf]>> %db%
  type %%f>> %db%
  echo.>> %db%
  echo [/patches]>> %db%
  echo //IMPORT-END:%%f>> %db%
  echo.>> %db%
  echo.>> %db%
)
