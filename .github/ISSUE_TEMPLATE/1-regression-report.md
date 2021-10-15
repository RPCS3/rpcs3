---
name: Regression report
about: If something used to work before, but now it doesn't
title: ''
labels: ''
assignees: ''

---

## Please do not ask for help or report compatibility regressions here, use [RPCS3 Discord server](https://discord.me/RPCS3) or [forums](https://forums.rpcs3.net/) instead.

## Quick summary
Please briefly describe what has stopped working correctly.

## Details
Please describe the regression as accurately as possible.

#### 0. Make sure you're running with settings as close to default as possible
* **Do NOT enable any emulator game patches when reporting issues**
* Only change settings that are required for the game to work

#### 1. Please provide _exact_ build (or commit) information that introduced the regression you're reporting.
* Please see [How to find the build that caused a regression](https://wiki.rpcs3.net/index.php?title=Help:Using_different_versions_of_RPCS3#How_to_find_the_build_that_caused_a_regression.3F) in our wiki.
* You can find [History of RPCS3 builds](https://rpcs3.net/compatibility?b) here.

#### 2. Please attach TWO logs:
* One for the build with regression.
* One for the build that works as expected.

##### How to obtain RPCS3's log:
* Run the game until you find the regression.
* Completely close RPCS3, or move to the next step in case it has crashed.
* Locate RPCS3's log file:
	+ ```RPCS3.log``` (It can show up as just RPCS3 and have a notepad icon)
	or
	+ ```RPCS3.log.gz``` (It can show up as RPCS3.log and have a zip or rar icon)
![image](https://user-images.githubusercontent.com/44116740/84433247-aa15fa80-ac36-11ea-913e-6fe25a1d040e.png)
	On Linux it will be in ```~/.cache/rpcs3/```.
	On Windows it will be near the executable.
* Attach the log:
	+ Drag and drop your log file into the issue.
	+ Or upload it to the cloud, such as Dropbox, Mega etc.

#### 3. If you describe graphical regression, please provide an RSX capture and a RenderDoc capture that demonstrate it.
* To create an RSX capture, use _Create_ _RSX_ _Capture_ under _Utilities_.
Captures will be stored in RPCS3 folder → captures.
	+ Compress your capture with 7z, Rar etc.
	And drag and drop it into the issue.
	+ Or upload it to the cloud, such as Dropbox, Mega etc.
* To create a RenderDoc capture, please refer to [RenderDoc's documentation](https://renderdoc.org/docs/how/how_capture_frame.html).

#### 4. Please attach screenshots of your problem.
* Enable performance overlay with at least medium level of detail.
You can find it in _Emulator_ tab in _Settings_.

#### 5. Please provide comparison with real PS3.

#### 6. Please provide your system configuration:
* OS
* CPU
* GPU
* Driver version
* etc.

#### Please include.
* Anything else you deem to be important
