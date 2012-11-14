if exist ..\include\wx\msw\setup.h (
    echo include\wx\msw\setup.h already exists
) else (
    copy /y ..\include\wx\msw\setup0.h ..\include\wx\msw\setup.h
)

if exist ..\lib\cw7mswd (
    echo lib\cw7mswd already exists
) else (
    mkdir ..\lib\cw7mswd
)

if exist ..\lib\cw7mswd\include (
    echo lib\cw7mswd\include already exists
) else (
    mkdir ..\lib\cw7mswd\include
)

if exist ..\lib\cw7mswd\include\wx (
    echo lib\cw7mswd\include\wx already exists
) else (
    mkdir ..\lib\cw7mswd\include\wx
)

if exist ..\lib\cw7mswd\include\wx\setup.h (
    echo lib\cw7mswd\include\wx\setup.h already exists
) else (
    copy /y ..\include\wx\msw\setup.h ..\lib\cw7mswd\include\wx\setup.h
)

rem pause

