

# **OdinFleet AWS Gamelift Integration**

This guide explains how to run an Unreal Engine dedicated server on [Odin Fleet](https://odin.4players.io/fleet/), a hosting service for scalable dedicated game servers. By integrating the server with Amazon GameLift Anywhere, the setup enables GameLift features such as matchmaking, session discovery, and lifecycle management while Odin Fleet provides the underlying compute resources.

To demonstrate this integration, the repository includes an example Unreal project for building both a game client and a dedicated Linux server. It also provides a Docker directory containing everything needed to containerize the server for Odin Fleet: a Dockerfile, an entry script for launching the GameLift Agent, the gameliftagent.jar, and a minimal runtime configuration. Together, these components form a complete template for deploying an Unreal server on Odin Fleet and connecting it to GameLift’s backend services.

A matching backend implementation for session management is available here:
https://github.com/unterpunkt9/Odin_Gamelift_BackendService

## Requirements:
* **Unreal Engine Dedicated Server Build (Linux)**: Needed to run the game server inside the container on Odin Fleet.
* **Amazon GameLift Server SDK**: Enables the server to communicate with GameLift Anywhere for session activation, health checks, and lifecycle events.
* **Amazon GameLift Server Agent**: Handles compute registration, authentication, and process management for GameLift Anywhere inside the container.
* **Docker**: Used to build the container image that will be deployed on Odin Fleet.
* **Odin Fleet Account**: Required to host and run the containerized dedicated server. The integration with GameLift Anywhere enables matchmaking and game session discovery.

## Dedicated Game Server
If you don't already have one, create an Unreal Engine dedicated server build. A full instruction can be found [here](https://dev.epicgames.com/documentation/en-us/unreal-engine/setting-up-dedicated-servers?application_version=4.27).
The example is for 4.27 but the steps are the same in 5.6.

## GameLift Server SDK
To connect the Gameserver with Amazon Gamelift, the server needs the Gamelift Server SDK. The different versions can be found [here](https://docs.aws.amazon.com/gameliftservers/latest/developerguide/reference-serversdk.html).
We are using the [C++ SDK](https://github.com/amazon-gamelift/amazon-gamelift-plugin-unreal) for Unreal. Download and Build the SDK.

Linux or Mac:
```bash
chmod +x setup.sh
sh setup.sh
```
Windows
```bash
powershell -file setup.ps1
```

When this is done, copy the sdk to your plugins inside the Unreal Project. There are two folders you can use.
 * GameLiftServerSDK: Just contains the server SDK
 * GameLiftPlugin: Includes the Server SDK and additional UI components for the Editor.

Just copy the folder that fits your need the most.
Then add the Plugin to your PublicDependencyModules in your <projectName>.Build.cs

```c++
    if (Target.Type == TargetType.Server)
    {
        PublicDependencyModuleNames.Add("GameLiftServerSDK");
    }else{
        PublicDefinitions.Add("WITH_GAMELIFT=0");
    }
    bEnableExceptions =  true;
```

The next step is to initiate the SDK in the server. To do that, add the following code to your GameMode implementation:
```c++
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "YourGameMode.generated.h"

struct FProcessParameters;

DECLARE_LOG_CATEGORY_EXTERN(GameServerLog, Log, All);

UCLASS(minimalapi)
class AYourGameMode : public AGameModeBase
{
    GENERATED_BODY()

public:
    AYourGameMode();

protected:
    virtual void BeginPlay() override;

private:
    void InitiateGameLift();

private:
    TSharedPtr<FProcessParameters> ProcessParameters;
}; 
```
and implement the ``InitiateGameLift`` function.

Add the SDK includes and instantiate the ProcessParameters:

```c++
#include "YourGameMode.h"

#include "UObject/ConstructorHelpers.h"
#include "Kismet/GameplayStatics.h"

#if WITH_GAMELIFT
#include "GameLiftServerSDK.h"
#include "GameLiftServerSDKModels.h"
#endif

#include "GenericPlatform/GenericPlatformOutputDevices.h"
DEFINE_LOG_CATEGORY(GameServerLog);

AYourGameMode::AYourGameMode() : ProcessParameters(nullptr)
{
  ... // Your constructor code
}
```

Implement `InitiateGameLift`:

```c++
void AYourGameMode::InitiateGameLift()
{
    #if WITH_GAMELIFT
    UE_LOG(GameServerLog, Log, TEXT("Calling InitGameLift..."));
    FGameLiftServerSDKModule* GameLiftSdkModule = &FModuleManager::LoadModuleChecked<FGameLiftServerSDKModule>(FName("GameLiftServerSDK"));

    UE_LOG(GameServerLog, Log, TEXT("Initializing the GameLift Server..."));
    // InitSDK will establish a local connection with the gamelift agent to enable further communication.
    FGameLiftGenericOutcome InitSdkOutcome = GameLiftSdkModule->InitSDK();

        if (InitSdkOutcome.IsSuccess())
    {
        UE_LOG(GameServerLog, SetColor, TEXT("%s"), COLOR_GREEN);
        UE_LOG(GameServerLog, Log, TEXT("GameLift InitSDK succeeded!"));
        UE_LOG(GameServerLog, SetColor, TEXT("%s"), COLOR_NONE);
    }
    else
    {
        UE_LOG(GameServerLog, SetColor, TEXT("%s"), COLOR_RED);
        UE_LOG(GameServerLog, Log, TEXT("ERROR: InitSDK failed : ("));
        FGameLiftError GameLiftError = InitSdkOutcome.GetError();
        UE_LOG(GameServerLog, Log, TEXT("ERROR: %s"), *GameLiftError.m_errorMessage);
        UE_LOG(GameServerLog, SetColor, TEXT("%s"), COLOR_NONE);
        return;
    }

        ProcessParameters = MakeShared<FProcessParameters>();

    // When a game session is created, Amazon GameLift sends an activation request to the game server and 
    // passes along the game session object containing game properties and other settings.
    // Here is where your game server should take action based on the game session object.
    // Once the game server is ready to receive incoming player connections, it should invoke GameLiftServerAPI.ActivateGameSession()
    ProcessParameters->OnStartGameSession.BindLambda([=](Aws::GameLift::Server::Model::GameSession InGameSession)
        {
            FString GameSessionId = FString(InGameSession.GetGameSessionId());
            UE_LOG(GameServerLog, Log, TEXT("GameSession Initializing: %s"), *GameSessionId);
            GameLiftSdkModule->ActivateGameSession();
        });

    // OnProcessTerminate callback. Amazon GameLift Servers will invoke this callback before shutting down an instance hosting this game server.
    // It gives this game server a chance to save its state, communicate with services, etc., before being shut down.
    // In this case, we simply tell Amazon GameLift Servers we are indeed going to shutdown.
    ProcessParameters->OnTerminate.BindLambda([=]()
	{
		UE_LOG(GameServerLog, Log, TEXT("Game Server Process is terminating"));
		FGameLiftGenericOutcome processEndingOutcome = GameLiftServerSdkModule->ProcessEnding();

		FGameLiftGenericOutcome destroyOutcome = GameLiftServerSdkModule->Destroy();
		if (processEndingOutcome.IsSuccess() && destroyOutcome.IsSuccess())
		{
			UE_LOG(GameServerLog, Log, TEXT("Server process ending successfully"));
			FGenericPlatformMisc::RequestExit(false); //Important, otherwise it could remain an open process, that blocks the used game server port
		}else{
			if (!processEndingOutcome.IsSuccess()) {
				const FGameLiftError& error = processEndingOutcome.GetError();
				UE_LOG(GameServerLog, Error, TEXT("ProcessEnding() failed. Error: %s"),
				error.m_errorMessage.IsEmpty() ? TEXT("Unknown error") : *error.m_errorMessage);
			}
			if (!destroyOutcome.IsSuccess()) {
				const FGameLiftError& error = destroyOutcome.GetError();
				UE_LOG(GameServerLog, Error, TEXT("Destroy() failed. Error: %s"),
				error.m_errorMessage.IsEmpty() ? TEXT("Unknown error") : *error.m_errorMessage);
			}
		}
	});
    // This is the HealthCheck callback.
    // Amazon GameLift Servers will invoke this callback every 60 seconds.
    // Here, a game server might want to check the health of dependencies and such.
    // Simply return true if healthy, false otherwise.
    // The game server has 60 seconds to respond with its health status. 
    // Amazon GameLift Servers will default to 'false' if the game server doesn't respond in time.
    // In this case, we're always healthy!
    ProcessParameters->OnHealthCheck.BindLambda([]()
        {
            UE_LOG(GameServerLog, Log, TEXT("Performing Health Check"));
            return true;
        });

    //GameServer.exe -port=7777 LOG=server.mylog
    ProcessParameters->port = FURL::UrlConfig.DefaultPort;
    TArray<FString> CommandLineTokens;
    TArray<FString> CommandLineSwitches;

    FCommandLine::Parse(FCommandLine::Get(), CommandLineTokens, CommandLineSwitches);

    for (FString SwitchStr : CommandLineSwitches)
    {
        FString Key;
        FString Value;

        if (SwitchStr.Split("=", &Key, &Value))
        {
            if (Key.Equals("port"))
            {
                ProcessParameters->port = FCString::Atoi(*Value);
            }
        }
    }
    //Here, the game server tells Amazon GameLift Servers where to find game session log files.
    //At the end of a game session, Amazon GameLift Servers uploads everything in the specified 
    //location and stores it in the cloud for access later.
    TArray<FString> Logfiles;
    Logfiles.Add(TEXT("GameLiftUnrealApp/Saved/Logs/server.log"));
    ProcessParameters->logParameters = Logfiles;

    //The game server calls ProcessReady() to tell Amazon GameLift Servers it's ready to host game sessions.
    UE_LOG(GameServerLog, Log, TEXT("Calling Process Ready..."));
    FGameLiftGenericOutcome ProcessReadyOutcome = GameLiftSdkModule->ProcessReady(*ProcessParameters);

    if (ProcessReadyOutcome.IsSuccess())
    {
        UE_LOG(GameServerLog, SetColor, TEXT("%s"), COLOR_GREEN);
        UE_LOG(GameServerLog, Log, TEXT("Process Ready!"));
        UE_LOG(GameServerLog, SetColor, TEXT("%s"), COLOR_NONE);
    }
    else
    {
        UE_LOG(GameServerLog, SetColor, TEXT("%s"), COLOR_RED);
        UE_LOG(GameServerLog, Log, TEXT("ERROR: Process Ready Failed!"));
        FGameLiftError ProcessReadyError = ProcessReadyOutcome.GetError();
        UE_LOG(GameServerLog, Log, TEXT("ERROR: %s"), *ProcessReadyError.m_errorMessage);
        UE_LOG(GameServerLog, SetColor, TEXT("%s"), COLOR_NONE);
    }

    UE_LOG(GameServerLog, Log, TEXT("InitGameLift completed!"));
    #endif
}
```

And call it in ``BeginPlay``
```c++
void AOdinFleetGameMode::BeginPlay()
{
	Super::BeginPlay();

#if WITH_GAMELIFT
	InitGameLift();
	#endif
}
```

You dont need to initalize and set any parameter used for AnywhereFleets. These get handled by the GameLift Server Agent.
  If your're interested in doing that manually, there is a guide inside the `README.md` in the plugin folder.

The minimum the game server has to do is:
* Call `InitSDK()`
* Implement GameSessionCallbacks (`OnStartGameSession`, `OnTerminate` and`OnHealthCheck`)
* Set the used port.
* Call `ProcessReady()`


## GameLift Server Agent

The GameLift Server Agent handles all neccessary steps to connect your game server with GameLift:

 * Reads the AccessKey of your AWS Account
 * Registers a compute device
 * Reads and refreshes the auth-token for the compute device
 * Starts game servers with preconfigured parameters
 * Manages the heartbeat

### AWS account and User Permissions

To allow the GameLift Agent to register your compute device and communicate with GameLift Anywhere, you must set up an IAM user with the correct permissions.

1. In the AWS Console, open IAM and create a new user.
2. Under Security Credentials, create a new Access Key (Security credentials > Create access key) and store it safely.
3. This user must have permissions for the tasks the GameLift Agent performs:
   - gamelift:RegisterCompute
   - gamelift:GetComputeAuthToken
   - gamelift:DeregisterCompute

The easiest way is to create a custom policy that includes these actions.
Go to IAM > Policies, create a new policy, and add the required permissions either via the visual editor or by switching to the JSON view to paste the following policy definition:
```
{
	"Version": "2012-10-17",
	"Statement": [
		{
			"Sid": "AllowRegisterComputeOnFleet",
			"Effect": "Allow",
			"Action": "gamelift:RegisterCompute",
			"Resource": "arn:aws:gamelift:<your-region>:<your-account-id>:fleet/<your-fleet-id>"
		},
		{
			"Sid": "DeregisterCompute",
			"Effect": "Allow",
			"Action": "gamelift:DeregisterCompute",
			"Resource": "arn:aws:gamelift:<your-region>:<your-account-id>:fleet/<your-fleet-id>"
		},
		{
			"Sid": "GetAuthTokenForFleet",
			"Effect": "Allow",
			"Action": "gamelift:GetComputeAuthToken",
			"Resource": "arn:aws:gamelift:<your-region>:<your-account-id>:fleet/<your-fleet-id>"
		},
	]
}
```
Click Next > Save Changes and go back to the user. Add the created policy and the user is ready to be used by the agent.

### Gamelift Location and Fleet
Navigate to [Amazon Gamlift Servers](https://eu-central-1.console.aws.amazon.com/gameliftservers/dashboard) and create a custom Location for your AnywhereFleet.
![Location](Documentation/Location.jpg)

Then create an AnywhereFleet
![Fleet](Documentation/Fleet.jpg)

Give it an name and select your custom location.

 
### Initiate the Agent
In this step, you prepare and build the GameLift Agent, then provide it with a runtime configuration so it knows how to start your dedicated server inside the container.

Clone the repository from the [Github page](https://github.com/amazon-gamelift/amazon-gamelift-agent).

To build the agent you need at least Java JDK 17 and maven 3.2.5.
You can verify that the required versions are installed by calling:

Java:
```bash
java -version
```

Maven:
```bash
mvn -version
```

If you're missing the requirements or have outdated versions, you can install the requirements at:
* [Java](https://www.oracle.com/java/technologies/javase/jdk17-archive-downloads.html)
* [Maven](https://maven.apache.org/download.cgi)

When both match the minimum version, open a new terminal/cmd in the root of the agent(where the pom.xml is located) and build the Agent
```bash
mvn clean compile assembly:single
```
If it is successfull, the .jar file is located at `./target/GameLiftAgent-1.0.jar`. This `.jar` file will be the entrypoint of the Docker image which is created later.

The agent needs to know where the server executable is located. This information is provided by a runtime-config.json.
This is structured as followed:
 ```json
 {
  "ServerProcesses": [
    {
      "LaunchPath": "/local/game/<server-executable>",
      "Parameters": "server parameter",
      "ConcurrentExecutions": 1
    }
  ],
  "MaxConcurrentGameSessionActivations": 1,
  "GameSessionActivationTimeoutSeconds": 300
}
 ```

**IMPORTANT** On Windows the LaunchPath is required to start with 
`C:/Game/`,
on Linux with `/local/game/`. Using a launch path that starts differently will not work with the default gamelift agent code.

For more information, please take a look at the [gamelift agent runtime configuration documentation](https://docs.aws.amazon.com/gameliftservers/latest/apireference/API_RuntimeConfiguration.html).

## Docker
Odin Fleet deploys your dedicated server as a Docker image, so you need a working Docker environment to build and package the server.
Install Docker Desktop using the official installer:

https://docs.docker.com/desktop/setup/install/windows-install/

Docker Desktop requires the Windows Subsystem for Linux (WSL). If it is not enabled yet, you can activate it through the command prompt:

```bash
wsl --install
```

Once Docker and WSL are installed, you can build the image that will later run on Odin Fleet.

### Unreal Server Build

 For the Docker image you are creating you need a Linux build of the game server. To do that you need to install the Linux Cross-Compile chain. Please take a look at the [Unreal Guide](https://dev.epicgames.com/documentation/en-us/unreal-engine/linux-development-requirements-for-unreal-engine?application_version=5.0) to setup the cross-compilation chain. You can verify the installation in a terminal by calling:

```echo %LINUX_MULTIARCH_ROOT%```

Now build the server-executable. Open the Editor and package the project using the `Server` Build Target.

![Editor Package](Documentation/Package.jpg)

You can also package the server using the following command prompt:
```bash
<path-to-source-built-engine>/Engine/Build/BatchFiles/RunUAT.bat BuildCookRun ^
 -project="<path-to-your-project>/<YourProject>.uproject" ^
 -noP4 -server -platform=Linux -clientconfig=Shipping -serverconfig=Shipping ^
 -cook -allmaps -build -stage -pak -archive ^
 -archivedirectory="<Your-package-folder>"
```
### Preparing the Docker image
Create a folder (e.g., DockerImageData) that contains all files required to build the server image. Your file structure should look like this:

```
/DockerImageData/
    /<Your-package-folder>  # Your packaged Unreal Linux server build
    /GameLiftAgent-1.0.jar  # The built GameLift Agent
    /runtime-config.json    # Runtime configuration for the agent
```
Inside this folder, create a Dockerfile. This file defines how the image is built, copies all necessary files into the container, and sets the entrypoint, the script or executable that runs when the container starts.

Dockerfile:
```docker
FROM ghcr.io/epicgames/unreal-engine:runtime

USER root

# install java to run the agent.jar this need root user permissions
RUN apt-get update && apt-get install -y --no-install-recommends openjdk-17-jre-headless \
 && rm -rf /var/lib/apt/lists/*
# switch to ue4 user. Some executions cant be made as root
USER ue4
# Put game under /local/game and make ue4 the owner
WORKDIR /local/game
COPY --chown=ue4:ue4 LinuxServer/ /local/game/

# copy Agent and entrypoint; also owned by ue4
COPY --chown=ue4:ue4 GameLiftAgent-1.0.jar /gamelift/agent.jar
COPY --chown=ue4:ue4 entrypoint.sh /entrypoint.sh

# create log folders and set permissions for user ue4
RUN chmod 0755 /entrypoint.sh \
 && mkdir -p /local/game/Saved/Logs /local/game/logs /local/gameliftagent \
 && chown -R ue4:ue4 /local /gamelift


ENTRYPOINT ["/entrypoint.sh"]
```

We are using a script as entrypoint to start the agent and pass required parameters.
Create a file `entrypoint.sh` inside your docker file folder.

entrypoint.sh:
```sh
#!/usr/bin/env bash
set -euo pipefail

# If COMPUTE_NAME isn't provided, generate one
if [[ -z "${COMPUTE_NAME:-}" || "${COMPUTE_NAME}" == "auto" ]]; then
  # /proc/sys/kernel/random/uuid exists by default and doesn't need uuidgen
  RAND_ID="$(cat /proc/sys/kernel/random/uuid)"
  # Optional: prefix to recognize the node in AWS console
  COMPUTE_NAME="odin-$(echo "$RAND_ID" | tr '[:upper:]' '[:lower:]')"
fi
: "${REGION:?Set REGION}"
export AWS_REGION="${AWS_REGION:-$REGION}"
: "${FLEET_ID:?Set FLEET_ID}"
: "${LOCATION:?Set LOCATION}"
: "${REGION:?Set REGION}"
: "${PUBLIC_IP:?Set PUBLIC_IP}"

exec java -jar /gamelift/agent.jar -c "${COMPUTE_NAME}" -f "${FLEET_ID}" -loc "${LOCATION}" -r "${REGION}" -ip-address "${PUBLIC_IP}"
```

This script reads the enviromnent variables and passes them to the agent.
The compute name is generated randomly, because when the agent is closed, the registered compute device remains in the state `TERMINATING` for about 1-3 days until AWS sets it back to ACTIVE. When you try to register an existing Compute-name that is in the `TERMINATING` state, the register fails and the agent cannot succesfull connect to an GameLift compute device.

### Runtime Config

You can either upload the config to AWS or copy it to the image and set the path in the agent.jar call.

To upload it using the [AWS-CLI](https://docs.aws.amazon.com/cli/latest/userguide/getting-started-install.html), call:

```sh
aws gamelift update-runtime-configuration --<your-fleet-id> --runtime-configuration file://<path-to-config>/runtime-config.json --region <your-region>
```

and the agent loads it automatically.

If you want to set it manually, add `-runtime-configuration` or `-rc` as a parameter to the `exec java -jar /gamelift/agent.jar` call and pass the config as inline JSON.

The Unreal Linux build contains an `GameServer.sh` in its root directory. This script calls the server executable which is located in `\LinuxServer\<your-project-name>\Binaries\Linux`.
We use that .sh scrip as the `LaunchPath` value in the runtime-config. 
Inside the `GameServer.sh` script we can pass the external port of the OdinFleet server. 

You can use this sample `GameServer.sh` to automatically read the external Odin Fleet port from the environment variables:
```sh
#!/bin/sh
UE_TRUE_SCRIPT_NAME=$(echo \"$0\" | xargs readlink -f)
UE_PROJECT_ROOT=$(dirname "$UE_TRUE_SCRIPT_NAME")
chmod +x "$UE_PROJECT_ROOT/<your-project-name>/Binaries/Linux/<your-project-executable>"

PORT_ARG=""
if [ -n "${EXTERNAL_PORT:-}" ]; then
  PORT_ARG="-port=${EXTERNAL_PORT}"
fi

"$UE_PROJECT_ROOT/<your-project-name>/Binaries/Linux/<your-project-executable>" <your-project-name> "$@" $PORT_ARG
```
**IMPORTANT: Please keep in mind that this file will be overridden if your repackage your project, you will need to update this every time you create a new game server build or adjust your Build Scripts accordingly.**

Now we can build the docker image.

### Building the Docker Image

Open a command prompt in the directory where the Dockerfile is located and call:

```sh
docker build -t <your-image-name>:<your-image-tag> .
```

You can now test the image locally on your PC using Docker Desktop. Navigate to image, locate your server image and click run. Now you need to pass the environment variables and set the port.

The following variables are required when using the example setup:
* FLEET-ID
* LOCATION
* REGION
* PUBLIC_IP (IP address of the Odin Fleet server)
* EXTERNAL_PORT

These are used by the Agent internally:
* AWS_ACCESS_KEY_ID
* AWS_SECRET_ACCESS_KEY

If everything works locally you can tag and push this image to [Docker Hub](https://hub.docker.com/). 

```sh
docker tag <your-image-name>:<image-tag> <docker-username>/<your-image-name>:<release-tag>  
# release tag can use any versioning scheme

docker push <docker-username>/<your-image-name>:<release-tag>
```

## OdinFleet
The final step is to load the image on an Odin Fleet server.
You can follow the guide steps for creating a Fleet App ([Minecraft-server example](https://docs.4players.io/fleet/guides/getting-started/) or [Unreal game server example](https://docs.4players.io/fleet/guides/unreal-server/))

### Server Config

Create a port in the Port Settings as mentioned in the referenced guides

![Create Port](Documentation/Port.jpg)

Now add a Dynamic Variable to the Environment Variables

![Setting dynamic variables](Documentation/DynamicVar.jpg)

This supplies the EXTERNAL_PORT of the Odin Fleet server to the game server and is transmitted to Gamelift.

This also adds a portmapping. The dockerimage gets this port as image port.
```
ServerPort: 12345
Image uses 12345 as Imageport
Mapping:
12345->12345
```


## Backend Service

To avoid storing an AWS Access Key inside the client, the client itself cannot communicate with AWS to retrieve game session data. This communication should be done with a custom Game Backend Service. Usually this is some kind of a REST-API.

The client communicates with this Game Backend Service, which uses AWS Acces sKeys in a secure environment. 
To build this API you need the [AWS GameLift client sdk](https://docs.aws.amazon.com/AWSJavaScriptSDK/v3/latest/Package/-aws-sdk-client-gamelift/).

Install it using:

**NPM**: `npm install @aws-sdk/client-gamelift`

**Yarn**: `yarn add @aws-sdk/client-gamelift`

**pnpm**: `pnpm add @aws-sdk/client-gamelift`

Please keep in mind, that you can use any kind of Node.js/web-service of your choice. In this example we are using GoogleCloud Run functions. We are not going deep into the initialization of an GoogleCloud project, just know these act the same as any other https endpoint. The source code for the Cloud functions can be found in the [Backend Service Github repository](https://github.com/unterpunkt9/Odin_Gamelift_BackendService).

The base setup of the AWS client sdk is as follows:
* Create an input object
* Create the required command
* Pass the input object
* Execute the command

First include the sdk, set some variables and create a GameLiftClient:
```js
const {onRequest} = require("firebase-functions/v2/https");
const {GameLiftClient, SearchGameSessionsCommand, CreateGameSessionCommand, TerminateGameSessionCommand} = require('@aws-sdk/client-gamelift');

const FleetID = "<your-fleet-id>";
const Location = "your-location"; // the custom aws fleet location 
const AWSRegion = "your-aws-region"; //example: eu-central-1
const GCloudRegion = "your-gcloud-region";//example: europe-west3

//Create the GameliftClient
const gameLiftClient = new GameLiftClient({
    region: AWSRegion,
    credentials:{
        accessKeyId:"<your-access-key-id>",
        secretAccessKey:"<your-access-key>"
    }
});
```

**Note**: You can and should store your AWS Credentials in a credentials-file, environment-variable or another secure storage.

Now you can add the needed commands.

Example implementation for [Search for Game Sessions](https://docs.aws.amazon.com/AWSJavaScriptSDK/v3/latest/Package/-aws-sdk-client-gamelift/Class/SearchGameSessionsCommand/) logic:
```js
exports.<your-function-name> = onRequest({region:GCloudRegion},async(req,res)=>{

    const SearchInput = {
        FleetId:FleetID,
        Location:Location,
    };
    const command = new SearchGameSessionsCommand(SearchInput);
    await executeCommand(res,command);
    return;
});

async function executeCommand(res,command){
    try {
        const response = await gameLiftClient.send(command);
        res.status(200).send(response); 
        return;
    } catch (error) {
        console.error(error);
        res.status(400).send(error);
        throw error;
    }
}
```
Example implementation for [Create Game Sessions](https://docs.aws.amazon.com/AWSJavaScriptSDK/v3/latest/Package/-aws-sdk-client-gamelift/Class/CreateGameSessionCommand/) logic:
```js
exports.<your-function-name> = onRequest({region:GCloudRegion},async (req,res)=>{
    if(req.body.CreatorId === undefined){
        res.status(401).send("Missing CreatorId");
        return;
    }
    if(req.body.SessionName === undefined){
        res.status(401).send("Missing SessionName");
        return;
    }

    const input = {
        FleetId: FleetID,
        Location:Location,
        CreatorId:req.body.CreatorId,
        Name:req.body.SessionName,
        MaximumPlayerSessionCount:Number(2),
    };
    const command = new CreateGameSessionCommand(input);
    await executeCommand(res,command);
    return;
});
```
Example implementation for [Terminate Game Sessions](https://docs.aws.amazon.com/AWSJavaScriptSDK/v3/latest/Package/-aws-sdk-client-gamelift/Class/TerminateGameSessionCommand/) logic:
```js
exports.<your-function-name> = onRequest({region:GCloudRegion},async (req, res) =>{

    if(req.body.GameSessionId === undefined){
        res.status(401).send("Missing GameSessionId");
        return;
    }

    const input = { // TerminateGameSessionInput
        GameSessionId: req.body.GameSessionId, // required
        TerminationMode: "TRIGGER_ON_PROCESS_TERMINATE", // required
    };
    const command = new TerminateGameSessionCommand(input);
    await executeCommand(res,command);
});
```
**Note**: These examples don't use any authorization logic. To secure your service against unwanted or unauthorized calls you need to implement your own security layer!

## Unreal Game Client
To allow the client to discover available GameLift sessions, it must communicate with your backend service rather than contacting GameLift directly. The backend exposes simple HTTP endpoints (in our example, via Google Cloud Functions) that return session data or create new sessions.

The following example shows how to query your backend from Unreal Engine using a C++ HTTP request and parse the returned JSON into your own session structures.

```c++
void UGLBSServiceConnector::GetSessions(FSearchComplete OnReady)
{	
	TFunction<void(const FJsonObject& Result, const FString& Error)> Done;
	FHttpModule& Module= FHttpModule::Get();
	TSharedRef<IHttpRequest, ESPMode::ThreadSafe> Request = Module.CreateRequest();

	Request->SetURL("https://<your-region>-<your-gcloud-projectname>.cloudfunctions.net/<your-function-name>");
	Request->SetVerb("GET");
	Request->SetHeader("Content-Type", "application/json");

	Request->OnProcessRequestComplete().BindLambda([OnReady](FHttpRequestPtr, FHttpResponsePtr Response,bool bOK)
	{
		const FString ResponseString = Response->GetContentAsString();
		TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(ResponseString);
		TSharedPtr<FJsonObject> Json;
		if (!FJsonSerializer::Deserialize(Reader,Json) || !Json.IsValid())
		{
			TArray<FGameSessionData> GameSessions;
			FGameSessionData data;
			GameSessions.Add(data);
			OnReady.Execute(GameSessions);
			return;
		}
		if (Json->HasField(FString(TEXT("GameSessions"))))
		{
			TArray<FGameSessionData> GameSessionsStruct;
			FGameSessionData data;
			TArray<TSharedPtr<FJsonValue>> GameSessionsJson = Json->GetArrayField(FString(TEXT("GameSessions")));
			for (TSharedPtr<FJsonValue> GameSession : GameSessionsJson)
			{
                //Create a struct or Object from the json result
				data = CreateGameSessionFromJson(GameSession);
				GameSessionsStruct.Add(data);
			}			
			OnReady.Execute(GameSessionsStruct);
			return;
		}
		TArray<FGameSessionData> GameSessions;
		FGameSessionData data;
		GameSessions.Add(data);
		OnReady.Execute(GameSessions);

	});

	Request->ProcessRequest();
}
```


## Communication ways

The following chart shows the Gamelift communication. Game communication between server and client is excluded.

![Overview](Documentation/Mermaid.jpg)



















