## How to test a PR build

Please take into account, that RPCS3 build usually takes some time (about 15 mins), so you can't access a build if a PR was just submitted.

- Open a PR you want to test
- Scroll to the very bottom and locate the **Checks** section
- Click on **Show all checks**
	You are supposed to see something like this
	![image](https://user-images.githubusercontent.com/10283761/116630952-2cd99e00-a94c-11eb-933e-986d6020ca92.png)
- Click on __Details__ on either **Cirrus Linux GCC** or **Cirrus Windows**
- Click **View more details on Cirrus CI** at the very bottom
	![image](https://user-images.githubusercontent.com/10283761/116631111-5e526980-a94c-11eb-95f7-751e6f15e1ea.png)
- Click on the download button for **Artifact** on the **Artifacts** block
	![image](https://user-images.githubusercontent.com/10283761/116631322-bee1a680-a94c-11eb-89a3-be365783582e.png)

- Congratulations! You are now downloading an RPCS3 build for that specific PR.

__Please note that PR builds are not supposed to be stable because they contain new changesets.__
