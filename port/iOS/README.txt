To build for iOS simply execute "./build_ios.sh" from this folder. Currently this script requires the latest SDK (5.0) from Apple in order to build properly. In the future I will add support for specifying the SDK version on the command line. 

Once the build is completed you will see a "ios" folder under /lib. This folder has sub folders for each of the following architectures:

* armv6 (Older Devices)
* armv7 (New Devices)
* i386 (Simulator)

Each of these folders contains a single static library for that architecture. In addition the libassimp.a file in the root of this folder is a combined archive (fat binary) library for all of the above architectures.

This port is being maintained by Matt Mathias <mmathias01@gmail.com> please contact him with any questions or comments.


