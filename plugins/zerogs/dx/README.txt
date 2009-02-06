ZeroGS DirectX
--------------
author: zerofrog (@gmail.com)

ZeroGS heavily uses GPU shaders. All the shaders are written in Microsoft's HLSL language and can be found in ps2hw.fx, ps2hw_ctx0.fx and ps2hw_ctx1.fx.

'Dev' versions of ZeroGS directly read ps2hw.fx
'Release' versions of ZeroGS read a precompiled version of ps2hw.fx from ps2hw.dat. In order to build ps2hw.dat, compile ZeroGSShaders and execute:

./ZeroGSShaders ps2hw.fx ps2hw.dat

For Windows users, once ZeroGSShaders is built, run buildshaders.bat directly. It will update all necessary resource files.
Note that ZeroGSShaders has only been tested in Windows so far, but the Windows ps2hw.dat can be used in linux builds.
