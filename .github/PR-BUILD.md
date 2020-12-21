## How to test a PR build

Please take into account, that RPCS3 build usually takes some time (about 20 min), so you can't access a build, if a PR was just submitted.

- Open a PR you want to test
- Scroll to the very bottom and locate "Checks" section
- Expand all checks
	You are supposed to see something like this
	![image](https://user-images.githubusercontent.com/44116740/102790039-f98f6d00-43b5-11eb-9134-2359b732508d.png)
- Click on **Linux_Build** or **Windows_Build** __Details__
- Click **View more details on Azure Pipelines** at the very bottom
	![image](https://user-images.githubusercontent.com/44116740/102790281-45daad00-43b6-11eb-90e7-0fa8c37c31c6.png)
- Click on *1 artifact produced*
	![image](https://user-images.githubusercontent.com/44116740/102790526-9a7e2800-43b6-11eb-9925-be1ea1c8bdad.png)
- Select a build for your OS and click three dots on the right, Click **Download artifacts**
	![image](https://user-images.githubusercontent.com/44116740/102790636-c9949980-43b6-11eb-9692-1e3ba567b9be.png)
	
- Congratulations! You are now downloading RPCS3 build for specific PR.

Please note, that PR builds are not supposed to be stable, because they contain new changesets. 